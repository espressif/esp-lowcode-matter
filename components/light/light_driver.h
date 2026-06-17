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
 * @file light_driver.h
 * @brief Light driver component for controlling PWM LED and WS2812 lights
 *
 * This component provides functionality to control various types of lights including
 * single-color LEDs, RGB LEDs, and WS2812 addressable LEDs. It supports different
 * channel combinations and effects.
 */

#pragma once

#include <stdint.h>
#include <inttypes.h>
#include <color_format.h>
#include "soc/gpio_num.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Light channel combination types
 *
 * Defines the different combinations of light channels that can be controlled.
 */
typedef enum {
    LIGHT_CHANNEL_COMB_INVALID = 0,  /**< Invalid channel combination */
    LIGHT_CHANNEL_COMB_1CH_C,        /**< Single channel - Cold white */
    LIGHT_CHANNEL_COMB_1CH_W,        /**< Single channel - Warm white */
    LIGHT_CHANNEL_COMB_2CH_CW,       /**< Dual channel - Cold and Warm white */
    LIGHT_CHANNEL_COMB_3CH_RGB,      /**< Three channels - RGB */
    LIGHT_CHANNEL_COMB_5CH_RGBCW,    /**< Five channels - RGB + Cold and Warm white */
    LIGHT_CHANNEL_COMB_MAX,
} light_channel_comb_t;

/**
 * @brief Light device types supported by the driver
 */
typedef enum {
    LIGHT_DEVICE_TYPE_LED = 0,    /**< Standard LED device. This uses PWM driver. */
    LIGHT_DEVICE_TYPE_WS2812,     /**< WS2812 addressable LED device */
    LIGHT_DEVICE_TYPE_BP5758,     /**< BP5758 I2C LED driver device */
    LIGHT_DEVICE_TYPE_MAX,
} light_device_type_t;

/**
 * @brief GPIO configuration for different light types
 */
typedef union {
    struct {
        gpio_num_t red;    /**< GPIO for red channel */
        gpio_num_t green;  /**< GPIO for green channel */
        gpio_num_t blue;   /**< GPIO for blue channel */
        gpio_num_t cold;   /**< GPIO for cold white */
        gpio_num_t warm;   /**< GPIO for warm white */
    } led_io;             /**< Standard LED GPIO configuration */
    struct {
        gpio_num_t ctrl_io; /**< Control GPIO for WS2812 */
    } ws2812_io;          /**< WS2812 GPIO configuration */
    struct {
        gpio_num_t sda_io; /**< SDA GPIO for BP5758 I2C */
        gpio_num_t scl_io; /**< SCL GPIO for BP5758 I2C */
    } bp5758_io;          /**< BP5758 I2C GPIO configuration */
} light_io_conf_t;

/**
 * @brief Light working modes
 */
typedef enum {
    LIGHT_WORK_MODE_INVALID,  /**< Invalid mode */
    LIGHT_WORK_MODE_COLOR,    /**< Color mode (RGB) */
    LIGHT_WORK_MODE_WHITE,    /**< White mode (CW) */
    LIGHT_WORK_MODE_MAX,
} light_work_mode_t;

/**
 * @brief Light driver configuration structure
 */
typedef struct {
    light_device_type_t device_type;    /**< Type of light device */
    light_channel_comb_t channel_comb;  /**< Channel combination */
    light_io_conf_t io_conf;            /**< GPIO configuration */
    int min_brightness;                 /**< Minimum brightness level */
    int max_brightness;                 /**< Maximum brightness level */
} light_driver_config_t;

/**
 * @brief Light effect types
 */
typedef enum {
    LIGHT_EFFECT_INVALID,  /**< Invalid effect */
    LIGHT_EFFECT_BLINK,    /**< Blinking effect */
    LIGHT_EFFECT_BREATHE,  /**< Breathing effect */
} light_effect_type_t;

/**
 * @brief Light effect configuration
 */
typedef struct {
    light_effect_type_t type;           /**< Type of light effect */
    light_work_mode_t mode;             /**< Working mode of this effect */
    union {
        RGB_color_t RGB;                /**< RGB color for the effect */
        uint32_t cct;                   /**< Color temperature for the effect */
        HS_color_t HS;                  /**< HSV color space for the effect */
        CW_white_t CW;                  /**< Cold/Warm space for the effect */
    } color;                            /**< Color configuration */
    int8_t max_brightness;              /**< Maximum brightness */
    int8_t min_brightness;              /**< Minimum brightness */
} light_effect_config_t;

/**
 * @brief Initialize the light driver with given configuration
 *
 * @param config Pointer to light driver configuration structure
 * @return 0 on success, negative value on error
 */
int light_driver_init(light_driver_config_t *config);

/**
 * @brief Set the power state of the light
 *
 * @param val Power state value (0: off, 1: on)
 * @return 0 on success, negative value on error
 */
int light_driver_set_power(uint8_t val);

/**
 * @brief Set the brightness level of the light
 *
 * @param val Brightness value (0-100)
 * @return 0 on success, negative value on error
 */
int light_driver_set_brightness(uint8_t val);

/**
 * @brief Set the hue value of the light in color mode
 *
 * @param val Hue value (0-360)
 * @return 0 on success, negative value on error
 */
int light_driver_set_hue(uint16_t val);

/**
 * @brief Set the saturation value of the light in color mode
 *
 * @param val Saturation value (0-100)
 * @return 0 on success, negative value on error
 */
int light_driver_set_saturation(uint8_t val);

/**
 * @brief Set the color temperature of the light in white mode
 *
 * @param val Color temperature in Kelvin
 * @return 0 on success, negative value on error
 */
int light_driver_set_temperature(uint32_t val);

/**
 * @brief Set the working mode of the light
 *
 * @param val Working mode (0: invalid, 1: color mode, 2: white mode)
 * @return 0 on success, negative value on error
 */
int light_driver_set_color_mode(uint8_t val);

/**
 * @brief Stop old effect and start a new one
 *
 * @param effect Pointer to light effect configuration structure
 * @param speed Duration in milliseconds for one complete effect cycle
 * @param total_ms Total duration in milliseconds for the effect (-1 for infinite)
 */
void light_driver_effect_start(light_effect_config_t *effect, int speed, int total_ms);

/**
 * @brief Stop the current running effect
 */
void light_driver_effect_stop(void);

#ifdef __cplusplus
}
#endif
