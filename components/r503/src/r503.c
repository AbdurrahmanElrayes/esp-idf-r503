#include <string.h>
#include <stdio.h>

#include "esp_check.h"
#include "esp_log.h"

#include "r503.h"
#include "r503_defs.h"
#include "r503_errors.h"
#include "r503_types.h"

/* internal UART functions */
esp_err_t r503_uart_init(const r503_t *dev);
esp_err_t r503_uart_deinit(const r503_t *dev);
esp_err_t r503_uart_write(const r503_t *dev, const uint8_t *data, size_t len);
esp_err_t r503_uart_read_exact(const r503_t *dev, uint8_t *data, size_t len, uint32_t timeout_ms);
esp_err_t r503_uart_flush_input(const r503_t *dev);

/* internal packet functions */
uint16_t r503_packet_checksum(uint8_t pid, uint16_t length, const uint8_t *payload, size_t payload_len);
esp_err_t r503_packet_build(uint32_t address,
                            uint8_t pid,
                            const uint8_t *payload,
                            size_t payload_len,
                            uint8_t *out_buf,
                            size_t out_buf_size,
                            size_t *out_len);
esp_err_t r503_packet_parse(const uint8_t *raw, size_t raw_len, r503_packet_t *packet);
uint16_t r503_packet_payload_len(const r503_packet_t *packet);

static const char *TAG = "r503";

static esp_err_t r503_map_confirm_code(uint8_t code)
{
    switch (code) {
        case R503_CONFIRM_OK:                return ESP_OK;
        case R503_CONFIRM_PACKET_ERROR:      return ESP_ERR_R503_PACKET;
        case R503_CONFIRM_NO_FINGER:         return ESP_ERR_R503_NO_FINGER;
        case R503_CONFIRM_IMAGE_MESSY:
        case R503_CONFIRM_FEATURE_FAIL:
        case R503_CONFIRM_INVALID_IMAGE:     return ESP_ERR_R503_BAD_IMAGE;
        case R503_CONFIRM_NO_MATCH:          return ESP_ERR_R503_NO_MATCH;
        case R503_CONFIRM_NOT_FOUND:         return ESP_ERR_R503_NOT_FOUND;
        case R503_CONFIRM_WRONG_PASSWORD:    return ESP_ERR_R503_WRONG_PASSWORD;
        case R503_CONFIRM_FLASH_ERROR:       return ESP_ERR_R503_FLASH;
        case R503_CONFIRM_TIMEOUT:           return ESP_ERR_R503_TIMEOUT;
        case R503_CONFIRM_ALREADY_EXISTS:    return ESP_ERR_R503_ALREADY_EXISTS;
        case R503_CONFIRM_SENSOR_ERROR:
        case R503_CONFIRM_HW_ERROR:          return ESP_ERR_R503_SENSOR_ERROR;
        case R503_CONFIRM_LIBRARY_FULL:      return ESP_ERR_R503_LIBRARY_FULL;
        case R503_CONFIRM_LIBRARY_EMPTY:     return ESP_ERR_R503_LIBRARY_EMPTY;
        case R503_CONFIRM_BAD_LOCATION:      return ESP_ERR_R503_BAD_LOCATION;
        default:                             return ESP_ERR_R503_ACK;
    }
}

static esp_err_t r503_check_dev(const r503_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev is NULL");
    ESP_RETURN_ON_FALSE(dev->initialized, ESP_ERR_INVALID_STATE, TAG, "device not initialized");
    return ESP_OK;
}

static esp_err_t r503_send_command(const r503_t *dev,
                                   uint8_t command,
                                   const uint8_t *params,
                                   size_t params_len)
{
    uint8_t payload[257] = {0};
    uint8_t packet[300] = {0};
    size_t packet_len = 0;

    ESP_RETURN_ON_FALSE(params_len <= 256, ESP_ERR_INVALID_ARG, TAG, "params too large");

    payload[0] = command;
    if (params_len > 0 && params != NULL) {
        memcpy(&payload[1], params, params_len);
    }

    ESP_RETURN_ON_ERROR(
        r503_packet_build(dev->cfg.address,
                          R503_PID_COMMAND,
                          payload,
                          params_len + 1,
                          packet,
                          sizeof(packet),
                          &packet_len),
        TAG,
        "packet build failed"
    );

    return r503_uart_write(dev, packet, packet_len);
}

