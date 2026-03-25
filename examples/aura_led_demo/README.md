# aura_led_demo

ESP-IDF example for testing the R503 Aura LED control API.

## What it does

- Initializes the sensor
- Performs handshake and password verification
- Cycles through several Aura LED patterns:
  - Blue always on
  - Cyan breathing
  - Red flashing
  - Green always on
  - White gradual on
  - LED off

## Wiring

| R503 Pin | ESP32 |
|---------|------|
| VCC     | 3.3V   |
| GND     | GND  |
| TX      | GPIO18 (RX) |
| RX      | GPIO17 (TX) |

## Build

```bash
idf.py set-target esp32
idf.py build
idf.py flash monitor
```