/*
 * Copyright 2026 Abdelrahman
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "r503_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the R503 device handle and UART driver.
 *
 * This function copies the provided configuration into the device handle,
 * applies default values where needed, and initializes the UART driver.
 *
 * @param dev Pointer to device handle.
 * @param config Pointer to user configuration.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t r503_init(r503_t *dev, const r503_config_t *config);

/**
 * @brief Deinitialize the R503 device and release UART resources.
 *
 * @param dev Pointer to device handle.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t r503_deinit(r503_t *dev);

/**
 * @brief Convert an ESP or R503-specific error code to a readable string.
 *
 * @param err Error code.
 *
 * @return Constant string describing the error.
 */
const char *r503_err_to_name(esp_err_t err);

/**
 * @brief Wait for the module ready byte after power-up.
 *
 * Some R503 firmware versions send a single ready byte (0x55) after boot.
 * On some modules this byte may be missed depending on power timing, so
 * failure here does not always mean the module is unusable.
 *
 * @param dev Pointer to device handle.
 * @param timeout_ms Timeout in milliseconds.
 *
 * @return ESP_OK if the ready byte is received, otherwise an error code.
 */
esp_err_t r503_wait_ready(r503_t *dev, uint32_t timeout_ms);

/**
 * @brief Send handshake command to verify communication with the module.
 *
 * @param dev Pointer to device handle.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t r503_handshake(r503_t *dev);

/**
 * @brief Check whether the fingerprint sensor hardware is operating normally.
 *
 * @param dev Pointer to device handle.
 *
 * @return ESP_OK if the sensor reports normal status, otherwise an error code.
 */
esp_err_t r503_check_sensor(r503_t *dev);

/**
 * @brief Verify the configured module password.
 *
 * The password used is taken from dev->cfg.password.
 *
 * @param dev Pointer to device handle.
 *
 * @return ESP_OK if password verification succeeds, otherwise an error code.
 */
esp_err_t r503_verify_password(r503_t *dev);

/**
 * @brief Read system parameters from the module.
 *
 * Fills the output structure with values such as capacity, security level,
 * device address, packet size code, and baud multiplier.
 *
 * @param dev Pointer to device handle.
 * @param out Pointer to output structure.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t r503_read_sys_params(r503_t *dev, r503_sys_params_t *out);

/**
 * @brief Read the number of valid templates currently stored in the module.
 *
 * @param dev Pointer to device handle.
 * @param count Pointer to output count.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t r503_template_count(r503_t *dev, uint16_t *count);

/**
 * @brief Read firmware version string from the module.
 *
 * @param dev Pointer to device handle.
 * @param out Pointer to output structure.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t r503_get_fw_version(r503_t *dev, r503_fw_version_t *out);

/**
 * @brief Read algorithm library version string from the module.
 *
 * @param dev Pointer to device handle.
 * @param out Pointer to output structure.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t r503_get_alg_version(r503_t *dev, r503_alg_version_t *out);

/**
 * @brief Read product information from the module.
 *
 * This includes module type, batch number, serial number, hardware version,
 * sensor type, sensor image size, template size, and template capacity.
 *
 * @param dev Pointer to device handle.
 * @param out Pointer to output structure.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t r503_read_product_info(r503_t *dev, r503_product_info_t *out);

/**
 * @brief Capture a fingerprint image (basic command).
 *
 * This command captures an image from the sensor and stores it internally.
 * It may fail if no finger is detected or image quality is poor.
 *
 * @param dev Pointer to device handle.
 *
 * @return ESP_OK on success,
 *         ESP_ERR_R503_NO_FINGER if no finger is detected,
 *         ESP_ERR_R503_BAD_IMAGE if image quality is insufficient,
 *         or another error code.
 */
esp_err_t r503_get_image(r503_t *dev);

/**
 * @brief Capture a fingerprint image (extended version).
 *
 * Similar to r503_get_image(), but may provide improved compatibility with
 * newer firmware versions and more reliable detection behavior.
 *
 * @param dev Pointer to device handle.
 *
 * @return ESP_OK on success or an error code.
 */
esp_err_t r503_get_image_ex(r503_t *dev);