static esp_err_t r503_read_packet(const r503_t *dev, r503_packet_t *packet, uint32_t timeout_ms)
{
    uint8_t header_and_meta[9] = {0};
    uint8_t rest[258] = {0}; /* max length field is payload(256)+checksum(2) */
    uint8_t full[267] = {0};

    ESP_RETURN_ON_ERROR(r503_uart_read_exact(dev, header_and_meta, sizeof(header_and_meta), timeout_ms),
                        TAG, "failed reading packet header");

    const uint16_t length = ((uint16_t)header_and_meta[7] << 8) | header_and_meta[8];
    ESP_RETURN_ON_FALSE(length >= 2, ESP_ERR_R503_PACKET, TAG, "invalid packet length");
    ESP_RETURN_ON_FALSE(length <= sizeof(rest), ESP_ERR_R503_PACKET, TAG, "packet length too large");

    ESP_RETURN_ON_ERROR(r503_uart_read_exact(dev, rest, length, timeout_ms),
                        TAG, "failed reading packet body");

    memcpy(full, header_and_meta, sizeof(header_and_meta));
    memcpy(full + sizeof(header_and_meta), rest, length);

    return r503_packet_parse(full, sizeof(header_and_meta) + length, packet);
}

static esp_err_t r503_read_ack(const r503_t *dev, r503_packet_t *packet, uint8_t *confirm_code)
{
    ESP_RETURN_ON_ERROR(r503_read_packet(dev, packet, dev->cfg.rx_timeout_ms), TAG, "read packet failed");
    ESP_RETURN_ON_FALSE(packet->pid == R503_PID_ACK, ESP_ERR_R503_PROTOCOL, TAG, "expected ACK packet");

    const uint16_t payload_len = r503_packet_payload_len(packet);
    ESP_RETURN_ON_FALSE(payload_len >= 1, ESP_ERR_R503_PROTOCOL, TAG, "ACK payload too short");

    if (confirm_code != NULL) {
        *confirm_code = packet->payload[0];
    }

    return ESP_OK;
}

static uint16_t be16_from_payload(const uint8_t *p)
{
    return ((uint16_t)p[0] << 8) | p[1];
}

static uint32_t be32_from_payload(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  |
           ((uint32_t)p[3]);
}

esp_err_t r503_init(r503_t *dev, const r503_config_t *config)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev is NULL");
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "config is NULL");

    memset(dev, 0, sizeof(*dev));
    dev->cfg = *config;

    if (dev->cfg.baud_rate <= 0) {
        dev->cfg.baud_rate = R503_DEFAULT_BAUDRATE;
    }

    if (dev->cfg.address == 0) {
        dev->cfg.address = R503_DEFAULT_ADDRESS;
    }

    if (dev->cfg.rx_timeout_ms == 0) {
        dev->cfg.rx_timeout_ms = R503_DEFAULT_TIMEOUT_MS;
    }

    if (dev->cfg.uart_buffer_size <= 0) {
        dev->cfg.uart_buffer_size = R503_DEFAULT_UART_BUF_SIZE;
    }

    ESP_RETURN_ON_ERROR(r503_uart_init(dev), TAG, "uart init failed");

    dev->initialized = true;
    return ESP_OK;
}

esp_err_t r503_deinit(r503_t *dev)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");

    esp_err_t err = r503_uart_deinit(dev);
    dev->initialized = false;
    return err;
}

esp_err_t r503_wait_ready(r503_t *dev, uint32_t timeout_ms)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");

    uint8_t byte = 0;
    esp_err_t err = r503_uart_read_exact(dev, &byte, 1, timeout_ms);
    if (err == ESP_ERR_TIMEOUT) {
        return ESP_ERR_R503_NOT_READY;
    }
    ESP_RETURN_ON_ERROR(err, TAG, "failed reading ready byte");

    ESP_RETURN_ON_FALSE(byte == R503_READY_BYTE, ESP_ERR_R503_NOT_READY, TAG, "unexpected ready byte 0x%02X", byte);
    return ESP_OK;
}

esp_err_t r503_handshake(r503_t *dev)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_ERROR(r503_send_command(dev, R503_CMD_HANDSHAKE, NULL, 0), TAG, "send handshake failed");

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read handshake ack failed");

    return r503_map_confirm_code(code);
}

