# Introduction
- Rust on esp32-d0wd-v3
- Rust with xtensa ported llvm backend
- Embassy RTOS runtime

## Setup
1. Install esp toolchain: ```./scripts/install_esp.sh```
2. List board information: ```espflash board-info --port <PORT>```

```code
[2026-07-23T17:17:31Z INFO ] Serial port: 'COM3'
[2026-07-23T17:17:31Z INFO ] Connecting...
[2026-07-23T17:17:38Z INFO ] Using flash stub
Chip type:         esp32 (revision v3.1)
Crystal frequency: 40 MHz
Flash size:        4MB
Features:          WiFi, BT, Dual Core, 240MHz, VRef calibration in efuse, Coding Scheme None
MAC address:       d4:e9:f4:64:c0:58
Security features: None
```

3. Generate esp project using terminal ui: ```esp-generate```
    - Chip type: ```esp32```
    - Module/board selection: ```esp32-wroom-32d/u```
    - Refer to datasheet: ```datasheet/espressif_esp32_wroom-32du_datasheet.pdf``` which refers to the ```esp32-d0wd-v3``` IC
    - Enable unstable HAL features
    - Enable allocations via the esp-alloc crate
    - Enable Wi-Fi via the esp-radio crate
    - Enable BLE via the esp-radio crate (embassy-trouble)
    - Add embassy framework support

4. Alternatively generate without terminal ui: ```esp-generate <PROJECT_NAME> --headless --chip esp32 -o esp32-wroom-32d -o unstable-hal -o alloc -o wifi -o embassy -o ble-trouble -o log -o esp-backtrace -o esp```

## Building and flashing
1. Build: ```cargo build```
2. Flash: ```cargo run -- --port <PORT>```

## List more detailed chip info
1. Install esptool: ```pip install esptool```
2. Get flash id using esptool: ```python -m esptool --port <PORT> flash-id```
3. More information about flash [here](https://docs.espressif.com/projects/esptool/en/latest/esp32/advanced-topics/spi-flash-modes.html#spi-flash-modes)

```code
esptool v5.3.1
Connected to ESP32 on COM3:
Chip type:          ESP32-D0WD-V3 (revision v3.1)
Features:           Wi-Fi, BT, Dual Core + LP Core, 240MHz, Vref calibration in eFuse, Coding Scheme None
Crystal frequency:  40MHz
MAC:                d4:e9:f4:64:c0:58

Stub flasher running.

Flash Memory Information:
=========================
Manufacturer: 68
Device: 4016
Detected flash size: 4MB
Flash voltage set by a strapping pin: 3.3V
```

4. Go to flashrom repo to find flash chip manufacturer in [flashchips.h](https://github.com/flashrom/flashrom/blob/7021823717bab5fc80db45f4e89c9044cf8a1dd4/include/flashchips.h#L204)
5. Download datasheet of flash ic to determine what it supports: ```datasheet/boya_microelectronics_by25q32es_datasheet.pdf```
6. Deterine if it supports SPI, DIO (dual input/output), QIO (quad input/output)
    - For our case ```esp32-wroom-x``` series that uses qio supported flash only works with qout/dio/dout because of a [bootloader bug](https://esp32.com/viewtopic.php?t=996)
    - Espressif states that certain flash chips require a special command to enable quad SPI and the bootloader sometimes isn't written to handle this correctly [spi-flash-modes faq](https://docs.espressif.com/projects/esptool/en/latest/esp32/advanced-topics/spi-flash-modes.html#frequently-asked-questions)
7. Modify ```.cargo/config.toml``` to change ```runner``` command do reflect supported flash modes
