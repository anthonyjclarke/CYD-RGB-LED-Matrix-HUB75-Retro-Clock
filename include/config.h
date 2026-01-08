#pragma once

// -----------------------------
// Project: CYD LED Matrix Retro Clock
// Starting scope: WiFiManager + NTP + WebUI + TFT "LED Matrix" emulation + simple morphing
// -----------------------------

// ===== FIRMWARE VERSION =====
#define FIRMWARE_VERSION "1.0.0"

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

// ===== SENSOR (future) =====
// We'll add SHT30/BME280/HTU21D later (I2C).
#define DEFAULT_TEMP_C true

// ===== OTA =====
#define OTA_HOSTNAME "CYD-RetroClock"
#define OTA_PASSWORD "change-me"   // Change this before flashing for real use.

// ===== WEB =====
#define HTTP_PORT 80

// ===== RENDER =====
#define FRAME_MS 33   // ~12 FPS
#define MORPH_STEPS 20  // number of frames for morphing transitions