esp_err_t r503_check_sensor(r503_t *dev)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_ERROR(r503_send_command(dev, R503_CMD_CHECK_SENSOR, NULL, 0), TAG, "send check sensor failed");

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read check sensor ack failed");

    return r503_map_confirm_code(code);
}

esp_err_t r503_verify_password(r503_t *dev)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");

    uint8_t params[4] = {
        (uint8_t)((dev->cfg.password >> 24) & 0xFF),
        (uint8_t)((dev->cfg.password >> 16) & 0xFF),
        (uint8_t)((dev->cfg.password >> 8) & 0xFF),
        (uint8_t)(dev->cfg.password & 0xFF),
    };

    ESP_RETURN_ON_ERROR(r503_send_command(dev, R503_CMD_VFY_PWD, params, sizeof(params)),
                        TAG, "send verify password failed");

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read verify password ack failed");

    return r503_map_confirm_code(code);
}

esp_err_t r503_read_sys_params(r503_t *dev, r503_sys_params_t *out)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_FALSE(out != NULL, ESP_ERR_INVALID_ARG, TAG, "out is NULL");

    ESP_RETURN_ON_ERROR(r503_send_command(dev, R503_CMD_READ_SYS_PARA, NULL, 0),
                        TAG, "send read sys params failed");

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read sys params ack failed");

    esp_err_t mapped = r503_map_confirm_code(code);
    ESP_RETURN_ON_ERROR(mapped, TAG, "module returned error 0x%02X", code);

    const uint16_t payload_len = r503_packet_payload_len(&ack);
    ESP_RETURN_ON_FALSE(payload_len == 17, ESP_ERR_R503_PROTOCOL, TAG, "unexpected sys params payload len: %u", payload_len);

    const uint8_t *p = &ack.payload[1]; /* skip confirmation code */

    out->status_register  = be16_from_payload(&p[0]);
    out->system_id        = be16_from_payload(&p[2]);
    out->capacity         = be16_from_payload(&p[4]);
    out->security_level   = be16_from_payload(&p[6]);
    out->device_address   = be32_from_payload(&p[8]);
    out->packet_size_code = be16_from_payload(&p[12]);
    out->baud_multiplier  = be16_from_payload(&p[14]);

    return ESP_OK;
}

const char *r503_err_to_name(esp_err_t err)
{
    switch (err) {
        case ESP_OK:                       return "ESP_OK";
        case ESP_ERR_R503_PACKET:         return "ESP_ERR_R503_PACKET";
        case ESP_ERR_R503_ACK:            return "ESP_ERR_R503_ACK";
        case ESP_ERR_R503_NO_FINGER:      return "ESP_ERR_R503_NO_FINGER";
        case ESP_ERR_R503_BAD_IMAGE:      return "ESP_ERR_R503_BAD_IMAGE";
        case ESP_ERR_R503_NO_MATCH:       return "ESP_ERR_R503_NO_MATCH";
        case ESP_ERR_R503_NOT_FOUND:      return "ESP_ERR_R503_NOT_FOUND";
        case ESP_ERR_R503_WRONG_PASSWORD: return "ESP_ERR_R503_WRONG_PASSWORD";
        case ESP_ERR_R503_FLASH:          return "ESP_ERR_R503_FLASH";
        case ESP_ERR_R503_TIMEOUT:        return "ESP_ERR_R503_TIMEOUT";
        case ESP_ERR_R503_ALREADY_EXISTS: return "ESP_ERR_R503_ALREADY_EXISTS";
        case ESP_ERR_R503_SENSOR_ERROR:   return "ESP_ERR_R503_SENSOR_ERROR";
        case ESP_ERR_R503_LIBRARY_FULL:   return "ESP_ERR_R503_LIBRARY_FULL";
        case ESP_ERR_R503_LIBRARY_EMPTY:  return "ESP_ERR_R503_LIBRARY_EMPTY";
        case ESP_ERR_R503_BAD_LOCATION:   return "ESP_ERR_R503_BAD_LOCATION";
        case ESP_ERR_R503_NOT_READY:      return "ESP_ERR_R503_NOT_READY";
        case ESP_ERR_R503_PROTOCOL:       return "ESP_ERR_R503_PROTOCOL";
        default:                          return esp_err_to_name(err);
    }
}