/*
 * CYD LED Matrix Retro Clock
 * Version: 1.0.0
 *
 * A retro-style LED matrix clock for the ESP32-2432S028 (CYD - Cheap Yellow Display)
 *
 * FEATURES:
 * - 64×32 virtual LED matrix emulation on 320×240 TFT display
 * - Large 7-segment style digits with morphing animations
 * - WiFi configuration via WiFiManager (AP mode fallback)
 * - NTP time synchronization with timezone support
 * - Web-based configuration interface with live display mirror
 * - Adjustable LED appearance (diameter, gap, color, brightness)
 * - Status bar showing WiFi, IP, and date information
 * - OTA firmware updates for easy maintenance
 * - LittleFS-based web file serving
 *
 * HARDWARE:
 * - ESP32-2432S028 (CYD) - 2.8" ILI9341 320×240 TFT display
 * - Built-in backlight control
 * - WiFi connectivity
 *
 * DISPLAY LAYOUT:
 * - Top: Large clock digits (HH:MM:SS) rendered as LED matrix
 * - Bottom: Status bar (WiFi, IP address, date)
 *
 * CONFIGURATION:
 * - First boot: Creates WiFi AP "CYD-RetroClock-Setup"
 * - Connect and configure WiFi credentials
 * - Access web UI at device IP address
 * - Adjust timezone, NTP server, LED appearance
 *
 * WEB API ENDPOINTS:
 * - GET  /              - Main web interface
 * - GET  /api/state     - Current system state (JSON)
 * - POST /api/config    - Update configuration
 * - GET  /api/mirror    - Raw framebuffer data for display mirror
 *
 * Author: Built with Claude Code
 * License: MIT
 */

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
// Debug Output Macros
// =========================
#ifndef DEBUG_MODE
#define DEBUG_MODE 1
#endif

#if DEBUG_MODE
  #define DBG(...)        Serial.printf(__VA_ARGS__)
  #define DBGLN(s)        Serial.println(s)
  #define DBG_STEP(s)     do { Serial.print("[INIT] "); Serial.println(s); } while (0)
  #define DBG_OK(s)       do { Serial.print("[ OK ] "); Serial.println(s); } while (0)
  #define DBG_WARN(s)     do { Serial.print("[WARN] "); Serial.println(s); } while (0)
  #define DBG_ERR(s)      do { Serial.print("[ERR ] "); Serial.println(s); } while (0)
#else
  #define DBG(...)        do {} while (0)
  #define DBGLN(s)        do {} while (0)
  #define DBG_STEP(s)     do {} while (0)
  #define DBG_OK(s)       do {} while (0)
  #define DBG_WARN(s)     do {} while (0)
  #define DBG_ERR(s)      do {} while (0)
#endif

// =========================
// Global Objects & Application State
// =========================
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

WebServer server(HTTP_PORT);
Preferences prefs;

struct AppConfig {
  char tz[48]   = DEFAULT_TZ;
  char ntp[64]  = DEFAULT_NTP;
  bool use24h   = DEFAULT_24H;

  uint8_t ledDiameter = DEFAULT_LED_DIAMETER;
  uint8_t ledGap      = DEFAULT_LED_GAP;

  // LED color in 24-bit for web + convert to 565 for TFT
  uint32_t ledColor = 0xFF0000; // red
  uint8_t brightness = 255;     // 0..255
};

AppConfig cfg;

// Logical LED matrix framebuffer: 0..255 intensity
static uint8_t fb[LED_MATRIX_H][LED_MATRIX_W];

// Cached date string for status bar
static char currDate[11] = "----/--/--";
static uint8_t appliedDot = 0;
static uint8_t appliedGap = 0;
static uint8_t appliedPitch = 0;

// Sprite settings for flicker-free debug renderer
static int fbPitch = 2;      // logical LED -> TFT pixels (computed from TFT size + config)
static bool useSprite = false;

// =========================
// Utility Functions
// =========================

/**
 * Convert 24-bit RGB (0xRRGGBB) to 16-bit RGB565 format for TFT display
 * @param rgb 24-bit RGB color value
 * @return 16-bit RGB565 color value
 */
