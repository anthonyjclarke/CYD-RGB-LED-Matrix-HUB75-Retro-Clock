# CYD LED Matrix Retro Clock

![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32-green.svg)
![License](https://img.shields.io/badge/license-MIT-yellow.svg)

A retro-style LED matrix clock for the ESP32-2432S028 (CYD - Cheap Yellow Display) that emulates a **64×32 LED matrix** on a 320×240 TFT display. Features large 7-segment digits with smooth morphing animations, WiFi connectivity, NTP time synchronization, and a web-based configuration interface.

![Clock Display](docs/images/clock-display.jpg)

## Features

### Display
- **64×32 Virtual LED Matrix** emulation on 320×240 TFT display
- **Large 7-segment digits** with 1-pixel spacing for improved readability
- **Smooth morphing animations** when digits change
- **Adjustable LED appearance**: diameter, gap, color, and brightness
- **Status bar** showing WiFi, IP address, and date
- **Landscape orientation** optimized for desktop/shelf display

### Connectivity
- **WiFiManager** for easy WiFi setup (AP mode fallback)
- **NTP time synchronization** with IANA timezone support
- **Web-based configuration** interface accessible from any browser
- **OTA firmware updates** for easy maintenance
- **Live display mirror** in web UI showing real-time framebuffer

### Configuration
- Timezone selection (IANA format, e.g., "Australia/Sydney")
- NTP server configuration
- 12/24 hour time format
- LED diameter (1-10 pixels)
- LED gap spacing (0-8 pixels)
- LED color (RGB color picker)
- Backlight brightness (0-255)

## Hardware Requirements

### ESP32-2432S028 (CYD - Cheap Yellow Display)
- **MCU**: ESP32-WROOM-32
- **Display**: 2.8" ILI9341 TFT (320×240 pixels)
- **Interface**: SPI
- **Backlight**: PWM controlled (GPIO 21)
- **Power**: USB-C 5V

### Pin Configuration
The project uses the standard CYD pin configuration (defined in `include/User_Setup.h`):

```cpp
TFT_MISO   12
TFT_MOSI   13
TFT_SCLK   14
TFT_CS     15
TFT_DC      2
TFT_RST    -1  // Connected to ESP32 RST
TFT_BL     21  // Backlight control
```

### Purchase Links
- [ESP32-2432S028 on AliExpress](https://www.aliexpress.com/wholesale?SearchText=ESP32-2432S028)
- [ESP32-2432S028 on Amazon](https://www.amazon.com/s?k=ESP32-2432S028)

## Software Setup

### Prerequisites
- [Visual Studio Code](https://code.visualstudio.com/)
- [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode)
- USB cable (USB-C for CYD)

### Installation Steps

#### 1. Clone or Download the Project
```bash
git clone <repository-url>
cd CYD_LED_Matrix_Retro_Clock
```

#### 2. Open in VS Code with PlatformIO
```bash
code .
```

#### 3. Build the Project
- Click the PlatformIO icon in the sidebar
- Select "Build" or press `Ctrl+Alt+B` (Windows/Linux) / `Cmd+Shift+B` (Mac)

#### 4. Upload Firmware
- Connect your CYD via USB
- Select "Upload" from PlatformIO menu or press `Ctrl+Alt+U`
- Wait for upload to complete (~30 seconds)

#### 5. Upload Filesystem (Web UI)
- Select "Upload Filesystem Image" from PlatformIO menu
- This uploads the web interface files to LittleFS
- Wait for upload to complete (~10 seconds)

#### 6. Configure WiFi
On first boot, the device will create a WiFi access point:

1. Look for WiFi network: **CYD-RetroClock-Setup**
2. Connect to this network (no password required)
3. A captive portal should open automatically
4. If not, navigate to `http://192.168.4.1`
5. Click "Configure WiFi"
6. Select your WiFi network and enter the password
7. Click "Save"
8. The device will restart and connect to your WiFi

#### 7. Access the Web Interface
1. Check your router for the device's IP address, or
2. Look at the TFT display's status bar (bottom) for the IP
3. Open a web browser and navigate to: `http://<device-ip>`
4. You should see the configuration interface with a live display mirror

## Configuration

### Web Interface
Access the web interface at `http://<device-ip>` to configure:

- **Timezone**: IANA timezone string (e.g., "America/New_York", "Europe/London", "Australia/Sydney")
- **NTP Server**: Default is "pool.ntp.org", can use regional servers like "us.pool.ntp.org"
- **Time Format**: Choose between 12-hour or 24-hour display
- **LED Diameter**: Adjust the size of individual LED dots (1-10 pixels)
- **LED Gap**: Space between LEDs (0-8 pixels)
- **LED Color**: Use the color picker to choose any RGB color
- **Brightness**: Adjust backlight brightness (0-255)

### Configuration File
Settings are stored in `include/config.h`. Default values:

```cpp
#define DEFAULT_TZ "Australia/Sydney"
#define DEFAULT_NTP "pool.ntp.org"
#define DEFAULT_24H true
#define DEFAULT_LED_DIAMETER 5
#define DEFAULT_LED_GAP 0
#define DEFAULT_LED_COLOR_565 0xF800  // Red
#define STATUS_BAR_H 50  // pixels
```

### Security Configuration
**IMPORTANT**: Change the OTA password before deploying:

Edit `include/config.h`:
```cpp
#define OTA_PASSWORD "your-secure-password-here"
```

## Usage

### Display Layout
```
┌─────────────────────────────────────┐
│                                     │
│        16:30:45                     │  ← Clock digits (64×32 LED matrix)
│                                     │
│                                     │
├─────────────────────────────────────┤
│ WIFI: YourNetwork  IP: 192.168.1.x │  ← Status bar
│ 2026-01-07  LED: d5 g0 p5          │
└─────────────────────────────────────┘
```

### LED Matrix Emulation
- Each "LED" is rendered as a square pixel of adjustable size
- Pitch (spacing) is automatically calculated: `min(320/64, 190/32) = 5 pixels per LED`
- This gives a display size of 320×160 pixels for the clock
- Status bar occupies the bottom 50 pixels

### API Endpoints
The device provides a simple REST API:

- `GET /` - Main web interface
- `GET /api/state` - System state (JSON)
  ```json
  {
    "time": "16:30:45",
    "date": "2026-01-07",
    "wifi": "YourNetwork",
    "ip": "192.168.1.100",
    "tz": "Australia/Sydney",
    "ntp": "pool.ntp.org",
    "use24h": true,
    "ledDiameter": 5,
    "ledGap": 0,
    "ledColor": 16711680,
    "brightness": 255
  }
  ```
- `POST /api/config` - Update configuration (JSON body)
- `GET /api/mirror` - Raw framebuffer data (2048 bytes, 64×32 matrix)

## OTA Updates

### Using Arduino OTA
Once the device is connected to WiFi, you can update firmware wirelessly:

1. In PlatformIO, ensure the device is on the same network
2. The device should appear as "CYD-RetroClock" in the upload targets
3. Select it and upload as normal
4. Default password: "change-me" (change this in `config.h`!)

### Using Web OTA
The ArduinoOTA service runs on port 3232. You can use the Arduino IDE's network port feature or write a custom web OTA interface.

## Troubleshooting

### Display Issues

**Problem**: Display is blank or shows garbage
- **Solution**: Check TFT pin configuration in `include/User_Setup.h`
- Verify you have the correct CYD variant (ESP32-2432S028)
- Some variants use different display controllers (ST7789 vs ILI9341)

**Problem**: Display is upside down
- **Solution**: TFT is configured for landscape mode. Adjust `tft.setRotation()` in `main.cpp` if needed

**Problem**: Colors are wrong
- **Solution**: Verify RGB to RGB565 conversion in `rgb888_to_565()` function

### WiFi Issues

**Problem**: Can't connect to WiFi AP
- **Solution**:
  - Restart the device
  - The AP appears on first boot or if WiFi credentials are invalid
  - AP SSID: "CYD-RetroClock-Setup"
  - Wait 30 seconds after boot for AP to start

**Problem**: Device won't connect to my WiFi
- **Solution**:
  - Verify password is correct
  - Check that your network is 2.4GHz (ESP32 doesn't support 5GHz)
  - Try using WiFiManager to clear saved credentials and reconfigure

### Time Issues

**Problem**: Time is wrong
- **Solution**:
  - Check timezone setting (must be IANA format)
  - Verify internet connectivity (required for NTP)
  - Check NTP server is accessible
  - Wait a few minutes for NTP sync

**Problem**: Time doesn't update
- **Solution**:
  - Check Serial monitor (115200 baud) for NTP sync messages
  - Verify firewall isn't blocking NTP (UDP port 123)

### Web Interface Issues

**Problem**: Can't access web interface
- **Solution**:
  - Verify device IP address from serial monitor or TFT display
  - Check that your computer is on the same network
  - Try `http://<ip>` and `http://<ip>/index.html`
  - Clear browser cache

**Problem**: Display mirror not updating
- **Solution**:
  - Check browser console for errors (F12)
  - Verify `/api/mirror` endpoint is accessible
  - Refresh the page

## Development

### Project Structure
```
CYD_LED_Matrix_Retro_Clock/
├── data/                      # Web UI files (uploaded to LittleFS)
│   ├── index.html            # Main web interface
│   ├── app.js                # JavaScript for live updates & display mirror
│   └── style.css             # Stylesheet
├── include/
│   ├── config.h              # Configuration constants
│   └── User_Setup.h          # TFT_eSPI pin configuration
├── src/
│   └── main.cpp              # Main application code
├── platformio.ini            # PlatformIO configuration
├── CHANGELOG.md              # Version history
└── README.md                 # This file
```

### Key Functions

#### Framebuffer Management
- `fbClear()` - Clear the entire framebuffer
- `fbSet(x, y, v)` - Set a single pixel with intensity (0-255)

#### Digit Rendering
- `makeDigit7Seg(d)` - Generate 7-segment digit bitmap
- `drawBitmapSolid()` - Draw a bitmap to framebuffer
- `drawSpawnMorphToTarget()` - Animated morph effect for digit changes

#### Display Rendering
- `drawFrame()` - Main frame rendering (clock digits)
- `renderFBToTFT()` - Convert framebuffer to TFT display with LED emulation
- `computeRenderPitch()` - Calculate LED spacing based on display size

#### Configuration
- `loadConfig()` - Load settings from Preferences
- `saveConfig()` - Save settings to Preferences
- `handlePostConfig()` - Process configuration updates from web API

### Building Custom Variants

#### Change LED Matrix Size
Edit `platformio.ini`:
```ini
build_flags =
  -DLED_MATRIX_W=128   # Change width
  -DLED_MATRIX_H=64    # Change height
```

Note: Web interface constants must match (`data/app.js`):
```javascript
const LED_W = 128;
const LED_H = 64;
```

#### Add Different Display Controller
Edit `include/User_Setup.h`:
```cpp
// Change from ILI9341 to ST7789
#define ST7789_DRIVER
```

## Future Enhancements

See `CHANGELOG.md` for planned features:
- Temperature/humidity sensor support
- Additional display modes (date, temperature, messages)
- Color schemes and themes
- Alarm functionality
- Touch screen support
- MQTT integration

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Built using [PlatformIO](https://platformio.org/)
- TFT display library: [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)
- WiFi management: [WiFiManager](https://github.com/tzapu/WiFiManager)
- JSON parsing: [ArduinoJson](https://arduinojson.org/)
- Inspired by classic LED matrix clocks and 7-segment displays
- Built with assistance from [Claude Code](https://claude.com/claude-code)

## Support

For issues, questions, or suggestions:
- Open an issue on GitHub
- Check the troubleshooting section above
- Review the CHANGELOG.md for known issues

## Version History

See [CHANGELOG.md](CHANGELOG.md) for detailed version history.

**Current Version**: 1.0.0 (2026-01-07)

---

**Enjoy your retro LED matrix clock!** ⏰
