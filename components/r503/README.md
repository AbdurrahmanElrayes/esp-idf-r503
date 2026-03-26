# R503 Fingerprint Sensor Driver (ESP-IDF)

ESP-IDF component for the R503 fingerprint sensor with full support for low-level commands and high-level operations.

---

## ✨ Features

- UART communication (ESP-IDF native)
- Full packet handling (build / parse / checksum)
- Sensor initialization and verification
- System parameters and product information
- Template management (store / search / delete / count)
- Manual and automatic enrollment
- Manual and automatic identification
- Aura LED control (RGB effects)
- Clean error handling

---

## 🚀 Installation

### Recommended (ESP-IDF Component Registry)

```bash
idf.py add-dependency "abdurrahmanelrayes/r503"
```

---

### Manual

Copy this folder into your project:

```
components/r503/
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
> Use 3.3V with ESP32. Using 5V without level shifting may damage the GPIO.

---

## 🧠 Basic Usage

```c
r503_t sensor = {0};

r503_config_t cfg = {
    .uart_num = UART_NUM_1,
    .tx_pin = GPIO_NUM_17,
    .rx_pin = GPIO_NUM_18,
    .baud_rate = R503_DEFAULT_BAUDRATE,
};

r503_init(&sensor, &cfg);
r503_handshake(&sensor);
r503_verify_password(&sensor);
```

---

## 📦 Examples

- basic_info
- manual_enroll_identify
- auto_enroll_identify
- aura_led_demo
- delete_templates

---

## 📄 License

Apache License 2.0