static uint16_t rgb888_to_565(uint32_t rgb) {
  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (rgb >> 8) & 0xFF;
  uint8_t b = (rgb >> 0) & 0xFF;
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/**
 * Clear the entire framebuffer to a specific intensity value
 * @param v Intensity value (0-255), default 0 (off)
 */
static void fbClear(uint8_t v = 0) { memset(fb, v, sizeof(fb)); }

/**
 * Set a single pixel in the framebuffer with bounds checking
 * @param x X coordinate (0 to LED_MATRIX_W-1)
 * @param y Y coordinate (0 to LED_MATRIX_H-1)
 * @param v Intensity value (0-255)
 */
static inline void fbSet(int x, int y, uint8_t v) {
  if (x < 0 || y < 0 || x >= LED_MATRIX_W || y >= LED_MATRIX_H) return;
  fb[y][x] = v;
}

// =========================
// 7-Segment Digit Bitmaps & Layout Constants
// =========================
static const int DIGIT_W = 9;       // Width of each digit in pixels (9px fits HH:MM:SS with gaps in 64px)
static const int DIGIT_H = LED_MATRIX_H;  // Height matches full matrix height (32px)
static const int COLON_W = 2;       // Width of colon separator
static const int DIGIT_GAP = 1;     // 1px gap between digits for improved readability

struct Bitmap {
  uint16_t rows[DIGIT_H];  // each row is 16 bits, MSB left
};

static Bitmap DIGITS[10];  // Array of digit bitmaps (0-9)
static Bitmap COLON;       // Colon separator bitmap

/**
 * Generate a 7-segment style digit bitmap
 * Segments are labeled a-g in standard 7-segment notation:
 *     aaa
 *    f   b
 *     ggg
 *    e   c
 *     ddd
 * @param d Digit value (0-9)
 * @return Bitmap structure containing the rendered digit
 */
static Bitmap makeDigit7Seg(uint8_t d) {
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

  Bitmap bm{};
  for (int y=0; y<DIGIT_H; y++) bm.rows[y]=0;

  auto setPx = [&](int x, int y){
    if (x<0||y<0||x>=DIGIT_W||y>=DIGIT_H) return;
    bm.rows[y] |= (1u << (15-x));
  };

  const int padX = 0;
  const int padY = 1;
  const int th = 4;
  const int w = DIGIT_W;
  const int h = DIGIT_H;
  const int midY = h/2;

  if (seg[0]) for(int y=padY; y<padY+th; y++) for(int x=padX; x<w-padX; x++) setPx(x,y);            // a
  if (seg[3]) for(int y=h-padY-th; y<h-padY; y++) for(int x=padX; x<w-padX; x++) setPx(x,y);          // d
  if (seg[6]) for(int y=midY - th/2; y<midY - th/2 + th; y++) for(int x=padX; x<w-padX; x++) setPx(x,y); // g

  if (seg[5]) for(int x=padX; x<padX+th; x++) for(int y=padY; y<midY; y++) setPx(x,y);               // f
  if (seg[1]) for(int x=w-padX-th; x<w-padX; x++) for(int y=padY; y<midY; y++) setPx(x,y);           // b
  if (seg[4]) for(int x=padX; x<padX+th; x++) for(int y=midY; y<h-padY; y++) setPx(x,y);             // e
  if (seg[2]) for(int x=w-padX-th; x<w-padX; x++) for(int y=midY; y<h-padY; y++) setPx(x,y);         // c

  return bm;
}

/**
 * Initialize all digit and colon bitmaps
 * Called once during setup to pre-render all characters
 */
static void initBitmaps() {
  DBG_STEP("Building digit bitmaps...");
  for (int i=0;i<10;i++) DIGITS[i] = makeDigit7Seg(i);

  for (int y=0;y<DIGIT_H;y++) COLON.rows[y]=0;
  auto setPx = [&](int x, int y){
    if (x<0||y<0||x>=COLON_W||y>=DIGIT_H) return;
    COLON.rows[y] |= (1u << (15-x));
  };
  for(int yy=10; yy<13; yy++) for(int xx=0; xx<COLON_W; xx++) setPx(xx,yy);
  for(int yy=19; yy<22; yy++) for(int xx=0; xx<COLON_W; xx++) setPx(xx,yy);

  DBG_OK("Digit bitmaps ready.");
}

// Morph between two bitmaps into fb, step=0..MORPH_STEPS
static void drawMorph(const Bitmap& a, const Bitmap& b, int step, int x0, int y0, int w) {
  for (int y=0; y<DIGIT_H; y++) {
    for (int x=0; x<w; x++) {
      bool aon = (a.rows[y] >> (15-x)) & 0x1;
      bool bon = (b.rows[y] >> (15-x)) & 0x1;

      uint8_t val = 0;
      if (aon && bon) val = 255;
      else if (aon && !bon) val = (uint8_t)(255 * (MORPH_STEPS - step) / MORPH_STEPS);
      else if (!aon && bon) val = (uint8_t)(255 * step / MORPH_STEPS);

      if (val == 0) continue;

      int yScaled = (y * LED_MATRIX_H) / DIGIT_H;
      fbSet(x0 + x, y0 + yScaled, val);
    }
  }
}

struct Pt { int8_t x, y; };

static int buildPixelsFromBitmap(const Bitmap& bm, int w, Pt* out, int maxOut) {
  int n = 0;
  for (int y = 0; y < DIGIT_H; y++) {
    uint16_t row = bm.rows[y];
    for (int x = 0; x < w; x++) {
      bool on = (row >> (15 - x)) & 0x1;
      if (!on) continue;
      if (n < maxOut) out[n] = Pt{(int8_t)x, (int8_t)y};
      n++;
    }
  }
  return n;
}

static inline int dist2(const Pt& a, const Pt& b) {
  int dx = (int)a.x - (int)b.x;
  int dy = (int)a.y - (int)b.y;
  return dx*dx + dy*dy;
}

// Particle morph between two bitmaps into fb, step=0..MORPH_STEPS
static void drawParticleMorph(const Bitmap& fromBm,
                              const Bitmap& toBm,
                              int step, int x0, int y0,
                              int w)
{
  // Worst case: almost full 16x24 = 384 pixels.
  // Our 7-seg glyphs are much smaller, but allocate safely.
  static Pt fromPts[420];
  static Pt toPts[420];
  static int matchTo[420];
  static bool toUsed[420];

  int fromN = buildPixelsFromBitmap(fromBm, w, fromPts, 420);
  int toN   = buildPixelsFromBitmap(toBm,   w, toPts,   420);

  // Clamp counts to our buffers
  if (fromN > 420) fromN = 420;
  if (toN > 420) toN = 420;

  // Greedy nearest-neighbour matching (good enough for small glyphs)
  for (int j=0; j<toN; j++) toUsed[j] = false;

  int pairs = min(fromN, toN);
  for (int i=0; i<pairs; i++) {
    int bestJ = -1;
    int bestD = 1e9;
    for (int j=0; j<toN; j++) {
      if (toUsed[j]) continue;
      int d = dist2(fromPts[i], toPts[j]);
      if (d < bestD) { bestD = d; bestJ = j; }
    }
    if (bestJ < 0) bestJ = 0;
    matchTo[i] = bestJ;
    toUsed[bestJ] = true;
  }

  // Interp factor 0..1
  float t = (float)step / (float)MORPH_STEPS;

  // 1) Move matched particles
  for (int i=0; i<pairs; i++) {
    Pt a = fromPts[i];
    Pt b = toPts[matchTo[i]];

    float xf = a.x + (b.x - a.x) * t;
    float yf = a.y + (b.y - a.y) * t;

    int x = (int)lroundf(xf);
    int y = (int)lroundf(yf);

    int yScaled = (y * LED_MATRIX_H) / DIGIT_H;

    // Full intensity (motion provides the morph effect)
    fbSet(x0 + x, y0 + yScaled, 255);
  }

  // 2) Pixels that exist only in TO: fade in
  if (toN > fromN) {
    int extra = toN - fromN;
    float alpha = t; // 0->1
    for (int j=0; j<toN && extra>0; j++) {
      if (toUsed[j]) continue;
      Pt p = toPts[j];
      int yScaled = (p.y * LED_MATRIX_H) / DIGIT_H;
      fbSet(x0 + p.x, y0 + yScaled, (uint8_t)(255 * alpha));
      extra--;
    }
  }

  // 3) Pixels that exist only in FROM: fade out
  if (fromN > toN) {
    float alpha = 1.0f - t; // 1->0
    for (int i=toN; i<fromN; i++) {
      Pt p = fromPts[i];
      int yScaled = (p.y * LED_MATRIX_H) / DIGIT_H;
      fbSet(x0 + p.x, y0 + yScaled, (uint8_t)(255 * alpha));
    }
  }
}


// =========================
// Backlight (PWM if TFT_BL exists)
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

// =========================
// Flicker-free renderer using SMALL sprite (with intensity)
// =========================
static int computeRenderPitch() {
  int matrixAreaH = tft.height() - STATUS_BAR_H;
  if (matrixAreaH < 1) matrixAreaH = tft.height();

  // With CYD 320x240 landscape:
  // pitch = min(320/64, matrixAreaH/32) = min(5, matrixAreaH/32)
  // Maximum pitch is 5 (limited by width), giving 320×160 display
  // Use min to ensure it fits in both dimensions
  int maxPitch = min(tft.width() / LED_MATRIX_W, matrixAreaH / LED_MATRIX_H);
  if (maxPitch < 1) maxPitch = 1;
  return maxPitch;
}

static void rebuildSprite(int pitch) {
  if (useSprite) {
    spr.deleteSprite();
    useSprite = false;
  }

  spr.setColorDepth(16);
  int sprW = LED_MATRIX_W * pitch;
  int sprH = LED_MATRIX_H * pitch;
  if (sprW <= 0 || sprH <= 0) return;

  if (spr.createSprite(sprW, sprH)) {
    useSprite = true;
    spr.fillSprite(TFT_BLACK);
  } else {
    useSprite = false;
  }
}

static void updateRenderPitch(bool force = false) {
  int pitch = computeRenderPitch();
  if (!force && pitch == fbPitch && useSprite) return;
  fbPitch = pitch;
  rebuildSprite(fbPitch);
}

static void drawStatusBar() {
#if STATUS_BAR_H > 0
  static uint32_t lastDrawMs = 0;
  static char lastLine1[64] = "";
  static char lastLine2[64] = "";

  int barY = tft.height() - STATUS_BAR_H;
  if (barY < 0) barY = tft.height();

  char line1[64];
  if (WiFi.isConnected()) {
    snprintf(line1, sizeof(line1), "WIFI: %s  IP: %s",
             WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  } else {
    snprintf(line1, sizeof(line1), "WIFI: AP MODE");
  }
  char line2[64];
  snprintf(line2, sizeof(line2), "%s  LED: d%u g%u p%u (dot%u gap%u)",
           currDate, (unsigned)cfg.ledDiameter, (unsigned)cfg.ledGap,
           (unsigned)appliedPitch, (unsigned)appliedDot, (unsigned)appliedGap);

  uint32_t now = millis();
  bool changed = (strncmp(line1, lastLine1, sizeof(line1)) != 0) ||
                 (strncmp(line2, lastLine2, sizeof(line2)) != 0);
  if (!changed && (now - lastDrawMs) < 1000) return;

  strlcpy(lastLine1, line1, sizeof(lastLine1));
  strlcpy(lastLine2, line2, sizeof(lastLine2));
  lastDrawMs = now;

  tft.fillRect(0, barY, tft.width(), STATUS_BAR_H, TFT_BLACK);
  tft.drawFastHLine(0, barY, tft.width(), TFT_DARKGREY);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.drawString(line1, 6, barY + 6);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(line2, 6, barY + 24);
#endif
}

static void renderFBToTFT() {
  const int pitch = fbPitch;
  const int sprW = LED_MATRIX_W * pitch; // 320 when 64x32 with pitch 5
  const int sprH = LED_MATRIX_H * pitch; // 160 when 64x32 with pitch 5

  int matrixAreaH = tft.height() - STATUS_BAR_H;
  if (matrixAreaH < sprH) matrixAreaH = tft.height();

  int x0 = (tft.width()  - sprW) / 2;
  int y0 = (matrixAreaH - sprH) / 2;

  // Base RGB components from cfg.ledColor
  const uint8_t baseR = (cfg.ledColor >> 16) & 0xFF;
  const uint8_t baseG = (cfg.ledColor >> 8)  & 0xFF;
  const uint8_t baseB = (cfg.ledColor >> 0)  & 0xFF;

  int gapWanted = (int)cfg.ledGap;
  if (gapWanted < 0) gapWanted = 0;
  if (gapWanted > pitch - 1) gapWanted = pitch - 1;

  int dot = pitch - gapWanted;
  int maxDot = (int)cfg.ledDiameter;
  if (maxDot < 1) maxDot = 1;
  if (dot > maxDot) dot = maxDot;
  if (dot < 1) dot = 1;

  int gap = pitch - dot;
  const int inset = (pitch - dot) / 2;
  appliedDot = (uint8_t)dot;
  appliedGap = (uint8_t)gap;
  appliedPitch = (uint8_t)pitch;

  // Debug output (print once per second)
  static uint32_t lastDbg = 0;
  if (millis() - lastDbg > 1000) {
    DBG("[RENDER] pitch=%d dot=%d gap=%d inset=%d ledD=%d ledG=%d\n",
        pitch, dot, gap, inset, cfg.ledDiameter, cfg.ledGap);
    lastDbg = millis();
  }

  if (useSprite) {
    spr.fillSprite(TFT_BLACK);

    for (int y = 0; y < LED_MATRIX_H; y++) {
      for (int x = 0; x < LED_MATRIX_W; x++) {
        uint8_t v = fb[y][x];
        if (!v) continue;

        // Scale base color by intensity v (0..255)
        uint8_t r = (uint8_t)((baseR * (uint16_t)v) / 255);
        uint8_t g = (uint8_t)((baseG * (uint16_t)v) / 255);
        uint8_t b = (uint8_t)((baseB * (uint16_t)v) / 255);

        uint16_t colScaled = rgb888_to_565(((uint32_t)r << 16) | ((uint32_t)g << 8) | b);

        spr.fillRect(x * pitch + inset, y * pitch + inset, dot, dot, colScaled);
      }
    }

    // No need to clear TFT region; sprite fully covers its area
    tft.startWrite();
    spr.pushSprite(x0, y0);
    tft.endWrite();
    drawStatusBar();
    return;
  }

  // -------------------------
  // Fallback (direct draw): slower but correct
  // -------------------------
  tft.fillScreen(TFT_BLACK);

  for (int y = 0; y < LED_MATRIX_H; y++) {
    for (int x = 0; x < LED_MATRIX_W; x++) {
      uint8_t v = fb[y][x];
      if (!v) continue;

      uint8_t r = (uint8_t)((baseR * (uint16_t)v) / 255);
      uint8_t g = (uint8_t)((baseG * (uint16_t)v) / 255);
      uint8_t b = (uint8_t)((baseB * (uint16_t)v) / 255);

      uint16_t colScaled = rgb888_to_565(((uint32_t)r << 16) | ((uint32_t)g << 8) | b);

      tft.fillRect(x0 + x * pitch + inset, y0 + y * pitch + inset, dot, dot, colScaled);
    }
  }

  drawStatusBar();
}


// =========================
// Config persistence
// =========================
static void loadConfig() {
  DBG_STEP("Loading config from NVS...");
  prefs.begin("retroclock", true);

  String s;

  s = prefs.getString("tz", DEFAULT_TZ);
  strlcpy(cfg.tz, s.c_str(), sizeof(cfg.tz));

  s = prefs.getString("ntp", DEFAULT_NTP);
  strlcpy(cfg.ntp, s.c_str(), sizeof(cfg.ntp));

  cfg.use24h = prefs.getBool("24h", DEFAULT_24H);
  cfg.ledDiameter = (uint8_t)prefs.getUChar("ledd", DEFAULT_LED_DIAMETER);
  cfg.ledGap = (uint8_t)prefs.getUChar("ledg", DEFAULT_LED_GAP);
  cfg.ledColor = prefs.getUInt("col", 0xFF0000);
  cfg.brightness = (uint8_t)prefs.getUChar("bl", 255);

  prefs.end();

  DBG("  TZ: %s\n", cfg.tz);
  DBG("  NTP: %s\n", cfg.ntp);
  DBG("  24h: %s\n", cfg.use24h ? "true" : "false");
  DBG("  Color: #%06X\n", (unsigned)cfg.ledColor);
  DBG("  Brightness: %u\n", cfg.brightness);

  DBG_OK("Config loaded.");
}

static void saveConfig() {
  DBG_STEP("Saving config to NVS...");
  prefs.begin("retroclock", false);
  prefs.putString("tz", cfg.tz);
  prefs.putString("ntp", cfg.ntp);
  prefs.putBool("24h", cfg.use24h);
  prefs.putUChar("ledd", cfg.ledDiameter);
  prefs.putUChar("ledg", cfg.ledGap);
  prefs.putUInt("col", cfg.ledColor);
  prefs.putUChar("bl", cfg.brightness);
  prefs.end();
  DBG_OK("Config saved.");
}

// =========================
// Time / NTP
// =========================
static const char* tzIanaToPosix(const char* tz) {
  if (!tz || !tz[0]) return "UTC0";

  // ESP32 newlib expects POSIX TZ strings, not IANA names.
  if (strcmp(tz, "Australia/Sydney") == 0) {
    return "AEST-10AEDT,M10.1.0,M4.1.0/3";
  }
  if (strcmp(tz, "Australia/Melbourne") == 0) {
    return "AEST-10AEDT,M10.1.0,M4.1.0/3";
  }

  return tz;
}

static void startNtp() {
  DBG_STEP("Starting NTP...");
  const char* tzEnv = tzIanaToPosix(cfg.tz);
  configTzTime(tzEnv, cfg.ntp);
  DBG_OK("NTP configured.");
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
// Web handlers
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

  JsonDocument doc;
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

  JsonDocument doc;
  auto err = deserializeJson(doc, server.arg("plain"));
  if (err) {
    server.send(400, "text/plain", "bad json");
    return;
  }

  if (doc["tz"].is<const char*>()) strlcpy(cfg.tz, doc["tz"].as<const char*>(), sizeof(cfg.tz));
  if (doc["ntp"].is<const char*>()) strlcpy(cfg.ntp, doc["ntp"].as<const char*>(), sizeof(cfg.ntp));
  if (doc["use24h"].is<bool>()) cfg.use24h = doc["use24h"].as<bool>();
  if (!doc["ledDiameter"].isNull()) cfg.ledDiameter = (uint8_t)doc["ledDiameter"].as<int>();
  if (!doc["ledGap"].isNull()) cfg.ledGap = (uint8_t)doc["ledGap"].as<int>();
  if (doc["ledColor"].is<uint32_t>()) cfg.ledColor = doc["ledColor"].as<uint32_t>();
  if (doc["brightness"].is<int>()) cfg.brightness = (uint8_t)doc["brightness"].as<int>();

  // Constrain LED rendering parameters
  // ledDiameter: max size of each LED dot (pitch is typically 5 for 320x240)
  // ledGap: space between LEDs (gap + dot <= pitch)
  cfg.ledDiameter = constrain(cfg.ledDiameter, 1, 10);
  cfg.ledGap      = constrain(cfg.ledGap, 0, 8);

  DBG("[CONFIG] Saved: ledD=%d ledG=%d col=%06X bl=%d\n",
      cfg.ledDiameter, cfg.ledGap, cfg.ledColor, cfg.brightness);

  saveConfig();
  updateRenderPitch();  // Rebuild sprite if pitch changed
  startNtp();
  setBacklight(cfg.brightness);

  server.send(200, "application/json", "{\"ok\":true}");
}

static void handleGetMirror() {
  const size_t fbSize = LED_MATRIX_W * LED_MATRIX_H;  // 64 * 32 = 2048
  DBG("[MIRROR] Sending %u bytes (expected: %u, sizeof: %u)\n",
      (unsigned)fbSize, (unsigned)fbSize, (unsigned)sizeof(fb));
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "application/octet-stream", (const char*)fb, fbSize);
}

static void serveStaticFiles() {
  server.on("/", HTTP_GET, []() {
    File f = LittleFS.open("/index.html", "r");
    if (!f) {
      server.send(404, "text/plain", "Not found");
      return;
    }
    server.streamFile(f, "text/html");
    f.close();
  });
  server.serveStatic("/app.js", LittleFS, "/app.js");
  server.serveStatic("/style.css", LittleFS, "/style.css");

  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });
}

