# Changelog

All notable changes to the CYD LED Matrix Retro Clock project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-01-07

### Added
- Initial release of CYD LED Matrix Retro Clock
- 64×32 virtual LED matrix emulation on 320×240 TFT display
- Large 7-segment style clock digits with spacing for improved readability
- Morphing animations for smooth digit transitions
- WiFi configuration via WiFiManager with AP mode fallback
- NTP time synchronization with IANA timezone support
- Web-based configuration interface at device IP address
- Live display mirror in web interface showing real-time framebuffer
- Adjustable LED appearance settings:
  - LED diameter (1-10 pixels)
  - LED gap spacing (0-8 pixels)
  - LED color (RGB color picker)
  - Backlight brightness (0-255)
- Status bar displaying:
  - WiFi network name
  - IP address
  - Current date
  - LED rendering parameters
- OTA (Over-The-Air) firmware updates via ArduinoOTA
- LittleFS filesystem for serving web UI files
- Persistent configuration storage using Preferences
- 12/24 hour time format selection
- Configurable NTP server
- Debug logging via Serial (115200 baud)

### Display Layout
- Top section: Large LED matrix clock (HH:MM:SS format)
- Bottom section: 50-pixel status bar with system information
- Total display area: 320×240 pixels in landscape orientation

### Hardware Support
- ESP32-2432S028 (CYD - Cheap Yellow Display)
- ILI9341 TFT display controller
- Built-in backlight control (GPIO 21)
- SPI interface for display communication

### Web API
- `GET /` - Main web interface (index.html)
- `GET /api/state` - System state JSON (time, date, WiFi, config)
- `POST /api/config` - Update configuration settings
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
- LED matrix pitch calculation: min(320/64, 190/32) = 5 pixels per LED
- Framebuffer: 8-bit intensity values (0-255) per pixel
- Sprite-based rendering for flicker-free display updates
- Row-major framebuffer layout: fb[y][x]
- Digit width: 9 pixels (reduced to fit HH:MM:SS with gaps)
- Digit spacing: 1 pixel gap between digits
- Total clock width: 63 pixels (fits centered in 64-pixel width)

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
web configuration, and LED matrix emulation working correctly.
