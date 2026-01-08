# Changelog

All notable changes to the CYD LED Matrix Retro Clock project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-01-07

### Added
- Initial release of CYD RGB LED Matrix (HUB75) Retro Clock
- 64×32 virtual RGB LED Matrix (HUB75) emulation on 320×240 TFT display
- Large 7-segment style clock digits with spacing for improved readability
- Morphing animations for smooth digit transitions
- WiFi configuration via WiFiManager with AP mode fallback
- NTP time synchronization with IANA timezone support (88 timezones across 13 regions)
- Web-based configuration interface at device IP address
- Live display mirror in web interface showing real-time framebuffer
- Adjustable LED appearance settings:
  - LED diameter (1-10 pixels)
  - LED gap spacing (0-8 pixels)
  - LED color (RGB color picker with instant preview)
  - Backlight brightness (0-255)
- Status bar on TFT display showing:
  - WiFi network name and IP address
  - Current date in selected format
  - Timezone name
- Comprehensive system diagnostics panel in web interface:
  - Time & Network section (current time, date, WiFi status, IP address)
  - Hardware section (board type, display model, sensor status, firmware version, OTA status)
  - System Resources section (uptime, free heap, heap usage percentage, CPU frequency)
  - Debug Settings section (runtime-adjustable debug level with 5 levels)
- Enhanced serial logging with before/after change tracking for all configuration changes
- Timezone dropdown organized by 13 geographic regions with 88 timezones
- NTP server dropdown selector with 9 preset servers:
  - Global Pool (pool.ntp.org)
  - Google (time.google.com)
  - Cloudflare (time.cloudflare.com)
  - Apple (time.apple.com)
  - Microsoft (time.windows.com)
  - Regional pools (Australia, USA, Europe, Asia)
- Date format selection with 5 formats:
  - YYYY-MM-DD (ISO 8601)
  - DD/MM/YYYY (European)
  - MM/DD/YYYY (US)
  - DD.MM.YYYY (German)
  - Mon DD, YYYY (Verbose)
- Debug level selector with runtime control (Off, Error, Warning, Info, Verbose)
- Firmware version display in diagnostic panel (single source from config.h)
- Contact footer with GitHub repository and Bluesky profile links
- Instant auto-apply for all configuration changes (no save button required)
- Display flip/rotation toggle button for 180° orientation (allows mounting in either direction)
- OTA (Over-The-Air) firmware updates via ArduinoOTA
- LittleFS filesystem for serving web UI files
- Persistent configuration storage using Preferences (including debug level)
- Human-readable formatting for uptime (days/hours/minutes) and memory (KB/MB)

### Display Layout
- Top section: Large RGB LED Matrix (HUB75) clock (HH:MM:SS format)
- Bottom section: 50-pixel status bar with system information
- Total display area: 320×240 pixels in landscape orientation
- Emulates physical 64×32 RGB LED Matrix Panel (HUB75 protocol)

### Hardware Support
- ESP32-2432S028 (CYD - Cheap Yellow Display)
- ILI9341 TFT display controller
- Built-in backlight control (GPIO 21)
- SPI interface for display communication

### Web API
- `GET /` - Main web interface (index.html)
- `GET /api/state` - System state JSON (time, date, WiFi, config, diagnostics)
  - Includes: uptime, freeHeap, heapSize, cpuFreq, debugLevel, firmware version
  - Hardware info: board type, display model, sensor status, OTA status
- `GET /api/timezones` - List of 88 timezones grouped by 13 geographic regions
- `POST /api/config` - Update configuration settings with detailed logging
  - Logs before/after values for all changed fields
  - Includes client IP address in logs
- `GET /api/mirror` - Raw framebuffer data (2048 bytes for 64×32 matrix)

### Configuration
- Default timezone: Australia/Sydney
- Default NTP server: pool.ntp.org
- Default LED diameter: 5 pixels
- Default LED gap: 0 pixels (no gap)
- Default LED color: Red (0xFF0000)
- Default brightness: 255 (maximum)
- Status bar height: 50 pixels
- Frame rate: ~30 FPS (33ms per frame)
- Morph animation: 20 steps

### Technical Details
- RGB LED Matrix (HUB75) pitch calculation: min(320/64, 190/32) = 5 pixels per LED
- Framebuffer: 8-bit intensity values (0-255) per pixel
- Sprite-based rendering for flicker-free display updates
- Row-major framebuffer layout: fb[y][x]
- Digit width: 9 pixels (reduced to fit HH:MM:SS with gaps)
- Digit spacing: 1 pixel gap between digits
- Total clock width: 63 pixels (fits centered in 64-pixel width)

### Fixed
- Color picker now properly allows color selection without value being overwritten by auto-refresh
- Timezone dropdown properly displays all 88 timezones organized by geographic region
- Web interface automatically applies changes without requiring manual save button click
- Serial logging now consistently uses [INFO] level for all configuration changes
- All field type checks in API now use consistent `.isNull()` pattern for reliability

### Known Issues
- None at initial release

### Security Notes
- Default OTA password is "change-me" - **CHANGE THIS BEFORE DEPLOYMENT**
- WiFi credentials stored in ESP32 NVS (Non-Volatile Storage)
- No authentication on web interface - suitable for trusted networks only

## [Unreleased]

### Planned Features
- Temperature and humidity sensor support (SHT30/BME280/HTU21D)
- Additional display modes (date, temperature, custom messages)
- Multiple color schemes and themes
- Alarm functionality
- Touch screen support for direct configuration
- MQTT integration for remote control
- Automatic brightness adjustment based on ambient light

---

**Note**: Version 1.0.0 represents the first stable release with core clock functionality,
web configuration, and RGB LED Matrix (HUB75) emulation working correctly.