// =========================
// WiFi setup
// =========================
static void startWifi() {
  DBG_STEP("Starting WiFi (STA) + WiFiManager...");
  WiFi.mode(WIFI_STA);

  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  wm.setConnectTimeout(20);

  bool ok = wm.autoConnect("CYD-RetroClock-Setup");
  if (!ok) {
    DBG_WARN("WiFiManager autoConnect failed/timeout. Starting fallback AP...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("CYD-RetroClock-AP");
  }

  if (WiFi.isConnected()) {
    DBG("WiFi connected: SSID=%s IP=%s\n",
        WiFi.SSID().c_str(),
        WiFi.localIP().toString().c_str());
    DBG_OK("WiFi ready.");
  } else {
    DBG_WARN("WiFi not connected (AP mode).");
  }
}

// =========================
// OTA
// =========================
static void startOta() {
  DBG_STEP("Starting OTA...");
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() { DBG_OK("OTA start"); });
  ArduinoOTA.onEnd([]() { DBG_OK("OTA end"); });
  ArduinoOTA.onError([](ota_error_t error) {
    DBG("[ERR ] OTA error: %u\n", (unsigned)error);
  });

  ArduinoOTA.begin();
  DBG_OK("OTA ready.");
}

// =========================
// Clock logic & drawing
// =========================
static void formatTimeHHMMSS(struct tm& ti, char* out, size_t n) {
  if (cfg.use24h) strftime(out, n, "%H%M%S", &ti);
  else strftime(out, n, "%I%M%S", &ti);
}