/**
 * @brief Generate a character file from the captured image.
 *
 * Converts the captured image into a feature set and stores it into
 * CharBuffer1 or CharBuffer2.
 *
 * @param dev Pointer to device handle.
 * @param buffer_id Buffer index (1 or 2).
 *
 * @return ESP_OK on success,
 *         ESP_ERR_R503_BAD_IMAGE if feature extraction fails,
 *         or another error code.
 */
esp_err_t r503_gen_char(r503_t *dev, uint8_t buffer_id);

/**
 * @brief Merge CharBuffer1 and CharBuffer2 into a template.
 *
 * Combines the two feature sets into a single fingerprint template.
 * Both buffers must contain valid features before calling this function.
 *
 * @param dev Pointer to device handle.
 *
 * @return ESP_OK on success,
 *         ESP_ERR_R503_MERGE_FAIL if merging fails,
 *         or another error code.
 */
esp_err_t r503_reg_model(r503_t *dev);

/**
 * @brief Store a template from a character buffer into flash.
 *
 * Saves the template into the module's internal library at the given ID.
 *
 * @param dev Pointer to device handle.
 * @param buffer_id Source buffer (1 or 2).
 * @param location_id Target template ID (0 .. capacity-1).
 *
 * @return ESP_OK on success,
 *         ESP_ERR_R503_BAD_LOCATION if ID is invalid,
 *         ESP_ERR_R503_LIBRARY_FULL if storage is full,
 *         or another error code.
 */
esp_err_t r503_store(r503_t *dev, uint8_t buffer_id, uint16_t model_id);


/**
 * @brief Search for a matching fingerprint in the library.
 *
 * Compares the features in the specified buffer against templates
 * stored in the library within a given range.
 *
 * @param dev Pointer to device handle.
 * @param buffer_id Buffer index (usually 1).
 * @param start_page Starting template ID.
 * @param page_num Number of templates to search.
 * @param out Pointer to result structure.
 *
 * @return ESP_OK on success (match found),
 *         ESP_ERR_R503_NOT_FOUND if no match is found,
 *         or another error code.
 */
esp_err_t r503_search(r503_t *dev,
                      uint8_t buffer_id,
                      uint16_t start_id,
                      uint16_t count,
                      r503_search_result_t *out);

/**
 * @brief Delete one or more templates from the library.
 *
 * @param dev Pointer to device handle.
 * @param location_id Starting template ID.
 * @param count Number of templates to delete.
 *
 * @return ESP_OK on success,
 *         ESP_ERR_R503_DELETE_FAIL if deletion fails,
 *         or another error code.
 */
esp_err_t r503_delete(r503_t *dev, uint16_t start_id, uint16_t count);

/**
 * @brief Delete all templates from the library.
 *
 * This operation clears the entire fingerprint database.
 *
 * @param dev Pointer to device handle.
 *
 * @return ESP_OK on success,
 *         ESP_ERR_R503_EMPTY_FAIL if operation fails,
 *         or another error code.
 */
esp_err_t r503_empty_library(r503_t *dev);

/**
 * @brief Read index table for template usage.
 *
 * The module organizes templates into pages, each containing 32 bits.
 * Each bit indicates whether a template slot is occupied.
 *
 * @param dev Pointer to device handle.
 * @param page Index page number.
 * @param out Pointer to buffer of 32 bytes.
 *
 * @return ESP_OK on success or an error code.
 */
esp_err_t r503_read_index_table(r503_t *dev, uint8_t page_index, r503_index_table_t *out);

/**
 * @brief Find the next available (empty) template ID.
 *
 * Scans the index table to locate the first free slot.
 *
 * @param dev Pointer to device handle.
 * @param out_id Pointer to result ID.
 *
 * @return ESP_OK on success,
 *         ESP_ERR_R503_LIBRARY_FULL if no free slot exists,
 *         or another error code.
 */
esp_err_t r503_find_next_free_id(r503_t *dev, uint16_t *out_id);

/**
 * @brief Perform manual fingerprint enrollment.
 *
 * This function guides enrollment using the low-level manual flow:
 * capture image -> generate features twice -> merge -> store template.
 *
 * The target template ID is provided in the config structure.
 *
 * @param dev Pointer to device handle.
 * @param cfg Pointer to enrollment configuration.
 *
 * @return ESP_OK on success or an error code.
 */
