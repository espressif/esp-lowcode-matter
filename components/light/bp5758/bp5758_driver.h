// Copyright 2024 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file bp5758_driver.h
 * @brief BP5758 I2C LED driver for controlling up to 5-channel LEDs
 *
 * BP5758 is an I2C-based 5-channel constant current LED driver.
 * It supports up to 5 channels for controlling RGB, CW, or RGBCW LEDs.
 */

#pragma once

#include <stdint.h>
#include "soc/gpio_num.h"

#ifdef __cplusplus
extern "C" {
#endif

/* BP5758 channel definitions */
#define BP5758_CHANNEL_RED     0
#define BP5758_CHANNEL_GREEN   1
#define BP5758_CHANNEL_BLUE    2
#define BP5758_CHANNEL_COLD    3
#define BP5758_CHANNEL_WARM    4
#define BP5758_CHANNEL_BRIGHTNESS  5  /* Not used by BP5758 but kept for compatibility */

/**
 * @brief Initialize BP5758 driver
 *
 * @return 0 on success, negative value on error
 */
int bp5758_driver_init(void);

/**
 * @brief Deinitialize BP5758 driver
 */
void bp5758_driver_deinit(void);

/**
 * @brief Set value for a specific channel
 *
 * @param channel Channel number (0-4)
 * @param val Value to set (0-255)
 * @return 0 on success, negative value on error
 */
int bp5758_driver_set_channel(uint8_t channel, uint8_t val);

/**
 * @brief Get value for a specific channel
 *
 * @param channel Channel number (0-4)
 * @param val Pointer to store the value
 * @return 0 on success, negative value on error
 */
int bp5758_driver_get_channel(uint8_t channel, uint8_t *val);

/**
 * @brief Update all channels to the device
 *
 * @return 0 on success, negative value on error
 */
int bp5758_driver_update_channels(void);

/**
 * @brief Register channel with GPIO (for BP5758, this registers I2C pins)
 *
 * @param channel Pin type: 0 for SDA, 1 for SCL
 * @param gpio GPIO number to use for the specified pin type
 * @return 0 on success, negative value on error
 */
int bp5758_driver_regist_channel(uint8_t channel, gpio_num_t gpio);

#ifdef __cplusplus
}
#endif
