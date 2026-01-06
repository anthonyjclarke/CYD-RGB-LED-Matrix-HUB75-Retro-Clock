# CYD LED Matrix Retro Clock

A PlatformIO (Arduino) project targeting the ESP32-2432S028 “Cheap Yellow Display (CYD)” to emulate **two 64×32 RGB LED matrix panels** side-by-side (logical grid **128×32**) and render a retro **morphing clock**.

## What’s implemented (initial scaffold)
- WiFiManager captive portal setup (SSID: `CYD-RetroClock-Setup`)
- NTP time sync using IANA timezone string (default: `Australia/Sydney`)
- TFT “LED grid” renderer (128×32 framebuffer)
- Simple per-pixel morph between digit bitmaps (placeholder for HariFun morph technique)
- Minimal Web UI:
  - Current time/date + WiFi status
  - Config: timezone, NTP, 12/24h, LED size/gap/color, brightness
  - Display mirror: shows the 128×32 framebuffer in a canvas
- OTA via ArduinoOTA (password in `include/config.h`)

## Build & Upload
1. Open this folder in VS Code with PlatformIO.
2. Build/upload firmware:
   - `PlatformIO: Upload`
3. Upload Web UI to LittleFS:
   - `PlatformIO: Upload Filesystem Image`
4. First boot: connect to `CYD-RetroClock-Setup` AP and set WiFi.

## Configure TFT pins / driver
Edit `include/User_Setup.h` (TFT_eSPI setup). If you use a different CYD variant or driver (e.g., ST7789), change the driver define and pins.

## Next steps
- Replace placeholder morph with HariFun morphing clock technique.
- Add I2C sensors (SHT30/BME280/HTU21D) and show temp/humidity on display + Web UI.
- Add password-protected web OTA upload option.
- Add touch interactions on CYD (optional).
