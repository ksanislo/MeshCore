# Scoping: porting the LVGL companion to LilyGo T-Display P4 (ESP32-P4)

Status: **COMMITTED PORT — T-Display-P4 AMOLED is the primary target; needs first-rate support.**
Work on the `esp32p4` branch (2026-06). This is a full platform port (weeks), greenlit deliberately.

**Milestone 1a DONE:** the companion COMPILES on P4 (pioarduino 55.03.39). **But pure Arduino won't
BOOT this board** — proven empirically: silent in every config (both USB modes, both USB-C ports,
conservative flash, PSRAM off, full-image flash, USB-Serial/JTAG). Root cause (control-test confirmed):
this board's 32MB PSRAM is **HEX/16-line mode @200MHz** (`CONFIG_SPIRAM_MODE_HEX`), which the
**prebuilt** Arduino/pioarduino IDF libs + bootloader can't express, so the bootloader hangs before any
console. **Control test:** flashing the reference **Meck-P4** (pure-IDF) merged bin boots fine and
brings up USB-CDC on the same port → hardware + flash-setup are 100% good; the wall is our toolchain.

**Committed approach: PlatformIO `framework = arduino, espidf`** — build IDF from source with OUR
`sdkconfig.defaults` (HEX PSRAM etc.), arduino-esp32 as a component, keeping our C++ codebase.

**Reference:** github.com/pelgraine/Meck-P4 (a MeshCore fork, pure ESP-IDF, both panels working) — the
authoritative source for sdkconfig, pins, XL9535 handling, DSI panel init, C6/ESP-Hosted. Its
`sdkconfig.defaults`: `CONFIG_SPIRAM_MODE_HEX`, `CONFIG_SPIRAM_SPEED_200M`, `CONFIG_ESPTOOLPY_FLASHFREQ_120M`,
`CONFIG_CACHE_L2_CACHE_256KB`/`_LINE_128B`, 16MB flash, TinyUSB CDC; partitions = nvs 0x6000 / phy 0x1000 /
factory app 15M. IDF v5.4.1.

**Milestone roadmap:** M2 boot (arduino+espidf + custom sdkconfig → USB console) · M3 radio (XL9535
expander HAL) · M4 display (RM69A10 DSI via esp_lcd + GT9895 touch) · M5 connectivity (C6/ESP-Hosted:
WiFi/MQTT/OTA + BLE) · M6 peripherals+release (BQ27220, L76K GPS, PCF8563, SDMMC, portrait 568x1232).

## ★★ OUR BUILD BOOTS (2026-07) — full working recipe
Our own `p4/` project boots on the device: rev v1.0 chip, **32MB PSRAM @ 200MHz**, our app_main
heartbeat running. **Build with ESP-IDF v5.4.1 via idf.py (NOT pio/5.5.4).** Four root causes, all
toolchain/observation:
1. **IDF v5.4.1** — 5.5.4 defaults `REV_MIN_301` and its rev-<3 path won't link (`_bss_start_low`);
   5.4.1 defaults `REV_MIN_1` (v0.1) and runs 200MHz HEX PSRAM on our silicon.
