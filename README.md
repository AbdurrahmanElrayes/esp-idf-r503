# ESP-IDF R503 Fingerprint Sensor Driver

A clean, modular, and production-ready ESP-IDF driver for the R503 fingerprint sensor.

Supports both low-level commands and high-level operations such as enrollment, identification, and LED control.

---

## ✨ Features

- UART-based communication (native ESP-IDF)
- Full packet handling (build / parse / checksum)
- Sensor initialization and verification
- System parameters and product information
- Template management:
  - Store
  - Search
  - Delete
  - Count
  - Index table
- Manual enrollment and identification
- Auto enrollment and auto identification (firmware-driven)
- Aura LED control (RGB effects)
- Clear error handling (`r503_err_to_name`)

---

## 📁 Project Structure

```
components/r503/
├── include/
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

Copy the `r503` folder into your project's:

```
components/
```

---

### 2. Include in your code

```c
#include "r503.h"
```

---

### 3. Initialize the sensor

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
> Using 5V without proper level shifting may damage the ESP32 GPIO pins.

---

## 📦 Examples

- basic_info
- manual_enroll_identify
- auto_enroll_identify
- aura_led_demo
- delete_templates

---

## ⚙️ Requirements

- ESP-IDF v5.x or newer
- ESP32 (or compatible target with UART support)

---

## 📄 License

Apache License 2.0