static uint32_t lastSecond = 0;
static char prevT[7] = "------";
static char currT[7] = "------";
static int morphStep = MORPH_STEPS;

static void updateClockLogic() {
  struct tm ti{};
  if (!getLocalTimeSafe(ti, 50)) return;

  if ((uint32_t)ti.tm_sec == lastSecond) return;
  lastSecond = (uint32_t)ti.tm_sec;

  char t6[7] = {0};
  formatTimeHHMMSS(ti, t6, sizeof(t6));
  strftime(currDate, sizeof(currDate), "%Y-%m-%d", &ti);

  if (strncmp(t6, currT, 6) != 0) {
    memcpy(prevT, currT, 7);
    memcpy(currT, t6, 7);
    morphStep = 0;
    DBG("[TIME] %.2s:%.2s:%.2s\n", currT, currT+2, currT+4);
  }
}

/**
 * Draw a bitmap to the framebuffer with specified intensity
 * @param bm Bitmap to render
 * @param x0 X position in framebuffer
 * @param y0 Y position in framebuffer
 * @param w Width of bitmap
 * @param intensity Brightness level (0-255), default 255
 */
static void drawBitmapSolid(const Bitmap& bm, int x0, int y0, int w, uint8_t intensity = 255) {
  for (int y=0; y<DIGIT_H; y++) {
    for (int x=0; x<w; x++) {
      bool on = (bm.rows[y] >> (15-x)) & 0x1;
      if (!on) continue;
      int yScaled = (y * LED_MATRIX_H) / DIGIT_H;
      fbSet(x0 + x, y0 + yScaled, intensity);
    }
  }
}

