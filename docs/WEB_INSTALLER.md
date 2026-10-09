# Web installer

The browser installer at
[anthonyjclarke.github.io/CYD-RGB-LED-Matrix-HUB75-Retro-Clock](https://anthonyjclarke.github.io/CYD-RGB-LED-Matrix-HUB75-Retro-Clock/)
uses the shared [cyd-web-installer](https://github.com/anthonyjclarke/cyd-web-installer)
workflow. Release images are built only by CI on a `v*` tag on `main`.

---

## Project specifics

| Item              | Value                                   |
|:------------------|:----------------------------------------|
| `PROJECT_NAME`    | `CYD-RGB-LED-Matrix-HUB75-Retro-Clock`  |
| Envs in installer | `cyd_esp32_2432s028` (CYD 2.8″)         |
| Partition table   | `partitions_custom.csv` (frozen)        |
| Platform          | `espressif32@6.12.0`                    |
| Setup AP          | `CYD-RetroClock-Setup`                  |
| OTA               | ArduinoOTA – so the `app1` case applies |
| Filesystem        | None – web UI is in PROGMEM             |

v1.3.0 moved from `default.csv` to the standard dual-OTA table. NVS, `otadata`
and `app0` keep their offsets, so a v1.2.0 board can take **Install** without
erasing and keep its settings. LittleFS held only the web UI, which is now
embedded by `tools/embed_web.py`.

---

## Smoke test (RUNBOOK 5a)

| Item           | Result                                       |
|:---------------|:---------------------------------------------|
| Date           | 10-10-2026                                   |
| Board          | CYD 2.8″ ESP32-D0WD-V3 · `b0:cb:d8:da:ae:8c` |
| Image          | CI preview, run 37911340756 (`73960e0`)      |
| Browser        | macOS Chrome                                 |
| Fresh install  | Pass – erased, Install + erase               |
| Configure WiFi | Pass – Improv joined WiFi, no setup AP       |
| Boot log       | Pass – `Running from app0`, NTP, no crash    |
| Connect again  | Pass – "Connected to RetroClock-CBB0"        |
| Web UI         | Pass – all three files served from PROGMEM   |

The second Connect reported `CYD-RGB-LED-Matrix-HUB75-Retro-Clock 1.3.0-dev
(ESP32)` with **Visit Device** and **Change Wi-Fi**, so Improv answers in
time and `PROJECT_NAME` matches the manifest. Improv 1.0.1 (packet on a new
line) was in this image.

---

## Release check (RUNBOOK 7.4)

v1.3.0 released 10-10-2026 (tag run 37982120293). The live page loads,
`index.json` shows `1.3.0`, and the release has `*-firmware.bin`,
`*-merged.bin` and `SHA256SUMS.txt`. One Update from the live page passed on
the bench board (case 2 below).

---

## Tests owed

Smoke-tested only. Run these on the next real work on this project, or before
the next release, and tick them off with date and board MAC. This project
meets several 5b triggers (partition switch, platform pin, Improv and portal
loop changes, first release with OTA), so clear this list before the release
after v1.3.0.

- [x] Case 1 – fresh install, erased (10-10-2026, `b0:cb:d8:da:ae:8c`; the
      only board env)
- [x] Case 2 – Update on a provisioned board (10-10-2026, `b0:cb:d8:da:ae:8c`):
      live page v1.3.0 over 1.3.0-dev; `flipDisplay` changed beforehand was
      kept, WiFi kept, boot log `Version: 1.3.0`, `Running from app0`
- [ ] Case 3 – Update from `app1` (ArduinoOTA, then Update → `app0`)
- [ ] Extra – v1.2.0 board (`default.csv`, LittleFS) → Install without erase;
      settings and WiFi kept, web UI served from PROGMEM
