\
    #include <Arduino.h>
    #include <WiFi.h>
    #include <WebServer.h>
    #include <WiFiManager.h>
    #include <ArduinoJson.h>
    #include <Preferences.h>
    #include <LittleFS.h>
    #include <TFT_eSPI.h>
    #include <ArduinoOTA.h>
    #include <time.h>

    #include "config.h"

    // =========================
    // Globals / State
    // =========================
    TFT_eSPI tft = TFT_eSPI();
    WebServer server(HTTP_PORT);
    Preferences prefs;

    struct AppConfig {
      char tz[48] = DEFAULT_TZ;
      char ntp[64] = DEFAULT_NTP;
      bool use24h = DEFAULT_24H;

      uint8_t ledDiameter = DEFAULT_LED_DIAMETER;
      uint8_t ledGap = DEFAULT_LED_GAP;

      // LED color in 24-bit for web + convert to 565 for TFT
      uint32_t ledColor = 0xFF0000; // red
      uint8_t brightness = 255;     // 0..255 (TFT BL PWM if supported)
    };

    AppConfig cfg;

    // Logical LED matrix framebuffer: 0..255 intensity
    static uint8_t fb[LED_MATRIX_H][LED_MATRIX_W];

    // For web "mirror" - we send packed bytes (one byte per LED intensity)
    // Keep payload small: 128*32 = 4096 bytes
    static uint32_t lastMirrorMs = 0;

    // =========================
    // Utilities
    // =========================
    static uint16_t rgb888_to_565(uint32_t rgb) {
      uint8_t r = (rgb >> 16) & 0xFF;
      uint8_t g = (rgb >> 8) & 0xFF;
      uint8_t b = (rgb >> 0) & 0xFF;
      return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }

    static void fbClear(uint8_t v = 0) {
      memset(fb, v, sizeof(fb));
    }

    static inline void fbSet(int x, int y, uint8_t v) {
      if (x < 0 || y < 0 || x >= LED_MATRIX_W || y >= LED_MATRIX_H) return;
      fb[y][x] = v;
    }

    static inline uint8_t fbGet(int x, int y) {
      if (x < 0 || y < 0 || x >= LED_MATRIX_W || y >= LED_MATRIX_H) return 0;
      return fb[y][x];
    }

    // =========================
    // Simple 7-seg-ish digit bitmaps (16x24)
    // We morph between bitmaps by per-pixel blending over MORPH_STEPS frames.
    // This is intentionally simple; we can replace with the Harifun morphing algorithm later.
    // =========================
    struct Bitmap16x24 {
      // 24 rows, each row 16 bits (MSB left)
      uint16_t rows[24];
    };

    // Helper: build 7-seg style on a 16x24 grid
    static Bitmap16x24 makeDigit7Seg(uint8_t d) {
      // Segment layout:
      //  - a (top), b (upper right), c (lower right), d (bottom), e (lower left), f (upper left), g (middle)
      // thickness = 3, padding = 2
      bool seg[7] = {0};
      switch (d) {
        case 0: seg[0]=seg[1]=seg[2]=seg[3]=seg[4]=seg[5]=1; break;
        case 1: seg[1]=seg[2]=1; break;
        case 2: seg[0]=seg[1]=seg[6]=seg[4]=seg[3]=1; break;
        case 3: seg[0]=seg[1]=seg[6]=seg[2]=seg[3]=1; break;
        case 4: seg[5]=seg[6]=seg[1]=seg[2]=1; break;
        case 5: seg[0]=seg[5]=seg[6]=seg[2]=seg[3]=1; break;
        case 6: seg[0]=seg[5]=seg[6]=seg[4]=seg[2]=seg[3]=1; break;
        case 7: seg[0]=seg[1]=seg[2]=1; break;
        case 8: seg[0]=seg[1]=seg[2]=seg[3]=seg[4]=seg[5]=seg[6]=1; break;
        case 9: seg[0]=seg[1]=seg[2]=seg[3]=seg[5]=seg[6]=1; break;
        default: break;
      }

      Bitmap16x24 bm{};
      for (int y=0; y<24; y++) bm.rows[y]=0;

      auto setPx = [&](int x, int y){
        if (x<0||y<0||x>=16||y>=24) return;
        bm.rows[y] |= (1u << (15-x));
      };

      const int padX = 2;
      const int padY = 2;
      const int th = 3;
      const int w = 16;
      const int h = 24;
      const int midY = h/2;

      // a: y=padY .. padY+th-1, x=padX..w-padX-1
      if (seg[0]) for(int y=padY; y<padY+th; y++) for(int x=padX; x<w-padX; x++) setPx(x,y);
      // d: bottom
      if (seg[3]) for(int y=h-padY-th; y<h-padY; y++) for(int x=padX; x<w-padX; x++) setPx(x,y);
      // g: middle
      if (seg[6]) for(int y=midY - th/2; y<midY - th/2 + th; y++) for(int x=padX; x<w-padX; x++) setPx(x,y);

      // f: upper left
      if (seg[5]) for(int x=padX; x<padX+th; x++) for(int y=padY; y<midY; y++) setPx(x,y);
      // b: upper right
      if (seg[1]) for(int x=w-padX-th; x<w-padX; x++) for(int y=padY; y<midY; y++) setPx(x,y);
      // e: lower left
      if (seg[4]) for(int x=padX; x<padX+th; x++) for(int y=midY; y<h-padY; y++) setPx(x,y);
      // c: lower right
      if (seg[2]) for(int x=w-padX-th; x<w-padX; x++) for(int y=midY; y<h-padY; y++) setPx(x,y);

      return bm;
    }

    static Bitmap16x24 DIGITS[10];
    static Bitmap16x24 COLON;

    static void initBitmaps() {
      for (int i=0;i<10;i++) DIGITS[i] = makeDigit7Seg(i);
      // colon: two dots
      for (int y=0;y<24;y++) COLON.rows[y]=0;
      auto setPx = [&](int x, int y){
        if (x<0||y<0||x>=16||y>=24) return;
        COLON.rows[y] |= (1u << (15-x));
      };
      for(int yy=7; yy<10; yy++) for(int xx=7; xx<9; xx++) setPx(xx,yy);
      for(int yy=14; yy<17; yy++) for(int xx=7; xx<9; xx++) setPx(xx,yy);
    }

    // Draw a 16x24 bitmap into the logical LED grid with scaling to fit 32px height.
    // We target 32px height, so scale factor = 32/24 ~= 1.333; we do nearest-neighbor with y-expansion.
    static void drawBitmapToFB(const Bitmap16x24& bm, int x0, int y0, uint8_t intensity) {
      for (int y=0; y<24; y++) {
        for (int x=0; x<16; x++) {
          bool on = (bm.rows[y] >> (15-x)) & 0x1;
          if (!on) continue;

          // map 24->32 by duplicating some rows
          // simple: yScaled = y*32/24, and also fill next row if it changes (helps thickness)
          int y1 = (y * LED_MATRIX_H) / 24;
          int y2 = ((y+1) * LED_MATRIX_H) / 24;

          fbSet(x0 + x, y0 + y1, intensity);
          if (y2 != y1) fbSet(x0 + x, y0 + y2, intensity);
        }
      }
    }

    // Morph between two bitmaps into fb, step=0..MORPH_STEPS
    static void drawMorph(const Bitmap16x24& a, const Bitmap16x24& b, int step, int x0, int y0) {
      // per-pixel blend: intensity ramps from a->b
      // if a on and b off: fade out; if off->on: fade in; if both: stay on
      for (int y=0; y<24; y++) {
        for (int x=0; x<16; x++) {
          bool aon = (a.rows[y] >> (15-x)) & 0x1;
          bool bon = (b.rows[y] >> (15-x)) & 0x1;

          int val = 0;
          if (aon && bon) val = 255;
          else if (aon && !bon) val = (int)(255 * (float)(MORPH_STEPS - step) / (float)MORPH_STEPS);
          else if (!aon && bon) val = (int)(255 * (float)step / (float)MORPH_STEPS);
          else val = 0;

          if (val == 0) continue;

          int y1 = (y * LED_MATRIX_H) / 24;
          int y2 = ((y+1) * LED_MATRIX_H) / 24;

          fbSet(x0 + x, y0 + y1, (uint8_t)val);
          if (y2 != y1) fbSet(x0 + x, y0 + y2, (uint8_t)val);
        }
      }
    }

    // =========================
    // TFT rendering of the LED matrix emulation
    // =========================
    static void setBacklight(uint8_t b) {
    #ifdef TFT_BL
      static bool init = false;
      if (!init) {
        ledcSetup(0, 5000, 8);
        ledcAttachPin(TFT_BL, 0);
        init = true;
      }
      ledcWrite(0, b);
    #else
      (void)b;
    #endif
    }

    static void renderFBToTFT() {
      const uint16_t onColor = rgb888_to_565(cfg.ledColor);
      const uint16_t offColor = 0x0000; // black
      const int d = cfg.ledDiameter;
      const int gap = cfg.ledGap;
      const int pitch = d + gap;

      // Center the 128x32 grid on the TFT
      // Each "LED" becomes a filled circle/rounded-rect on TFT
      int gridW = LED_MATRIX_W * pitch - gap;
      int gridH = LED_MATRIX_H * pitch - gap;

      int xStart = (tft.width() - gridW) / 2;
      int yStart = (tft.height() - gridH) / 2;

      // Clear background
      tft.fillScreen(TFT_BLACK);

      // Render LEDs
      for (int y=0; y<LED_MATRIX_H; y++) {
        for (int x=0; x<LED_MATRIX_W; x++) {
          uint8_t v = fb[y][x];
          int px = xStart + x * pitch;
          int py = yStart + y * pitch;

          if (v == 0) {
            // draw "off" LED as dim outline (cheap)
            tft.drawRect(px, py, d, d, 0x2104); // very dark grey
          } else {
            // scale intensity by simple alpha on RGB565 (approx)
            // For now, just use onColor when v>0. Later: apply v to brightness.
            tft.fillRect(px, py, d, d, onColor);
          }
        }
      }
    }

    // =========================
    // Config persistence
    // =========================
    static void loadConfig() {
      prefs.begin("retroclock", true);
      String s;

      s = prefs.getString("tz", DEFAULT_TZ);
      strncpy(cfg.tz, s.c_str(), sizeof(cfg.tz)-1);

      s = prefs.getString("ntp", DEFAULT_NTP);
      strncpy(cfg.ntp, s.c_str(), sizeof(cfg.ntp)-1);

      cfg.use24h = prefs.getBool("24h", DEFAULT_24H);
      cfg.ledDiameter = (uint8_t)prefs.getUChar("ledd", DEFAULT_LED_DIAMETER);
      cfg.ledGap = (uint8_t)prefs.getUChar("ledg", DEFAULT_LED_GAP);
      cfg.ledColor = prefs.getUInt("col", 0xFF0000);
      cfg.brightness = (uint8_t)prefs.getUChar("bl", 255);

      prefs.end();
    }

    static void saveConfig() {
      prefs.begin("retroclock", false);
      prefs.putString("tz", cfg.tz);
      prefs.putString("ntp", cfg.ntp);
      prefs.putBool("24h", cfg.use24h);
      prefs.putUChar("ledd", cfg.ledDiameter);
      prefs.putUChar("ledg", cfg.ledGap);
      prefs.putUInt("col", cfg.ledColor);
      prefs.putUChar("bl", cfg.brightness);
      prefs.end();
    }

    // =========================
    // Time / NTP
    // =========================
    static void applyTimezone() {
      // Use IANA TZ database strings supported by ESP32 newlib.
      // Example: "Australia/Sydney"
      setenv("TZ", cfg.tz, 1);
      tzset();
    }

    static void startNtp() {
      applyTimezone();
      configTime(0, 0, cfg.ntp); // TZ handled by TZ env var
    }

    static bool getLocalTimeSafe(struct tm& timeinfo, uint32_t timeoutMs = 2000) {
      uint32_t start = millis();
      while ((millis() - start) < timeoutMs) {
        if (getLocalTime(&timeinfo, 50)) return true;
        delay(10);
      }
      return false;
    }

    // =========================
    // Web Handlers
    // =========================
    static void handleGetState() {
      struct tm ti{};
      bool ok = getLocalTimeSafe(ti, 300);
      char tbuf[16] = "--:--:--";
      char dbuf[16] = "----/--/--";
      if (ok) {
        strftime(tbuf, sizeof(tbuf), "%H:%M:%S", &ti);
        strftime(dbuf, sizeof(dbuf), "%Y-%m-%d", &ti);
      }

      StaticJsonDocument<512> doc;
      doc["time"] = tbuf;
      doc["date"] = dbuf;
      doc["wifi"] = (WiFi.isConnected() ? WiFi.SSID() : String("DISCONNECTED"));
      doc["ip"] = WiFi.isConnected() ? WiFi.localIP().toString() : String("0.0.0.0");
      doc["tz"] = cfg.tz;
      doc["ntp"] = cfg.ntp;
      doc["use24h"] = cfg.use24h;
      doc["ledDiameter"] = cfg.ledDiameter;
      doc["ledGap"] = cfg.ledGap;
      doc["ledColor"] = cfg.ledColor;
      doc["brightness"] = cfg.brightness;

      String out;
      serializeJson(doc, out);
      server.send(200, "application/json", out);
    }

    static void handlePostConfig() {
      if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "missing body");
        return;
      }

      StaticJsonDocument<512> doc;
      auto err = deserializeJson(doc, server.arg("plain"));
      if (err) {
        server.send(400, "text/plain", "bad json");
        return;
      }

      if (doc["tz"].is<const char*>()) {
        strlcpy(cfg.tz, doc["tz"].as<const char*>(), sizeof(cfg.tz));
      }
      if (doc["ntp"].is<const char*>()) {
        strlcpy(cfg.ntp, doc["ntp"].as<const char*>(), sizeof(cfg.ntp));
      }
      if (doc["use24h"].is<bool>()) cfg.use24h = doc["use24h"].as<bool>();
      if (doc["ledDiameter"].is<int>()) cfg.ledDiameter = (uint8_t)doc["ledDiameter"].as<int>();
      if (doc["ledGap"].is<int>()) cfg.ledGap = (uint8_t)doc["ledGap"].as<int>();
      if (doc["ledColor"].is<uint32_t>()) cfg.ledColor = doc["ledColor"].as<uint32_t>();
      if (doc["brightness"].is<int>()) cfg.brightness = (uint8_t)doc["brightness"].as<int>();

      // Basic sanity
      cfg.ledDiameter = constrain(cfg.ledDiameter, 2, 12);
      cfg.ledGap = constrain(cfg.ledGap, 0, 6);

      saveConfig();
      applyTimezone();
      startNtp();
      setBacklight(cfg.brightness);

      server.send(200, "application/json", "{\"ok\":true}");
    }

    static void handleGetMirror() {
      // Send raw bytes: 4096 bytes
      // Content-Type application/octet-stream
      server.sendHeader("Cache-Control", "no-store");
      server.send_P(200, "application/octet-stream", (const char*)fb, sizeof(fb));
    }

    static void serveStaticFiles() {
      server.serveStatic("/", LittleFS, "/index.html");
      server.serveStatic("/app.js", LittleFS, "/app.js");
      server.serveStatic("/style.css", LittleFS, "/style.css");
    }

    // =========================
    // WiFi setup (WiFiManager)
    // =========================
    static void startWifi() {
      WiFi.mode(WIFI_STA);

      WiFiManager wm;
      wm.setConfigPortalTimeout(180);
      wm.setConnectTimeout(20);

      // optional: expose timezone + ntp in captive portal later.
      bool ok = wm.autoConnect("CYD-RetroClock-Setup");
      if (!ok) {
        // fallback AP if it times out
        WiFi.mode(WIFI_AP);
        WiFi.softAP("CYD-RetroClock-AP");
      }
    }

    // =========================
    // OTA
    // =========================
    static void startOta() {
      ArduinoOTA.setHostname(OTA_HOSTNAME);
      ArduinoOTA.setPassword(OTA_PASSWORD);

      ArduinoOTA
        .onStart([]() {
          // optional: show OTA status
        })
        .onEnd([]() {
        })
        .onError([](ota_error_t error) {
          (void)error;
        });

      ArduinoOTA.begin();
    }

    // =========================
    // Clock render
    // =========================
    static void formatTime(struct tm& ti, char* out, size_t n) {
      if (cfg.use24h) strftime(out, n, "%H%M", &ti);
      else strftime(out, n, "%I%M", &ti);
    }

    static uint32_t lastSecond = 0;
    static char prevHHMM[5] = "----";
    static char currHHMM[5] = "----";
    static int morphStep = MORPH_STEPS;

    static void updateClockLogic() {
      struct tm ti{};
      if (!getLocalTimeSafe(ti, 50)) return;

      if ((uint32_t)ti.tm_sec != lastSecond) {
        lastSecond = (uint32_t)ti.tm_sec;

        char hhmm[5] = {0};
        formatTime(ti, hhmm, sizeof(hhmm));

        if (strncmp(hhmm, currHHMM, 4) != 0) {
          memcpy(prevHHMM, currHHMM, 5);
          memcpy(currHHMM, hhmm, 5);
          morphStep = 0;
        }
      }
    }

    static void drawFrame() {
      fbClear(0);

      // positions in 128x32 grid
      // Four digits (16px each) + colon (16px) + margins
      const int digitW = 16;
      const int colonW = 16;
      const int totalW = digitW*4 + colonW;
      int x0 = (LED_MATRIX_W - totalW) / 2;
      int y0 = 0;

      auto digitIdx = [&](char c)->int { return (c>='0' && c<='9') ? (c-'0') : 0; };

      // Build morphing for each digit between prev and curr
      int d0 = digitIdx(currHHMM[0]);
      int d1 = digitIdx(currHHMM[1]);
      int d2 = digitIdx(currHHMM[2]);
      int d3 = digitIdx(currHHMM[3]);

      int p0 = digitIdx(prevHHMM[0]);
      int p1 = digitIdx(prevHHMM[1]);
      int p2 = digitIdx(prevHHMM[2]);
      int p3 = digitIdx(prevHHMM[3]);

      int step = morphStep;
      if (step > MORPH_STEPS) step = MORPH_STEPS;

      drawMorph(DIGITS[p0], DIGITS[d0], step, x0 + 0*digitW, y0);
      drawMorph(DIGITS[p1], DIGITS[d1], step, x0 + 1*digitW, y0);
      drawMorph(COLON, COLON, 0,            x0 + 2*digitW, y0);
      drawMorph(DIGITS[p2], DIGITS[d2], step, x0 + 2*digitW + colonW, y0);
      drawMorph(DIGITS[p3], DIGITS[d3], step, x0 + 3*digitW + colonW, y0);

      // Animate colon blink (simple)
      if ((lastSecond % 2) == 0) {
        // turn colon off by clearing its region
        for (int yy=0; yy<LED_MATRIX_H; yy++)
          for (int xx=x0 + 2*digitW; xx<x0 + 2*digitW + colonW; xx++)
            fbSet(xx, yy, 0);
      }

      // advance morph
      if (morphStep < MORPH_STEPS) morphStep++;
    }

    // =========================
    // Setup / Loop
    // =========================
    void setup() {
      Serial.begin(115200);
      delay(200);

      initBitmaps();
      loadConfig();

      if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed");
      }

      // TFT
      tft.init();
      tft.setRotation(1); // landscape
      tft.fillScreen(TFT_BLACK);
      setBacklight(cfg.brightness);

      // WiFi
      startWifi();

      // NTP
      startNtp();

      // OTA
      startOta();

      // Web
      serveStaticFiles();
      server.on("/api/state", HTTP_GET, handleGetState);
      server.on("/api/config", HTTP_POST, handlePostConfig);
      server.on("/api/mirror", HTTP_GET, handleGetMirror);
      server.begin();

      Serial.print("Ready. IP: ");
      Serial.println(WiFi.isConnected() ? WiFi.localIP() : IPAddress(0,0,0,0));
    }

    void loop() {
      ArduinoOTA.handle();
      server.handleClient();

      updateClockLogic();

      static uint32_t lastFrame = 0;
      uint32_t now = millis();
      if (now - lastFrame >= FRAME_MS) {
        lastFrame = now;
        drawFrame();
        renderFBToTFT();
      }
    }
