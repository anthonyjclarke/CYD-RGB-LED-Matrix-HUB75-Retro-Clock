# Project: CYD RGB LED Matrix HUB75 Retro Clock 16-1-2025

## Overview

A retro-style RGB LED Matrix (HUB75) clock emulator for the ESP32-2432S028 (CYD) that renders a virtual 64×32 LED matrix panel on a 320×240 TFT display. Features large 7-segment digits with smooth morphing animations, WiFi connectivity with captive portal setup, NTP time synchronization across 88 global timezones, optional I2C sensor support (BME280/SHT3X/HTU21D), and a web-based configuration interface with live display mirroring. All settings persist to NVS with instant auto-apply and comprehensive system diagnostics.

## Hardware

- MCU: ESP32-2432S028R (CYD - Cheap Yellow Display)
- Display: 2.8" ILI9341 TFT (320×240 pixels, SPI interface)
- Built-in: RGB LED (active LOW), XPT2046 touch (unused), SD card slot
- Optional Sensors: BME280/SHT3X/HTU21D via I2C on CN1 connector (GPIO 27/22)
- Power: USB-C

## Build Environment

- Framework: Arduino
- Platform: espressif32 (esp32dev board)
- Key Libraries:
  - TFT_eSPI @ ^2.5.43 (display driver with custom User_Setup.h)
  - WiFiManager @ ^2.0.16-rc.2 (captive portal WiFi setup)
  - ArduinoJson @ ^7.0.4 (web API JSON parsing)
  - Adafruit sensor libraries (BME280, SHT31, HTU21DF)
- Filesystem: LittleFS (web UI files in data/ directory)
- OTA: ArduinoOTA enabled (hostname: CYD-RetroClock, default password: "change-me")

## Project Structure

```
CYD_RGB_LED_Matrix_HUB75_Retro_Clock/
├── src/
│   └── main.cpp              # Main application (1850 lines)
│                            # - Framebuffer logic, 7-segment rendering
│                            # - Morphing animations, web server
│                            # - NTP sync, sensor integration
├── include/
│   ├── config.h             # Hardware pins, defaults, FIRMWARE_VERSION
│   ├── timezones.h          # 88 POSIX timezone strings (13 regions)
│   └── User_Setup.h         # TFT_eSPI pin configuration for CYD
├── data/                    # Web UI served via LittleFS
│   ├── index.html          # Configuration interface
│   ├── app.js              # Live updates, display mirror canvas
│   └── style.css           # Styling with diagnostics panel
├── platformio.ini           # Build config, LED_MATRIX_W/H defines
├── README.md                # Comprehensive user documentation
├── CHANGELOG.md             # Version history (currently v1.2.0 WIP)
└── CLAUDE.md                # This file
```

## Pin Mapping

### Display (SPI)

| Function | GPIO | Notes |
|----------|------|-------|
| TFT_MISO | 12 | SPI read (minimal use) |
| TFT_MOSI | 13 | SPI data out |
| TFT_SCLK | 14 | SPI clock @ 40MHz |
| TFT_CS | 15 | Chip select (active LOW) |
| TFT_DC | 2 | Data/Command signal |
| TFT_RST | -1 | Connected to ESP32 RST |
| TFT_BL | 21 | Backlight PWM (0-255) |

### Status Indicators

| Function | GPIO | Notes |
|----------|------|-------|
| LED_RED | 4 | Active LOW, status indicator |
| LED_GREEN | 16 | Active LOW, status indicator |
| LED_BLUE | 17 | Active LOW, status indicator |

**RGB LED Status Codes:**
- Blue = Connecting to WiFi
- Green flash = Success (WiFi connected, NTP configured, sensor detected)
- Yellow = BOOT button pressed or no sensor detected
- Red = WiFi failed or reset confirmed
- Purple = Config portal active (AP mode)
- Cyan = OTA update in progress

### User Input

| Function | GPIO | Notes |
|----------|------|-------|
| BOOT_BTN | 0 | Built-in button, hold 3s during power-up to reset WiFi |

### Optional Sensors (I2C on CN1 Connector)

| Function | GPIO | Notes |
|----------|------|-------|
| SENSOR_SDA | 27 | I2C data (BME280/SHT3X/HTU21D) |
| SENSOR_SCL | 22 | I2C clock |