/**
 * Animated "spawn" morph effect for digit transitions
 * Pixels appear from random positions and move into their final positions
 * @param toBm Target bitmap to morph into
 * @param step Current animation step (0 to MORPH_STEPS)
 * @param x0 X position in framebuffer
 * @param y0 Y position in framebuffer
 * @param w Width of bitmap
 */
static void drawSpawnMorphToTarget(const Bitmap& toBm, int step, int x0, int y0, int w) {
  // Gather all ON pixels in target glyph
  static Pt toPts[420];
  int toN = buildPixelsFromBitmap(toBm, w, toPts, 420);
  if (toN > 420) toN = 420;

  // 0..1
  float t = (float)step / (float)MORPH_STEPS;
  if (t < 0) t = 0;
  if (t > 1) t = 1;

  // Ease-out (nice “snap into place”)
  float te = 1.0f - (1.0f - t) * (1.0f - t);

  // Spawn origin inside the glyph (center-ish)
  const float sx = (float)(w - 1) * 0.5f;
  const float sy = (float)(DIGIT_H - 1) * 0.5f;

  // Fade-in as it moves
  uint8_t alpha = (uint8_t)(255 * t);

  for (int i=0; i<toN; i++) {
    float tx = (float)toPts[i].x;
    float ty = (float)toPts[i].y;

    float xf = sx + (tx - sx) * te;
    float yf = sy + (ty - sy) * te;

    int x = (int)lroundf(xf);
    int y = (int)lroundf(yf);

    int yScaled = (y * LED_MATRIX_H) / DIGIT_H;
    fbSet(x0 + x, y0 + yScaled, alpha);
  }
}

