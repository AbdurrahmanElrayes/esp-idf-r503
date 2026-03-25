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

#include "esp_err.h"

#define ESP_ERR_R503_BASE               0x7000

#define ESP_ERR_R503_PACKET             (ESP_ERR_R503_BASE + 1)
#define ESP_ERR_R503_ACK                (ESP_ERR_R503_BASE + 2)
#define ESP_ERR_R503_NO_FINGER          (ESP_ERR_R503_BASE + 3)
#define ESP_ERR_R503_BAD_IMAGE          (ESP_ERR_R503_BASE + 4)
#define ESP_ERR_R503_NO_MATCH           (ESP_ERR_R503_BASE + 5)
#define ESP_ERR_R503_NOT_FOUND          (ESP_ERR_R503_BASE + 6)
#define ESP_ERR_R503_WRONG_PASSWORD     (ESP_ERR_R503_BASE + 7)
#define ESP_ERR_R503_FLASH              (ESP_ERR_R503_BASE + 8)
#define ESP_ERR_R503_TIMEOUT            (ESP_ERR_R503_BASE + 9)
#define ESP_ERR_R503_ALREADY_EXISTS     (ESP_ERR_R503_BASE + 10)
#define ESP_ERR_R503_SENSOR_ERROR       (ESP_ERR_R503_BASE + 11)
#define ESP_ERR_R503_LIBRARY_FULL       (ESP_ERR_R503_BASE + 12)
#define ESP_ERR_R503_LIBRARY_EMPTY      (ESP_ERR_R503_BASE + 13)
#define ESP_ERR_R503_BAD_LOCATION       (ESP_ERR_R503_BASE + 14)
#define ESP_ERR_R503_NOT_READY          (ESP_ERR_R503_BASE + 15)
#define ESP_ERR_R503_PROTOCOL           (ESP_ERR_R503_BASE + 16)

#define ESP_ERR_R503_ENROLL_FAIL        (ESP_ERR_R503_BASE + 17)
#define ESP_ERR_R503_MERGE_FAIL         (ESP_ERR_R503_BASE + 18)
#define ESP_ERR_R503_LOAD_FAIL          (ESP_ERR_R503_BASE + 19)
#define ESP_ERR_R503_UPLOAD_FAIL        (ESP_ERR_R503_BASE + 20)
#define ESP_ERR_R503_RECEIVE_FAIL       (ESP_ERR_R503_BASE + 21)
#define ESP_ERR_R503_DELETE_FAIL        (ESP_ERR_R503_BASE + 22)
#define ESP_ERR_R503_EMPTY_FAIL         (ESP_ERR_R503_BASE + 23)
#define ESP_ERR_R503_PASSWORD_REQUIRED  (ESP_ERR_R503_BASE + 24)
#define ESP_ERR_R503_TEMPLATE_EMPTY     (ESP_ERR_R503_BASE + 25)
#define ESP_ERR_R503_INVALID_REGISTER   (ESP_ERR_R503_BASE + 26)
#define ESP_ERR_R503_BAD_REGISTER_CONF  (ESP_ERR_R503_BASE + 27)
#define ESP_ERR_R503_BAD_NOTEPAD_PAGE   (ESP_ERR_R503_BASE + 28)
#define ESP_ERR_R503_COMM_ERROR         (ESP_ERR_R503_BASE + 29)
#define ESP_ERR_R503_UNSUPPORTED        (ESP_ERR_R503_BASE + 30)
#define ESP_ERR_R503_HW_ERROR           (ESP_ERR_R503_BASE + 31)
#define ESP_ERR_R503_EXEC_FAIL          (ESP_ERR_R503_BASE + 32)