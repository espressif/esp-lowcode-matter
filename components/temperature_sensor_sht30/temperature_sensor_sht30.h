/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the SHT30 temperature sensor.
 *
 * This function sets up the sensor for communication over the given I2C port.
 *
 * @param[in] i2c_port I2C port number to which the sensor is connected.
 * @return 0 on success, non-zero on failure.
 */
int temperature_sensor_sht30_init(int i2c_port);

/**
 * @brief Read temperature in Celsius from the SHT30 sensor.
 *
 * This function reads the temperature data from the sensor and stores it in
 * the provided pointer.
 *
 * @param[in] i2c_port I2C port number to which the sensor is connected.
 * @param[out] temperature Pointer to a float where the temperature will be stored.
 * @return 0 on success, non-zero on failure.
 */
int temperature_sensor_sht30_get_celsius(int i2c_port, float *temperature);

/**
 * @brief Read relative humidity from the SHT30 sensor.
 *
 * This function reads the humidity data from the sensor and stores it in
 * the provided pointer.
 *
 * @param[in] i2c_port I2C port number to which the sensor is connected.
 * @param[out] humidity Pointer to a float where the relative humidity (0-100 %) will be stored.
 * @return 0 on success, non-zero on failure.
 */
int temperature_sensor_sht30_get_humidity(int i2c_port, float *humidity);

/**
 * @brief Read temperature and relative humidity from the SHT30 sensor in a single measurement.
 *
 * This function reads both values in one I2C transaction, which is more efficient
 * than calling the individual functions separately.
 *
 * @param[in]  i2c_port    I2C port number to which the sensor is connected.
 * @param[out] temperature Pointer to a float for temperature in Celsius (may be NULL).
 * @param[out] humidity    Pointer to a float for relative humidity in % (may be NULL).
 * @return 0 on success, non-zero on failure.
 */
int temperature_sensor_sht30_get_celsius_and_humidity(int i2c_port, float *temperature, float *humidity);

#ifdef __cplusplus
}
#endif