/**
 * Main frame rendering function - draws the complete clock display
 * Renders HH:MM:SS format with morphing animations on digit changes
 * Layout: 6 digits + 2 colons + 5 gaps, centered horizontally at top
 */
static void drawFrame() {
  fbClear(0);

  const int digitW = DIGIT_W;
  const int colonW = COLON_W;
  const int gap = DIGIT_GAP;

  // HH:MM:SS with gaps between digit pairs for readability
  // Total width = (6 * digitW) + (2 * colonW) + (5 * gap)
  // Gaps: after each digit except the last one in each pair
  const int totalW = (6 * digitW) + (2 * colonW) + (5 * gap);
  int x0 = (LED_MATRIX_W - totalW) / 2;
  if (x0 < 0) x0 = 0;
  const int y0 = 0;  // Clock at top of display

  auto digitIdx = [&](char c)->int { return (c>='0' && c<='9') ? (c-'0') : 0; };

  // Indices for each digit
  int c[6] = {
    digitIdx(currT[0]), digitIdx(currT[1]),
    digitIdx(currT[2]), digitIdx(currT[3]),
    digitIdx(currT[4]), digitIdx(currT[5])
  };
  int p[6] = {
    digitIdx(prevT[0]), digitIdx(prevT[1]),
    digitIdx(prevT[2]), digitIdx(prevT[3]),
    digitIdx(prevT[4]), digitIdx(prevT[5])
  };

  int step = morphStep;
  if (step > MORPH_STEPS) step = MORPH_STEPS;

  auto drawDigit = [&](int pos, int xx) {
    if (currT[pos] != prevT[pos] && step < MORPH_STEPS) {
      // Digit changed → redraw whole digit with spawn morph
      drawSpawnMorphToTarget(DIGITS[c[pos]], step, xx, y0, digitW);
    } else {
      // Digit unchanged or morph finished → solid draw
      drawBitmapSolid(DIGITS[c[pos]], xx, y0, digitW, 255);
    }
  };

  // HH with gap between digits
  drawDigit(0, x0);
  drawDigit(1, x0 + digitW + gap);

  // :
  drawBitmapSolid(COLON, x0 + 2*digitW + gap, y0, colonW, 255);

  // MM with gap between digits
  drawDigit(2, x0 + 2*digitW + gap + colonW + gap);
  drawDigit(3, x0 + 3*digitW + 2*gap + colonW + gap);

  // :
  drawBitmapSolid(COLON, x0 + 4*digitW + 2*gap + colonW + gap, y0, colonW, 255);

  // SS with gap between digits
  drawDigit(4, x0 + 4*digitW + 2*gap + 2*colonW + 2*gap);
  drawDigit(5, x0 + 5*digitW + 3*gap + 2*colonW + 2*gap);

  if (morphStep < MORPH_STEPS) morphStep++;
}


