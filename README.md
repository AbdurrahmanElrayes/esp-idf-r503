# ESP-IDF R503 Fingerprint Sensor Driver

A clean, modular, and production-ready ESP-IDF driver for the R503 fingerprint sensor.

Designed for real-world embedded systems such as smart locks, access control, and home automation.

---

## ✨ Features

### Core
- UART-based communication (ESP-IDF native driver)
- Full packet implementation (build / parse / checksum)
- Robust error handling with readable error names

### Sensor Control
- Initialization and handshake
- Password verification
- System parameters (capacity, security level, etc.)
- Firmware / algorithm version
- Product information

### Fingerprint Operations
- Image capture (GetImage, GetImageEx)
- Feature extraction (GenChar)
- Template creation (RegModel)
- Template storage (Store)
- Template search (Search)

### Template Management
- Template count
- Delete single or multiple templates
- Clear entire database
- Index table reading
- Find next free ID

### High-Level APIs
- Manual enrollment helper
- Auto enrollment (firmware-driven)
- Manual identify
- Auto identify

### UI / Feedback
- Aura LED control:
  - Always on
  - Breathing
  - Flashing
  - Gradual on/off

---

## 📁 Project Structure

```
components/r503/
├── include/
│   ├── r503.h
│   ├── r503_defs.h
│   └── r503_errors.h
├── src/
│   ├── r503.c
│   ├── r503_core.c
│   ├── r503_packet.c
│   ├── r503_uart.c
│   ├── r503_info.c
│   ├── r503_template.c
│   ├── r503_highlevel.c
│   └── r503_led.c
```

---

## 🚀 Quick Start

### 1. Add the component

Copy the `r503` folder into:

```
components/
```

---

### 2. Include

```c
#include "r503.h"
```

---

### 3. Initialize

```c
r503_t sensor = {0};

r503_config_t cfg = {
    .uart_num = UART_NUM_1,
    .tx_pin = GPIO_NUM_17,
    .rx_pin = GPIO_NUM_18,
    .baud_rate = R503_DEFAULT_BAUDRATE,
    .address = R503_DEFAULT_ADDRESS,
    .password = R503_DEFAULT_PASSWORD,
    .rx_timeout_ms = 1000,
    .uart_buffer_size = 512,
};

ESP_ERROR_CHECK(r503_init(&sensor, &cfg));
ESP_ERROR_CHECK(r503_handshake(&sensor));
ESP_ERROR_CHECK(r503_verify_password(&sensor));
```

---

## 🔌 Wiring

| R503 Pin | ESP32 |
|---------|------|
| VCC     | 3.3V |
| GND     | GND  |
| TX      | GPIO18 (RX) |
| RX      | GPIO17 (TX) |

> ⚠️ Important  
> The R503 sensor must be powered with **3.3V** when connected directly to ESP32.  
> Using 5V without level shifting may damage the ESP32 GPIO pins.

---

## 🧠 How It Works

### Manual Enrollment Flow

```
GetImage
→ GenChar(1)
→ Remove finger
→ GetImage
→ GenChar(2)
→ RegModel
→ Store
```

---

### Auto Enrollment Flow

```
AutoEnroll
→ (capture + process internally)
→ Store
```

---

### Identification Flow

```
GetImage
→ GenChar
→ Search
→ Match ID + Score
```

---

## 📦 Examples

### basic_info
- Read system parameters
- Firmware & product info

### manual_enroll_identify
- Full manual enrollment flow
- Manual search

### auto_enroll_identify
- Fully automatic enrollment
- Callback-based progress tracking

### aura_led_demo
- LED effects showcase

### delete_templates
- Clears entire fingerprint database

> ⚠️ Warning  
> This permanently deletes all stored fingerprints.

---

## ⚠️ Notes & Behavior

- Fingerprints are stored inside the sensor (non-volatile)
- Data persists after power loss
- Auto commands rely on sensor firmware behavior
- Some firmware versions return variable packet lengths (handled internally)

---

## 🧪 Common Issues

### No match found
- Try adjusting finger placement
- Check security level

### Bad image / noisy
- Clean sensor surface
- Ensure stable power supply

### Ready byte not received
- Normal on some modules

---

## ⚙️ Requirements

- ESP-IDF v5.x+
- ESP32 or compatible target

---

## 📄 License

Apache License 2.0

---

## 👨‍💻 Author

Built for real-world embedded systems:
- Smart locks
- Access control
- Home automation (Home Assistant)
