# R503 Fingerprint Sensor Driver for ESP-IDF

A clean, modular, and production-ready ESP-IDF component for the **R503 / R5xx fingerprint sensor**.

This driver supports both **low-level fingerprint commands** and **high-level helper APIs** for enrollment, identification, template management, and Aura LED control.

It is designed for practical embedded applications such as:

- Smart locks
- Access control systems
- Door entry systems
- Home automation
- Home Assistant / ESP-based integrations

---

## ✨ Features

### Core Communication
- Native UART communication using ESP-IDF drivers
- Full packet handling:
  - packet build
  - packet parse
  - checksum verification
- Robust response validation and protocol error handling

### Sensor Control
- Handshake
- Password verification
- Sensor health check
- Ready-byte detection after boot (when supported by firmware)

### System & Device Information
- System parameters
- Firmware version
- Algorithm version
- Product information
- Template count

### Fingerprint Operations
- Image capture:
  - `GetImage`
  - `GetImageEx`
- Feature extraction:
  - `GenChar`
- Template generation:
  - `RegModel`
- Template storage:
  - `Store`
- Template search:
  - `Search`

### Template Management
- Delete one or more templates
- Empty the entire library
- Read index table
- Find next free template ID

### High-Level APIs
- Manual enrollment helper
- Manual identify helper
- Auto enrollment (sensor firmware-driven)
- Auto identify (sensor firmware-driven)
- Progress callbacks for auto flows

### Visual Feedback
- Aura LED control
- Support for multiple LED modes:
  - Always on
  - Always off
  - Breathing
  - Flashing
  - Gradual on
  - Gradual off

---

## 📦 Installation

### Option 1 — ESP-IDF Component Registry (recommended)

From your ESP-IDF project directory:

```bash
idf.py add-dependency "abdurrahmanelrayes/r503"
```

### Option 2 — Manual installation

Copy the `r503` component folder into your project:

```text
components/r503/
```

---

## ⚙️ Requirements

- ESP-IDF **v5.x or newer**
- ESP32 or compatible ESP target with UART support
- R503 / compatible R5xx fingerprint module

---

## 🔌 Wiring

### ESP32 example

| R503 Pin | ESP32 |
|----------|-------|
| VCC      | 3.3V  |
| GND      | GND   |
| TX       | GPIO18 (RX) |
| RX       | GPIO17 (TX) |

> ⚠️ Important  
> The R503 sensor should be powered with **3.3V** when connected directly to ESP32.  
> Feeding 5V logic directly into ESP32 GPIO pins may damage the board.

---

## 🚀 Quick Start

### Include the driver

```c
#include "r503.h"
#include "r503_defs.h"
```

### Initialize the sensor

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

## 🧠 Typical Workflows

### Manual Enrollment Flow

```text
GetImage
→ GenChar(1)
→ Remove finger
→ GetImage
→ GenChar(2)
→ RegModel
→ Store
```

### Manual Identification Flow

```text
GetImage
→ GenChar(1)
→ Search
→ Match ID + Score
```

### Auto Enrollment Flow

Handled internally by the sensor firmware:

```text
AutoEnroll
→ internal image capture
→ internal feature generation
→ internal template generation
→ Store
```

### Auto Identification Flow

Handled internally by the sensor firmware:

```text
AutoIdentify
→ internal image capture
→ internal feature generation
→ Search
→ Match ID + Score
```

---

## 📚 Examples

This component includes several ready-to-run examples.

### 1. `basic_info`
Reads and prints:
- system parameters
- template count
- firmware version
- algorithm version
- product information

### 2. `manual_enroll_identify`
Demonstrates:
- manual image capture
- manual feature generation
- manual enrollment
- manual search

### 3. `auto_enroll_identify`
Demonstrates:
- firmware-driven enrollment
- firmware-driven identification
- auto progress callbacks

### 4. `aura_led_demo`
Demonstrates:
- LED colors
- LED modes
- visual sensor feedback effects

### 5. `delete_templates`
Demonstrates:
- template count
- clearing the fingerprint database

> ⚠️ Warning  
> `delete_templates` permanently removes stored fingerprints.

---

## 🧩 Public API Overview

### Lifecycle
- `r503_init()`
- `r503_deinit()`

### Utility
- `r503_err_to_name()`

### Basic Communication
- `r503_wait_ready()`
- `r503_handshake()`
- `r503_check_sensor()`
- `r503_verify_password()`

### Device Information
- `r503_read_sys_params()`
- `r503_template_count()`
- `r503_get_fw_version()`
- `r503_get_alg_version()`
- `r503_read_product_info()`

### Image / Feature Operations
- `r503_get_image()`
- `r503_get_image_ex()`
- `r503_gen_char()`
- `r503_reg_model()`

### Template Operations
- `r503_store()`
- `r503_search()`
- `r503_delete()`
- `r503_empty_library()`
- `r503_read_index_table()`
- `r503_find_next_free_id()`

### High-Level Helpers
- `r503_enroll_manual()`
- `r503_enroll_manual_next_free()`
- `r503_identify_once()`
- `r503_auto_enroll()`
- `r503_auto_identify()`

### LED Control
- `r503_aura_led_config()`

---

## 📁 Component Structure

```text
components/r503/
├── idf_component.yml
├── README.md
├── LICENSE
├── include/
│   ├── r503.h
│   ├── r503_defs.h
│   ├── r503_errors.h
│   └── r503_types.h
└── src/
    ├── r503.c
    ├── r503_core.c
    ├── r503_packet.c
    ├── r503_uart.c
    ├── r503_info.c
    ├── r503_template.c
    ├── r503_highlevel.c
    └── r503_led.c
```

---

## ⚠️ Notes

- Fingerprints are stored in the sensor's internal non-volatile memory.
- Stored templates remain available after power loss.
- Auto commands depend on firmware behavior inside the sensor.
- Some firmware versions may return slightly different payload lengths; the driver handles known variations.
- In identification results, the sensor may return the **first matching template**, not necessarily the most recently stored one.

---

## 🧪 Common Issues

### `Ready byte not received`
This can be normal on some firmware versions depending on power-up timing.

### `No match found`
- Check finger placement
- Try lowering security level
- Make sure the template really exists

### `Bad image`
- Clean the sensor surface
- Use stable finger placement
- Ensure proper power supply

### UART communication problems
- Double-check TX/RX crossover
- Verify that the sensor TX is not feeding 5V logic into ESP32
- Confirm baud rate and shared ground

---

## 🛠️ Development Notes

This component was structured to keep responsibilities separated:

- transport / protocol logic
- information commands
- template operations
- high-level helpers
- LED control

That makes it easier to maintain, extend, and integrate into larger ESP-IDF projects.

---

## 📄 License

Apache License 2.0

See the `LICENSE` file for details.

---

## 👨‍💻 Author

Maintained for practical embedded use cases and real hardware testing.