// =========================
// Setup / Loop
// =========================
void setup() {
  Serial.begin(115200);
  delay(250);

  DBGLN("");
  DBGLN("========================================");
  DBGLN(" CYD LED Matrix Retro Clock - DEBUG BOOT");
  DBGLN("========================================");

  DBG("Build: %s %s\n", __DATE__, __TIME__);
  DBG("LED grid: %dx%d (fb size: %u bytes)\n", LED_MATRIX_W, LED_MATRIX_H, (unsigned)sizeof(fb));
  DBG("TFT_eSPI version check...\n");

  initBitmaps();
  loadConfig();

  DBG_STEP("Mounting LittleFS...");
  if (!LittleFS.begin(true)) {
    DBG_ERR("LittleFS mount failed");
  } else {
    DBG_OK("LittleFS mounted");
  }

  // TFT init
  DBG_STEP("Initialising TFT...");
  tft.init();
  tft.setRotation(1); // landscape wide along top/long edge
  tft.fillScreen(TFT_BLACK);
  setBacklight(cfg.brightness);
  DBG("TFT size (w x h): %d x %d\n", tft.width(), tft.height());
  DBG_OK("TFT ready.");

  // Create SMALL sprite for framebuffer rendering (avoid RAM issues)
  DBG_STEP("Creating framebuffer sprite (small)...");
  updateRenderPitch(true);
  int sprW = LED_MATRIX_W * fbPitch;
  int sprH = LED_MATRIX_H * fbPitch;

  if (useSprite) {
    int matrixAreaH = tft.height() - STATUS_BAR_H;
    if (matrixAreaH < sprH) matrixAreaH = tft.height();
    int x0 = (tft.width()-sprW)/2;
    int y0 = (matrixAreaH - sprH)/2;
    tft.fillScreen(TFT_BLACK);
    spr.pushSprite(x0, y0);
    DBG("Sprite OK: %dx%d\n", sprW, sprH);
    DBG_OK("Sprite ready.");
  } else {
    DBG_WARN("Sprite create FAILED. Falling back to direct draw (may flicker).");
  }

  // WiFi
  startWifi();

  // NTP
  startNtp();

  // OTA
  startOta();

  // Web
  DBG_STEP("Starting WebServer + routes...");
  serveStaticFiles();
  server.on("/api/state", HTTP_GET, handleGetState);
  server.on("/api/config", HTTP_POST, handlePostConfig);
  server.on("/api/mirror", HTTP_GET, handleGetMirror);
  server.begin();
  DBG_OK("WebServer ready.");

  DBG("Ready. IP: %s\n", WiFi.isConnected() ? WiFi.localIP().toString().c_str() : "0.0.0.0");
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