**Sensor I2C Addresses:**
- BME280: 0x76 or 0x77
- SHT3X: 0x44 or 0x45
- HTU21D: 0x40 (fixed address)

## Configuration

### Config File Locations

- **Hardware/Defaults:** `include/config.h`
  - `FIRMWARE_VERSION` (currently "1.2.0")
  - Pin definitions, LED matrix size (64×32)
  - Default timezone, NTP server, LED appearance
  - Sensor type selection (uncomment ONE: USE_BME280 / USE_SHT3X / USE_HTU21D)
- **TFT Display:** `include/User_Setup.h`
  - ILI9341 driver selection, pin mappings, SPI frequencies
- **Runtime Settings:** Stored in ESP32 NVS via `Preferences` API
  - Persists: timezone, NTP server, time/date format, LED color/size/gap, brightness, display flip, temperature unit, debug level

### Key Configurable Settings (via Web UI)

- **Time:** Timezone (88 options), NTP server (9 presets), 12/24h format
- **Date:** 5 formats (ISO, European, US, German, Verbose)
- **LED Appearance:** Diameter (1-10px), gap (0-8px), color (RGB picker), brightness (0-255)
- **Display:** Flip/rotation toggle (normal vs 180° flip)
- **Temperature:** °C or °F
- **Debug:** 5 levels (Off, Error, Warning, Info, Verbose) - runtime adjustable

### Web API Endpoints

```
GET  /                   # Main web interface (index.html)
GET  /api/state          # System state JSON (time, config, diagnostics)
GET  /api/timezones      # List of 88 timezones grouped by 13 regions
POST /api/config         # Update configuration (logs changes to Serial)
POST /api/reset-wifi     # Reset WiFi credentials and restart in AP mode
GET  /api/mirror         # Raw framebuffer (2048 bytes, 64×32 matrix @ 8-bit intensity)
```

**Note:** Web UI display mirror replicates physical TFT layout including status bar showing temp/humidity and date/timezone (added in v1.1.0).

### Important Build Flags (platformio.ini)

```ini
-DLED_MATRIX_W=64        # Virtual LED matrix width
-DLED_MATRIX_H=32        # Virtual LED matrix height
-DCORE_DEBUG_LEVEL=0     # Minimal ESP32 core debug output
```

## Current State

### Version: 1.2.0 (Work In Progress)

- **Last Stable Release:** v1.1.0 (2026-01-08)
- **Branch:** dev (main is production)

### Implemented Features

✅ Core clock with HH:MM:SS display in 7-segment style
✅ Smooth morphing animations (20-step spawn/particle morph)
✅ WiFiManager captive portal with BOOT button reset (3s hold)
✅ NTP sync with 88 global timezones (POSIX strings with auto-DST)
✅ Web configuration interface with instant auto-apply
✅ Live display mirror (64×32 framebuffer streaming)
✅ Sprite-based flicker-free rendering (320×160 sprite)
✅ Runtime-adjustable debug levels (0-4 with Serial logging)
✅ Persistent configuration (NVS storage)
✅ OTA firmware updates via ArduinoOTA
✅ I2C sensor support (BME280/SHT3X/HTU21D with auto-detection)
✅ Status bar with 2 lines (Line 1: Temp/Humidity or "Sensor: Not detected", Line 2: Date and Timezone)
✅ RGB LED visual status indicators
✅ Display flip/rotation for flexible mounting
✅ Comprehensive system diagnostics in web UI

### Current Development Focus (v1.2.0 WIP)

- **OTA Progress Visualization**: Visual progress bar on TFT during firmware updates
  - Color-coded progress: Red (0-33%), Yellow (33-66%), Green (66-100%)
  - Real-time percentage display and status messages
  - Comprehensive error handling with detailed messages
- Sensor data integration complete (temperature, humidity, pressure for BME280)
- Status bar displays sensor readings in real-time (Line 1: Temp/Humidity, Line 2: Date/Timezone)
- Temperature unit conversion (°C/°F) working
- All v1.1.0 features stable and tested

## Architecture Notes

### Rendering Pipeline