esp_err_t r503_enroll_manual(r503_t *dev, const r503_enroll_config_t *cfg);


/**
 * @brief Perform manual enrollment using the next available template ID.
 *
 * This helper automatically finds the first free slot, performs manual
 * enrollment, stores the template, and returns the assigned ID.
 *
 * @param dev Pointer to device handle.
 * @param saved_id Pointer to returned template ID.
 * @param capture_timeout_ms Timeout for image capture in milliseconds.
 * @param poll_delay_ms Poll delay between capture attempts in milliseconds.
 *
 * @return ESP_OK on success or an error code.
 */
esp_err_t r503_enroll_manual_next_free(r503_t *dev,
                                       uint16_t *saved_id,
                                       uint32_t capture_timeout_ms,
                                       uint32_t poll_delay_ms);

/**
 * @brief Capture and identify a fingerprint once using the manual flow.
 *
 * This helper captures an image, generates features, then searches the
 * fingerprint library in the given range.
 *
 * @param dev Pointer to device handle.
 * @param start_id Starting template ID.
 * @param count Number of templates to search.
 * @param out Pointer to search result structure.
 *
 * @return ESP_OK on success (match found),
 *         ESP_ERR_R503_NOT_FOUND if no match is found,
 *         or another error code.
 */
esp_err_t r503_identify_once(r503_t *dev,
                             uint16_t start_id,
                             uint16_t count,
                             r503_search_result_t *out);

/**
 * @brief Configure the Aura LED effect on the sensor.
 *
 * This controls the RGB LED ring for visual feedback.
 *
 * @param dev Pointer to device handle.
 * @param mode LED mode (always on, breathing, flashing, etc.).
 * @param speed Effect speed value.
 * @param color LED color.
 * @param times Repeat count (0 may mean infinite depending on mode/firmware).
 *
 * @return ESP_OK on success or an error code.
 */
esp_err_t r503_aura_led_config(r503_t *dev,
                               r503_led_mode_t mode,
                               uint8_t speed,
                               r503_led_color_t color,
                               uint8_t times);

/**
 * @brief Perform automatic fingerprint enrollment using module firmware.
 *
 * The module handles repeated image capture, feature generation, merging,
 * and storage internally. Optional progress callbacks can be used to track
 * intermediate steps.
 *
 * @param dev Pointer to device handle.
 * @param location_id Target template ID.
 * @param allow_override Whether an existing template at this ID may be overwritten.
 * @param allow_duplicate Whether duplicate fingerprints are allowed.
 * @param return_step_status Whether intermediate step callbacks are enabled.
 * @param require_finger_leave Whether the user must remove the finger between captures.
 * @param saved_id Pointer to returned stored ID.
 * @param cb Optional progress callback.
 * @param user_ctx User context passed back to the callback.
 *
 * @return ESP_OK on success or an error code.
 */
esp_err_t r503_auto_enroll(r503_t *dev,
                           uint16_t location_id,
                           bool allow_override,
                           bool allow_duplicate,
                           bool return_step_status,
                           bool require_finger_leave,
                           uint16_t *saved_id,
                           r503_auto_enroll_callback_t cb,
                           void *user_ctx);


/**
 * @brief Perform automatic fingerprint identification using module firmware.
 *
 * The module handles image capture, feature generation, and library search
 * internally. Optional progress callbacks can be used to monitor the steps.
 *
 * @param dev Pointer to device handle.
 * @param security_level Matching security level (typically 1..5).
 * @param start_id Starting template ID.
 * @param count Number of templates to search.
 * @param return_step_status Whether intermediate step callbacks are enabled.
 * @param search_error_times Number of retry/error attempts for the search.
 * @param out Pointer to search result structure.
 * @param cb Optional progress callback.
 * @param user_ctx User context passed back to the callback.
 *
 * @return ESP_OK on success (match found),
 *         ESP_ERR_R503_NOT_FOUND if no match is found,
 *         or another error code.
 */
esp_err_t r503_auto_identify(r503_t *dev,
                             uint8_t security_level,
                             uint16_t start_id,
                             uint16_t count,
                             bool return_step_status,
                             uint8_t search_error_times,
                             r503_search_result_t *out,
                             r503_auto_identify_callback_t cb,
                             void *user_ctx);

#ifdef __cplusplus
}
#endif