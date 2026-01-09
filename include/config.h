#pragma once

// -----------------------------
// Project: CYD LED Matrix Retro Clock
// Starting scope: WiFiManager + NTP + WebUI + TFT "LED Matrix" emulation + simple morphing
// -----------------------------

// ===== FIRMWARE VERSION =====
#define FIRMWARE_VERSION "1.2.0"

// ===== LED MATRIX EMULATION =====
// Logical LED grid (single 64x32 panel)
#ifndef LED_MATRIX_W
#define LED_MATRIX_W 64
#endif

#ifndef LED_MATRIX_H
#define LED_MATRIX_H 32
#endif

// Default "LED" rendered size (in TFT pixels) and spacing between LEDs (pitch simulation)
// Adjust in Web UI later (stored in config).
#define DEFAULT_LED_DIAMETER 5     // pixels (max, fills the pitch completely)
#define DEFAULT_LED_GAP      0     // pixels (no gap for maximum fill)

// Reserve space below the matrix for status/info
#define STATUS_BAR_H 50            // pixels (bottom status bar)

// Default LED color (RGB565). Start with red.
#define DEFAULT_LED_COLOR_565 0xF800

// ===== TIME / NTP =====
#define DEFAULT_TZ "Sydney, Australia"    // Timezone name from timezones.h
#define DEFAULT_NTP "pool.ntp.org"
#define DEFAULT_24H true

// ===== SENSOR CONFIGURATION =====
// Choose your sensor type by uncommenting ONE of the following:
// #define USE_BME280        // BME280: Temperature, Humidity, Pressure sensor
// #define USE_SHT3X         // SHT3X: Temperature and Humidity sensor (no pressure)
#define USE_HTU21D           // HTU21D: Temperature and Humidity sensor (no pressure) - DEFAULT

// I2C pins for sensor (using extended GPIO connector CN1)
#define SENSOR_SDA_PIN    27    // I2C Data
#define SENSOR_SCL_PIN    22    // I2C Clock

// Sensor update interval
#define SENSOR_UPDATE_INTERVAL 60000  // Update sensor every 60 seconds

// Temperature unit
#define DEFAULT_TEMP_C true

// ===== RGB LED STATUS INDICATOR =====
// CYD board has built-in RGB LED (active LOW)
#define LED_R_PIN      4    // Red LED
#define LED_G_PIN     16    // Green LED
#define LED_B_PIN     17    // Blue LED

// ===== BOOT BUTTON =====
// Boot button for WiFi reset (active LOW)
#define BOOT_BTN_PIN   0    // Boot button (built-in on CYD)

// ===== OTA =====
#define OTA_HOSTNAME "CYD-RetroClock"
#define OTA_PASSWORD "change-me"   // Change this before flashing for real use.

// ===== WEB =====
#define HTTP_PORT 80

// ===== RENDER =====
#define FRAME_MS 33   // ~12 FPS
#define MORPH_STEPS 20  // number of frames for morphing transitions
