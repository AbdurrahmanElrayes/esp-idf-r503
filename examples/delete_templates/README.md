# delete_templates

ESP-IDF example for clearing fingerprint templates from the R503 sensor.

## What it does

- Initializes the sensor
- Reads current template count
- Deletes all templates
- Verifies that the library is empty

## Warning

This example permanently deletes all stored fingerprints.

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