2. **Chip is rev v1.0** — build for the low rev (5.4.1 default is fine).
3. **Console = CH340 UART-bridge port** (the non-OTG USB-C), not the OTG/flash port.
4. **CH340 DTR/RTS hold EN/BOOT** — open the serial port with **DTR=False, RTS=False** or the board
   stays held in reset and looks silent (this was the last gremlin; esptool's watchdog-reset also
   doesn't reliably run the app — a manual RESET/EN press does).

**Reproduce (our p4/ project):**
`source scratchpad/esp-idf-5.4.1/export.sh; cd p4; idf.py set-target esp32p4; idf.py build;`
`idf.py -p <OTG-port> flash;` then read the **CH340 port** at 115200 with DTR/RTS de-asserted and tap
RESET. Console = UART_DEFAULT; sdkconfig has HEX PSRAM 200M + IDF_EXPERIMENTAL_FEATURES + 16MB/80M flash.
(pio env in p4/platformio.ini is retired for this board — use idf.py 5.4.1.)

**Next:** vendor the LilyGo board tree (cpp_bus_driver + private_library) + MeshCore `src/` as a
component onto this proven p4/ + 5.4.1 base (the Meck-P4 architecture), and build the radio.

## ★ BOOT PROVEN (2026-06) — the two things that blocked everything
A LilyGo example (`iic_scan`), built **as-shipped with ESP-IDF v5.4.1**, **BOOTS on our board** and its
I2C scan finds the real devices (0x5D GT9895 touch, etc.). Two root causes, both toolchain/observation —
NOT hardware:
1. **Use ESP-IDF v5.4.1, NOT pio's 5.5.4.** Our chip is **ESP32-P4 rev v1.0** (early ES silicon). IDF
   5.5.4 defaults `CONFIG_ESP32P4_REV_MIN_301` (rev v3.1) → firmware won't run, hangs before app_main.
   IDF **5.4.1 defaults `CONFIG_ESP32P4_REV_MIN_1`** (rev v0.1) → compatible. Any P4 build for THIS board
   must target `REV_MIN_1` (or `_100`). Install: clone esp-idf v5.4.1 + `install.sh esp32p4` (in
   ~/.espressif; env at scratchpad/esp-idf-5.4.1). Flash freq as-shipped = 80M (not 120M).
2. **Console is on the OTHER USB-C port (a CH340 USB-UART bridge).** The board has TWO USB-C ports:
   - **USB-OTG port** = flashing / download mode / the factory TinyUSB "Device 123456" companion CDC.
   - **CH340 UART-bridge port** (`1a86_USB_Single_Serial`) = the `UART_DEFAULT` console (`printf`/ESP_LOG).
   We were watching the OTG port the whole time; the console comes out the CH340 port. Read it at 115200.
   esptool's "hard reset with watchdog" does NOT reliably run the app — a MANUAL RESET/EN press is needed.

**Build+flash+monitor the LilyGo baseline (reproducible):**
`source scratchpad/esp-idf-5.4.1/export.sh; cd scratchpad/T-Display-P4; idf.py set-target esp32p4;`
(select example via `CONFIG_EXAMPLE_BUILD_IIC_SCAN=y` in sdkconfig.defaults) `idf.py build; idf.py -p
<OTG-port> flash;` then read the **CH340 port** at 115200 and tap RESET. (`iic_scan` builds clean;
`lvgl_9_ui` needs `ENABLE_USB_DISPLAY` set so esp_tinyusb resolves.)

**Next:** build MeshCore on this proven base — IDF 5.4.1 + LilyGo board tree (cpp_bus_driver +
private_library) + MeshCore `src/` as a component (the Meck-P4 architecture), targeting REV_MIN_1.

## M2 progress (2026-06)
**The hard science is SOLVED:** pioarduino's **`custom_sdkconfig`** option (`framework` stays `arduino`)
triggers a "HybridCompile" that rebuilds IDF + arduino-esp32 **from source** with our merged sdkconfig —
so this board's **HEX PSRAM** builds and the whole IDF (mbedtls, etc.) + our app **compiled and linked**.
Recipe (in `variants/lilygo_tdisplay_p4/platformio.ini`):
- `board = lilygo-tdisplay-p4` (`boards/lilygo-tdisplay-p4.json`: mcu esp32p4, 16MB, `psram_type hex`,
  f_psram 200M, f_flash 120M qio).
- `custom_sdkconfig`: `CONFIG_SPIRAM=y`, `CONFIG_SPIRAM_MODE_HEX=y`, `CONFIG_SPIRAM_SPEED_200M=y`,
  `CONFIG_ESPTOOLPY_FLASHFREQ_120M=y`, `CONFIG_CACHE_L2_CACHE_256KB=y`/`_LINE_128B=y`,
  `CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y`, and `CONFIG_FMB_MASTER_MAX_API_BLOCKING_TIME_MS=10000` (arduino
  base leaves esp-modbus's blocking<timeout assertion tripped — one-line fix).
- `board_build.partitions = variants/lilygo_tdisplay_p4/partitions.csv` (must be a real file path in the
  IDF flow, not the arduino `default_16MB.csv` name).
- Bring-up console = `ARDUINO_USB_MODE=1` (USB-Serial/JTAG, on the plugged port); TinyUSB CDC (mode 0)
  needs `CONFIG_TINYUSB_CDC_ENABLED` and is deferred to the phone-companion data port.
- First build is SLOW (compiles IDF from source, minutes).

**OPEN M2 ISSUE (build-system plumbing, not science):** HybridCompile swaps in a `.dummy` "Hello World"
sketch (its own `setup/loop`) as `PROJECT_SRC_DIR` to build the libs; our `build_src_filter` (which
reaches into `../variants`/`../examples`) then links our source **alongside** the dummy → duplicate
`setup/loop`. Two candidate resolutions, decide next: (A) tame the HybridCompile two-pass so `.dummy`
isn't linked into the app (e.g. app sketch in the real src_dir), or (B) move this variant to
`framework = arduino, espidf` (explicit CMake `idf_component_register(SRCS ...)`, no `build_src_filter`
— cleaner/deterministic, matches the pure-IDF reference, but must enumerate our sources in CMake).
Reference control test already proved the HW + flashing are good, so this is the only thing between us
and a booting board.
Verdict: **feasible but it's a platform port, not a variant add.** Two of the three pillars our UI
rests on (LovyanGFX display, native BLE/WiFi radio) do not exist on ESP32-P4 and must be
re-backended. Weeks of work vs. the few-day "new variant" the CrowPanel 2.4–7.0 siblings were.

---

## CONFIRMED HARDWARE — T-Display-P4 AMOLED (2026-06)
Ground truth from LilyGo's own `components/private_library/t_display_p4_config.h`
(github.com/Xinyuan-LilyGO/T-Display-P4) + repo README + BQ27220 PDF in-repo. **Corrects the
earlier guesses in this doc** (esp. "maybe QSPI AMOLED" → it's DSI; and the radio pin story).

**🔴 The dominant fact: an XL9535 I2C GPIO expander (addr 0x20, INT on GPIO5, I2C-1) gates almost
everything** — power rails AND the SX1262's RST/DIO1. So *even the LoRa-only milestone* must bring
up I2C + the XL9535 and assert power-enables before any peripheral responds. Peripheral power
sequencing is expander-first.

- **Display: MIPI-DSI, NOT QSPI.** Controller **RM69A10**, AMOLED **568×1232**, 2-lane DSI @
  1000 Mbps/lane, 60 MHz DPI. Reset via `XL9535 IO2`. AMOLED brightness = DSI DCS command (no
  backlight GPIO). (TFT SKU is HI8561 540×1168 — not our board.) → highest-risk pillar, deferred.
- **Touch: GT9895** (I2C-1 `SDA=7/SCL=8`, addr 0x5D). RST=`XL9535 IO3`, INT=`XL9535 IO4`.
- **LoRa: SX1262** (830–945 MHz; some BOMs use LR2021 — assume SX1262). SPI host "SPI_1":
  `SCLK=2, MOSI=3, MISO=4`; `CS=24`, `BUSY=6`; **`RST=XL9535 IO16`, `DIO1=XL9535 IO17` (expander!)**.
  RF switch `SKY13453 VCTL = XL9535 IO1`. SPI_1 is also on the `EXT_2X8P` header (share → mutex if
  used). **SD and display do NOT share this SPI** (SD=SDMMC, display=DSI). TCXO/DIO2-RF-switch
  config UNCONFIRMED (check LilyGo RadioLib example). **DIO1-on-expander = the #1 port risk**:
  RadioLib wants a native IRQ pin; start by **polling DIO1 via the expander**, optimize to the
  XL9535 INT (GPIO5) later.
- **SD: SDMMC 4-bit** (`CLK=43, CMD=44, D0=39,D1=40,D2=41,D3=42`, power-enable `XL9535 IO15`,
  `/sdcard`). Board also defines an SPI-SD fallback (`SCLK=43,MOSI=44,MISO=39,CS=42`). Our
  SdFat/SdSvc assumes SPI → SPI-fallback may be the cheaper path. Deferred.
- **Connectivity: ESP32-C6-MINI (4 MB) over SDIO 4-bit** (ESP-Hosted): P4 side `CLK=18, CMD=19,
  D0=14,D1=15,D2=16,D3=17` (matches Espressif ESP32-P4-Function-EV exactly). C6 enable=`XL9535 IO14`,
  wake=`XL9535 IO13`. Community: needs ESP-Hosted ≥ v2.9.4 on host+slave or WiFi drops. Deferred.
- **Fuel gauge: TI BQ27220** (I2C-1, 0x55) — real gas gauge, V+% free (unlike CrowPanel).
- **RTC: PCF8563** (I2C-1, 0x51) — same chip we already support. **GPS: L76K** UART (`TX=22, RX=23`,
  wake `XL9535 IO11`). **IMU** ICM-20948, **audio** ES8311+NS4150B, **haptic** AW86224 — all on I2C-2
  (`SDA=20/SCL=21`). **PMIC** SGM38121 (I2C-2, 0x28); rail enables via XL9535 (3V3=IO0, 5V=IO6,
  P4 VCCA=IO10).
- **Silicon:** 32 MB PSRAM, 16 MB flash, **native USB** (dual USB-C + USB-A host; no serial bridge),
  boot button GPIO35. Two I2C buses (I2C-1 SDA7/SCL8, I2C-2 SDA20/SCL21).
- **LilyGo stack = ESP-IDF 5.4 + LVGL 9 + their `cpp_bus_driver`** → reference only; we're
  Arduino/pioarduino + LVGL 8.3.

Repo: github.com/Xinyuan-LilyGO/T-Display-P4 (schematic + BQ27220 PDF under `information/`).

---

## Why the CrowPanel siblings were cheap and this isn't

The 2.4/2.8/4.3/5.0/7.0 CrowPanels were a few-day, blind-buildable add because they share the
**exact stack** of our reference board: ESP32-S3, stock `espressif32` Arduino, LovyanGFX display,
native BLE + WiFi. A variant is then just pins + an LGFX header + a `platformio.ini` env
(see CLAUDE.md "variant boundary"; ~9 files, zero changes to shared code).

The P4 shares the *application* (mesh, UITask logic, MeshProxy) but **none of the three platform
pillars**. Each pillar that differs forces work *below* the variant boundary, which is exactly
what the golden rule tells us to avoid — so this port stresses the architecture, it doesn't slot
into it.

---

## The three pillars, by severity

### 1. Display — the real wall (LovyanGFX has no P4 path *at all*)
- Our entire UI — `UITask`, `MapView` (incl. the new prefetch engine), `LGFXDisplay`,
  `MapThumb` — renders through **LovyanGFX**. The pinned LovyanGFX (`lovyan03/LovyanGFX@^1.2.7`)
  ships **no `esp32p4` platform directory at all** — verified in the installed package: no
  `esp32p4` dir, no DSI panel, *and no `esp32p4/Bus_RGB.hpp`* either.
- **This is the key correction to the naive read:** the wall is *not* "DSI vs RGB." LovyanGFX
  drivers are **per-SoC** (`esp32`, `esp32s3`, `esp32c3`, …). Our RGB big-CrowPanels work only
  because they bolt to **`lgfx/v1/platforms/esp32s3/Bus_RGB.hpp`** — there is no `esp32p4`
  equivalent. So **any** P4 board needs a non-LovyanGFX display backend regardless of panel type:
  feed LVGL from Espressif's **`esp_lcd`** driver directly (an LVGL `flush_cb` over an
  `esp_lcd_panel_handle_t`), plus a new touch path without LovyanGFX's `Touch_*` helpers.
- The interface only decides *which* `esp_lcd` backend you write — see the two-path split below.
- Knock-on: anywhere we touch LovyanGFX outside the flush path (rotation, any `lgfx::` type)
  needs a P4 shim. `board_set_backlight` is already a weak hook, so backlight is clean; the
  panel/touch bring-up is the cost.

#### Two distinct P4 display paths (different boards, different risk)
The "P4 with RGB" boards and the T-Display P4 are **different efforts** — picking the right entry
point matters if a P4 port is ever greenlit.

- **Path A — RGB CrowPanel Advance P4 (LOWER risk, the sane entry point).** Meshtastic's
  `crowpanel-p4` branch targets exactly this: envs `crowpanel-advanced-p4-50` and
  `crowpanel-advanced-p4-70-90-101` = CrowPanel Advance **5.0 / 7.0 / 9.0 / 10.1**, RGB-parallel,
  but on **ESP32-P4** silicon. This is the **same physical product family we already build on S3**
  (`elecrow_crowpanel_advance_50/_70`, `board = esp32-s3-devkitc-1`, RGB 800×480 via
  `esp32s3/Bus_RGB`). The split is **chip, not panel** — same Elecrow timings, same `CROWPANEL_RGB`
  board-platform branch, same UITask. The 9/10.1 sizes are effectively **P4-only** (S3 can't push
  that pixel bandwidth). The display work is a **single `esp_lcd` RGB → LVGL flush** (well-trodden
  on P4) reusing our known panel timings — much smaller than DSI. **If we ever do P4, start here.**

- **Path B — LilyGo T-Display P4 (HIGHER risk, the original question).** A small board, almost
  certainly **MIPI-DSI**. DSI-via-`esp_lcd` is real but less trodden, the panel/touch controllers
  are unknown until we have the schematic, and there's **no ecosystem reference for it** — even
  Meshtastic's P4 config enables only `CONFIG_SOC_LCD_I80_SUPPORTED` + `CONFIG_SOC_LCD_RGB_SUPPORTED`,
  **not** DSI. This stacks the DSI unknown on top of everything in Path A.

Both paths carry the **no-native-radio / C6 co-processor** connectivity cost (pillar 2) equally —
that's chip-level, independent of the display. So Path A isolates the connectivity port without
also gambling on DSI; Path B does not. Neither reuses our LovyanGFX RGB code.

### 2. Connectivity — the P4 has no native radio
- ESP32-P4 has **no native WiFi and no native BLE**. Our firmware leans on both:
  - **BLE companion** (the phone app — `BLE_PIN_CODE`, the whole NimBLE companion path).
  - **Native WiFi** — `WITH_WIFI`, `WITH_MQTT_BRIDGE`, NTP, and **the manifest-OTA system**
    (`MyMesh::updateReleaseList()` + the hourly refresh) just reworked.
- Meshtastic's answer (from `variants/esp32p4/esp32p4.ini`): **ESP-Hosted + an ESP32-C6
  co-processor over 4-bit SDIO**, with BLE tunneled as **NimBLE-over-VHCI**. Key sdkconfig:
  `CONFIG_BT_CONTROLLER_DISABLED=y`, `CONFIG_ESP_WIFI_REMOTE_ENABLED=y`,
  `CONFIG_ESP_HOSTED_ENABLED=y`, `CONFIG_ESP_HOSTED_TRANSPORT_SDIO=y`,
  `CONFIG_ESP_HOSTED_IDF_SLAVE_TARGET="esp32c6"`, `CONFIG_ESP_HOSTED_ENABLE_BT_NIMBLE=y`,
  `CONFIG_ESP_HOSTED_NIMBLE_HCI_VHCI=y`, `CONFIG_ESP_HOSTED_SDIO_RESET_DELAY_MS=1500`.
- **Hard prerequisite:** the T-Display P4 must actually *carry a C6/H2 co-processor* wired for
  SDIO. If it doesn't, there is **no WiFi/BLE at all** and the board is USB-serial-companion only
  (acceptable for a mesh node, but it kills the phone app + WiFi-OTA dev loop we depend on).
- Re-plumbing scope: the BLE companion transport and the WiFi/MQTT/NTP/OTA stack would have to
  run against `esp_wifi_remote` + hosted-NimBLE instead of the native stacks. This is below the
  variant boundary (touches `examples/companion_radio` connectivity), and must stay `#ifdef`-gated
  so it can't regress the S3 boards.

### 3. Framework/toolchain — the easy pillar (already half-done)
- The **pioarduino fork** that adds P4 (Arduino 3.x / IDF 5.3) is **already in our tree**:
  `esp32c6_base` uses
  `https://github.com/pioarduino/platform-espressif32/releases/download/53.03.13-1/platform-espressif32.zip`.
  An `esp32p4_base` extends the same platform — `board_build.mcu = esp32p4`, a P4 partition
  table, PSRAM config. Mechanically straightforward; the risk is library-compat churn on the
  newer IDF (our lib_deps were validated against `espressif32@6.11.0`).
- Note `merge_factory.py` hardcodes `--chip esp32s3`; a P4 variant needs `esp32p4` (and possibly
  different flash offsets). Trivial, but don't forget it.

---

## What LoRa/SD look like (the parts that *do* port)
- **LoRa/RadioLib**: ports cleanly. SX1262 over SPI works on P4; our `HspiLockHal` bus-mutex
  pattern is chip-agnostic. Just need the T-Display's real SX126x pin map.
- **SD**: P4 boards typically use **SDMMC** (4-bit), not SPI SD. Meshtastic sets
  `CONFIG_ARDUINO_SELECTIVE_SD_MMC=y`. Our `SdFat`/`SdSvc` + the `lv_fs` driver + the bus-share
  mutex assume **SPI SD**. Either wire the T-Display's SD as SPI (if the board exposes it) or add
  an SDMMC path to `SdSvc` — the latter is real work and touches the map/emoji/chat-store SD code.
- **LDO/GPIO caveat** (from Meshtastic's P4 `pins_arduino.h`): high-numbered GPIOs hit LDO power
  issues; SPI pins can collide with SDMMC slot0 and need a "SDMMC POWER" bus-type pre-tag in
  `variant.cpp`. Budget time for board-bring-up gotchas like this.

---

## What Meshtastic actually gives us (and what it doesn't)
**Gives:**
- `variants/esp32p4/esp32p4.ini` (main) — generic P4 SDK config: pioarduino platform, the full
  C6/SDIO ESP-Hosted + NimBLE-over-VHCI setup, SDMMC, component-prune list, linker response-file
  workaround. The blueprint for pillar 2 & 3.
- `origin/crowpanel-p4` branch — a *real working* P4 variant (`variant.h/.cpp`, `pins_arduino.h`,
  `boards/crowpanel-p4.json`, LoRa pins, SDMMC, LDO workarounds). Good structural template.

**Doesn't give:**
- **Anything T-Display P4 specific** — no pin map, no display config, no board JSON.
- **A MIPI-DSI display path** — their P4 boards are RGB parallel; DSI is unsolved ecosystem-wide.
- The `crowpanel-p4` branch is **unmerged** (expect churn / not battle-tested).

---

## Rough effort shape (if we ever do it)
1. **De-risk the toolchain (days):** `esp32p4_base` env (pioarduino) + LoRa-only build, no display,
   no BLE/WiFi. Proves the platform + RadioLib compile and a node joins the mesh over USB.
2. **Display backend (1–2 weeks):** `esp_lcd` MIPI-DSI → LVGL flush + a touch driver, behind the
   existing weak `board_*` hooks where possible; new shim where LovyanGFX types leak. **Highest
   risk; gated entirely on knowing the exact panel + touch controllers.**
3. **Connectivity (1–2 weeks):** ESP-Hosted/C6 bring-up; re-target BLE companion + WiFi/MQTT/NTP/OTA
   onto hosted stacks, `#ifdef`-gated so S3 builds are untouched. **Only if the board has a C6.**
4. **SD + polish (days):** SDMMC path or SPI-SD wiring; LDO/GPIO fixes; OTA asset prefix + merge
   script `--chip esp32p4`; new `gui_version`-tracked release env.

Net: **~3–5 weeks of platform engineering**, most of it below the variant boundary, contingent on
(a) a MIPI-DSI panel we can identify and drive via `esp_lcd`, and (b) an on-board C6 for WiFi/BLE.

---

## Decision gates before any code
1. **Pick the path.** If P4 is ever greenlit, **Path A (RGB CrowPanel Advance P4)** is the entry
   point — it isolates the connectivity port (pillar 2) and the `esp_lcd` RGB backend without the
   DSI gamble, and reuses our existing S3 RGB panel timings + `CROWPANEL_RGB` branch + UITask.
   **Path B (T-Display P4 / DSI)** only after A proves the platform, or if there's specific demand
   for that board.
2. **Get the board schematic / vendor docs** for whichever path: display controller + interface,
   touch controller, **whether a C6/H2 co-processor is present and how it's wired (SDIO pins)**,
   SX126x LoRa pins, SD interface (SDMMC vs SPI), battery ADC. For Path A, Meshtastic's
   `origin/crowpanel-p4` branch already has most of this.
3. **Confirm demand.** This is a from-scratch platform; the S3 line covers current users.
4. If clear, start at effort step 1 (toolchain de-risk) — cheap, validates pioarduino before
   sinking time into display/connectivity.

(See also memory `project-esp32p4-tdisplay-port` for the one-line index entry.)