1. **Logical Framebuffer:** `uint8_t fb[32][64]` stores 8-bit intensity per pixel (0-255)
2. **7-Segment Generation:** Bitmask-based digit rendering with 1px gaps between digits
3. **Morphing System:** Spawn morph (particles from center) + particle morph (nearest-neighbor matching)
4. **TFT Rendering:** Sprite-based (TFT_eSprite) for flicker-free updates
   - Pitch calculation: `min(320/64, 190/32) = 5 pixels per LED`
   - LED appearance: configurable diameter and gap within pitch constraint
   - Color scaling: Base RGB × intensity (0-255) → RGB565 conversion

### Time Management

- NTP client with POSIX TZ strings (automatic DST handling)
- Safe time retrieval with timeout protection (2000ms default)
- Second-change detection triggers morph animation
- No RTC backup - requires WiFi/NTP for accurate time

### Memory Management

- Sprite size: 320×160 pixels (102,400 bytes) at 16-bit color depth
- Framebuffer: 2048 bytes (64×32 @ 8-bit intensity)
- Morph buffers: Static arrays (420 points max) to avoid heap fragmentation
- LittleFS: Minimal RAM usage for web file serving

### Debug System

```cpp
// 5-level debug system with runtime control
DBG_ERROR(...)   // Level 1: Critical errors only
DBG_WARN(...)    // Level 2: Warnings + errors
DBG_INFO(...)    // Level 3: General info (default)
DBG_VERBOSE(...) // Level 4: All debug including frequent events
// Level 0: Off (no output)
```
- Controlled via `debugLevel` variable (adjustable via web UI or serial)
- All config changes logged with before/after values and client IP
- Sensor readings always logged at INFO level for visibility

### Non-Blocking Design

- `millis()` timing for frame rate (33ms nominal, comment says ~12 FPS but achieves ~30 FPS)
- NTP sync with timeout protection
- Sensor updates every 60 seconds (non-blocking)
- Web server request handling in main loop
- No `delay()` calls except during startup/OTA

## Known Issues

### Hardware Limitations

- **No RTC backup:** Device requires WiFi/NTP connection for time. If WiFi drops, time continues but may drift without periodic NTP sync.
- **Touch screen unused:** XPT2046 resistive touch hardware present but not implemented in software.
- **Single color mode:** All LEDs currently share same RGB color. No per-pixel color effects yet.
- **Memory constraints:** ESP32 heap limits sprite size. Current 320×160 sprite works but larger displays would need optimization.

### Software Limitations

- **No time history:** Status bar shows current sensor readings only, no graphing or data logging.
- **OTA requires tool:** No web-based firmware upload interface (planned for future). Must use ArduinoOTA or PlatformIO upload. Visual progress bar shows upload status on TFT.
- **No error recovery:** If LittleFS mount fails, web UI is unavailable. No fallback UI.
- **Fixed layout:** Clock always HH:MM:SS format. No alternate display modes (date-only, stopwatch, etc.).
- **Status bar fixed content:** Always shows temp/humidity (if sensor present) or "Sensor: Not detected". No WiFi SSID or IP shown on TFT (only in web UI).

### Security Considerations

- **Default OTA password:** Ships with "change-me" - must be changed in `include/config.h` before deployment.
- **No authentication:** Web interface has no login/password protection.
- **No HTTPS:** Web server runs plain HTTP on port 80.

## TODO

### High Priority

- [ ] **SECURITY:** Change default OTA password before any production deployment (currently "change-me" in config.h)
- [ ] Test sensor failover (what happens if sensor dies mid-operation?)
- [ ] Verify memory stability over 24+ hour runtime (heap fragmentation check)
- [ ] Document actual vs. nominal frame rate (config says ~12 FPS but achieves ~30 FPS)

### Medium Priority (v1.2.0+)

- [ ] Finalize OTA progress visualization (currently in v1.2.0 WIP)
- [ ] Add option to show WiFi SSID/IP on status bar (currently only temp/humidity)
- [ ] Status bar content configurability (choose what to display)

### Future Enhancements (from main.cpp:53-63)

