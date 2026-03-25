📦 esp-idf-r503

ESP-IDF component for interfacing with R503 / R5xx fingerprint sensors over UART.

Supports enrollment, identification, template management, device info, and Aura LED control.

⸻

✨ Features
	•	✅ UART communication (ESP-IDF native)
	•	✅ Full packet protocol implementation
	•	✅ Fingerprint enrollment (manual + auto ID allocation)
	•	✅ Fingerprint identification (search)
	•	✅ Template management:
	•	Store / Delete
	•	Empty library
	•	Read index table
	•	Find next free ID
	•	✅ Device info:
	•	Firmware version
	•	Algorithm version
	•	Product info (sensor type, size, capacity…)
	•	✅ Aura LED control (RGB ring)
	•	✅ Clean layered API (low-level + high-level)

⸻

🧰 Supported Hardware
	•	R503
	•	R503-M22
	•	R5xx series (tested on GR192RGB sensor)

⸻

🔌 Wiring (ESP32 example)

R503 Pin	ESP32
VCC	5V
GND	GND
TX	GPIO18 (RX)
RX	GPIO17 (TX)

⚠️ Use 5V power for stable operation.

⸻

🚀 Quick Start

1. Add component

Clone into your project:

components/
  r503/


⸻

2. Initialize

r503_t sensor = {0};

r503_config_t cfg = {
    .uart_num = UART_NUM_1,
    .tx_pin = GPIO_NUM_17,
    .rx_pin = GPIO_NUM_18,
    .baud_rate = R503_DEFAULT_BAUDRATE,
    .address = R503_DEFAULT_ADDRESS,
    .password = R503_DEFAULT_PASSWORD,
};

ESP_ERROR_CHECK(r503_init(&sensor, &cfg));
ESP_ERROR_CHECK(r503_handshake(&sensor));
ESP_ERROR_CHECK(r503_verify_password(&sensor));


⸻

3. Enroll (auto ID)

uint16_t id = 0;

ESP_ERROR_CHECK(
    r503_enroll_manual_next_free(&sensor, &id, 10000, 150)
);


⸻

4. Identify

r503_search_result_t result = {0};

ESP_ERROR_CHECK(
    r503_identify_once(&sensor, 0, 200, &result)
);

printf("MATCH ID=%u score=%u\n",
       result.match_id,
       result.match_score);


⸻

💡 Aura LED Example

r503_aura_led_config(&sensor,
                     R503_LED_BREATHING,
                     0x40,
                     R503_LED_BLUE,
                     0);

Suggested usage:

State	LED
Idle	Blue breathing
Finger	Yellow
Processing	Purple
Success	Green
Error	Red flashing


⸻

🧠 API Overview

Lifecycle

r503_init()
r503_deinit()


⸻

Core Commands

r503_handshake()
r503_check_sensor()
r503_verify_password()
r503_wait_ready()


⸻

Image / Feature

r503_get_image()
r503_get_image_ex()
r503_gen_char()
r503_reg_model()


⸻

Template Management

r503_store()
r503_search()
r503_delete()
r503_empty_library()


⸻

Index / Allocation

r503_read_index_table()
r503_find_next_free_id()


⸻

High-Level

r503_enroll_manual()
r503_enroll_manual_next_free()
r503_identify_once()


⸻

Device Info

r503_read_sys_params()
r503_template_count()
r503_get_fw_version()
r503_get_alg_version()
r503_read_product_info()


⸻

LED

r503_aura_led_config()


⸻

⚠️ Notes
	•	Search returns first matching ID, not last enrolled.
	•	Some firmware versions return shorter product info payload (handled internally).
	•	Index table bit order is LSB-first (verified in practice).
	•	Aura LED behavior depends on sensor hardware variant.

⸻

📁 Project Structure

components/r503/
  include/
  src/
main/
  main.c (example)


⸻

🛠️ Future Improvements
	•	Auto-enroll command (0x31)
	•	Auto-identify command (0x32)
	•	Image upload support
	•	Async API (event-driven)
	•	Multiple UART instances
	•	ESPHome / Home Assistant integration

⸻

## 📜 License

This project is licensed under the Apache License 2.0.

See the [LICENSE](LICENSE) file for details.
⸻

🤝 Contributing

PRs and improvements are welcome.

⸻

⭐ Acknowledgment

Based on R503 protocol documentation and real device testing.