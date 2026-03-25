# basic_info

Basic ESP-IDF example for reading R503 sensor information.

## What it does

- Initializes the sensor
- Performs handshake
- Verifies password
- Reads system parameters
- Reads template count
- Reads firmware version
- Reads algorithm version
- Reads product information

## Wiring

| R503 Pin | ESP32 |
|---------|------|
| VCC     | 5V   |
| GND     | GND  |
| TX      | GPIO18 (RX) |
| RX      | GPIO17 (TX) |

## Build

```bash
idf.py set-target esp32
idf.py build
idf.py flash monitor
```