- [ ] Multiple display modes (date-only, temperature display, custom messages)
- [ ] Per-LED color control for RGB matrix effects (rainbow, gradient, animations)
- [ ] Touch screen support for direct configuration (tap to cycle modes, swipe for brightness)
- [ ] Color themes and presets (fire, ice, matrix green, retro amber)
- [ ] MQTT integration for remote control and monitoring
- [ ] Mobile-friendly web interface enhancements (responsive design)
- [ ] Home Assistant integration (auto-discovery, entity configuration)
- [ ] Web-based OTA upload interface (drag-drop .bin file) - OTA progress display exists, needs web upload form
- [ ] Customizable animations and transition effects (wipe, fade, explode)
- [ ] Touch panel diagnostic overlay (show sensor readings, WiFi signal, heap usage on tap)
- [ ] Data logging to SD card (temperature/humidity history with timestamps)
- [ ] Alarm/timer functionality with buzzer support
- [ ] Multiple timezone display (world clock mode)
- [ ] Weather API integration (show forecast icons)
- [ ] Scrolling text message display mode

### Nice to Have

- [ ] Battery backup support (ESP32 deep sleep on power loss, RTC module)
- [ ] Automatic brightness based on ambient light sensor
- [ ] Sound effects for animations (I2S DAC output)
- [ ] Export configuration as JSON file (backup/restore)
- [ ] Multiple named profiles (home, office, travel configs)

### Documentation

- [ ] Add wiring diagram for sensor connection to CN1
- [ ] Create video tutorial for first-time setup
- [ ] Document common troubleshooting steps (WiFi issues, sensor detection failures)
- [ ] Add schematic showing all GPIO usage and conflicts

---

## Development Notes

### Building and Flashing

```bash
# Initial build and upload
pio run -t upload

# Upload filesystem (web UI files)
pio run -t uploadfs

# OTA update (after initial flash)
# 1. Uncomment OTA settings in platformio.ini
# 2. Set device IP address
# 3. Match password to OTA_PASSWORD in config.h
pio run -t upload
```

### Debugging

- Serial monitor @ 115200 baud
- Set debug level in web UI or via `debugLevel` variable
- Use `DBG_VERBOSE()` for frequent events, `DBG_INFO()` for general logging
- Check heap usage: `ESP.getFreeHeap()` and `ESP.getHeapSize()`

### Testing Checklist

- [ ] WiFi connect (check LED status codes: Blue→Green flash)
- [ ] Web UI accessible at device IP
- [ ] Configuration changes persist after reboot
- [ ] Sensor readings update (if sensor connected) - verify status bar shows temp/humidity
- [ ] Sensor disconnected - verify status bar shows "Sensor: Not detected"
- [ ] Display flip/rotation toggle works
- [ ] OTA update succeeds with progress bar display (Red→Yellow→Green)
- [ ] OTA error handling (test with wrong password - should show error message)
- [ ] Morph animations smooth (no flicker)
- [ ] Time updates every second
- [ ] Status bar updates correctly:
  - Line 1: Temperature and Humidity (or "Sensor: Not detected")
  - Line 2: Date (in selected format) and Timezone name
- [ ] BOOT button WiFi reset (3-second hold: Yellow→Red LED, restart in AP mode)
- [ ] Web API WiFi reset endpoint works

### Code Style

- Explicit types over `auto` where practical
- Meaningful variable names (avoid single letters except loop counters)
- Comments for timing-critical sections and hardware-specific quirks
- `#define` for pin mappings, `constexpr` for computed constants
- Prefer non-blocking code patterns (`millis()` over `delay()`)
- Use `DBG_*()` macros for all debug output (never raw `Serial.print`)

---

## Version History Summary

### v1.2.0 (WIP - Current Development)

- OTA progress visualization with color-coded progress bar
- Error handling with detailed messages on TFT display
- RGB LED status indicators during OTA (Cyan = updating, Green = success, Red = error)

### v1.1.0 (2026-01-08)

- WiFi reset via BOOT button (3-second hold)
- Web interface WiFi reset endpoint (`/api/reset-wifi`)
- RGB LED status indicators for all operations
- Status bar now shows temp/humidity instead of WiFi info
- Display flip setting properly stored

### v1.0.0 (2026-01-07)

- Initial stable release
- Core clock functionality with HH:MM:SS display
- 64×32 RGB LED Matrix (HUB75) emulation
- Web configuration interface with live mirror
- 88 global timezones, 9 NTP servers, 5 date formats
- OTA updates, persistent configuration, debug levels

---

**Last Updated:** 2026-01-16
**Maintainer:** Anthony Clarke
**Repository:** https://github.com/anthonyjclarke/CYD-RGB-LED-Matrix-HUB75-Retro-Clock
**License:** MIT
