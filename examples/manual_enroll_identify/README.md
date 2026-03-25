# manual_enroll_identify

Manual ESP-IDF example for fingerprint enrollment and identification using the R503 sensor.

## What it does

- Initializes the sensor
- Performs handshake and password verification
- Finds the next free template ID
- Captures the same finger twice
- Generates two feature buffers
- Merges them into a template
- Stores the template
- Captures again and searches the fingerprint database

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