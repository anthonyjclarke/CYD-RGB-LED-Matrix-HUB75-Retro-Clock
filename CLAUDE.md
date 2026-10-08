# CYD RGB LED Matrix (HUB75) Retro Clock – Claude context

## Target

ESP32-2432S028R (CYD 2.8″, ILI9341 320×240, no PSRAM), single env
`cyd_esp32_2432s028`. Emulates a 64×32 HUB75 panel on the TFT: a logical
framebuffer `fb[32][64]` of 8-bit intensities is drawn as round "LEDs" into a
320×160 sprite, with morphing 7-segment HH:MM:SS digits and a 2-line status
bar. WiFiManager + NTP, optional I2C sensor, web UI with live mirror.
Version: `FIRMWARE_VERSION` in `include/config.h` (1.3.0-dev on `dev`).

## Non-standard choices

- **TFT_eSPI is configured by `include/User_Setup.h`** (force-included via
  `-include` in `build_flags`), not by `-D` pin flags. SPI runs at 40 MHz.
- **ArduinoJson v7** (`JsonDocument`), not v6 – don't use v6 APIs.
- **Time is `configTzTime()` + POSIX TZ strings** from `include/timezones.h`,
  not ezTime. `handleGetTimezones()` hard-codes region index ranges into that
  array – adding/removing a timezone means updating those ranges.
- Platform pinned to `espressif32@6.12.0`. Unpinned resolves to an Arduino 3.x
  core and `ledcSetup`/`ledcAttachPin` no longer exist.
- Debug macros live in `include/debug.h` with a runtime `debugLevel` (NVS).
  They do **not** append a newline – every call passes its own `\n`.

## Pins (beyond the standard CYD set)

| Signal      | GPIO    | Notes                              |
|:------------|:--------|:-----------------------------------|
| Sensor SDA  | 27      | CN1; BME280 / SHT3X / HTU21D       |
| Sensor SCL  | 22      | CN1                                |
| BOOT button | 0       | Hold 3 s at power-up → WiFi reset  |
| RGB LED     | 4/16/17 | R/G/B, active LOW, status codes    |

Sensor type is a compile-time choice: exactly one `USE_*` in `config.h`.

## Persistence

NVS namespace `retroclock`: `tz`, `ntp`, `24h`, `dfmt`, `ledd`, `ledg`, `col`,
`bl`, `flip`, `useFahr`, `dbglvl`. Nothing is stored in LittleFS – the data
partition is unused.

## Web UI

Edit `data/` only. `tools/embed_web.py` (a `pre:` script) regenerates the
gitignored `src/web_assets.h`, served from PROGMEM with `send_P()`. Never
reintroduce LittleFS for the UI: the installer flashes four parts only.

## Web installer and releases

- Release images come only from CI on a `v*` tag on `main`; never publish a
  local build or `_site/`.
- Never put `firmware-merged.bin` in a manifest – it wipes NVS.
- `PROJECT_NAME` and `partitions_custom.csv` are frozen; changing either breaks
  Update for existing boards.
- Never put Improv back in `lib_deps` – `lib/ImprovWiFi` is the patched copy.
- `improvTick()` must run at least every ~1 s (loop and WiFiManager portal);
  nothing in `loop()` may block longer.

## Gotchas

- `OTA_PASSWORD` defaults to `"change-me"`; ArduinoOTA can leave the board on
  `app1` (boot log prints `Running from …`).
- `FRAME_MS` is 33 ms (~30 FPS); the "~12 FPS" comment in `config.h` is stale.
- The fallback AP after a portal timeout is `CYD-RetroClock-AP`, distinct from
  the setup AP `AP_NAME`.
