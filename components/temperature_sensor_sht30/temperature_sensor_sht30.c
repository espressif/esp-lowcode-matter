// Copyright 2022 Espressif Systems (Shanghai) CO LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License
#include <hal/i2c_types.h>
#include <ulp_lp_core_utils.h>

#include <esp_err.h>

#include <i2c_master.h>

#include <temperature_sensor_sht30.h>

static const char *TAG = "temp_sensor_driver_sht30";

#define MAX_MEASURABLE_TEMPERATURE              125
#define MIN_MEASURABLE_TEMPERATURE              -40
#define MAX_MEASURABLE_HUMIDITY                 100
#define MIN_MEASURABLE_HUMIDITY                 0

/* Commands */

/* Single shot mode with clock stretching disabled */
#define MEASUREMENT_SINGLE_SHOT_H_CS_DIS_CMD    0x2400U
#define MEASUREMENT_SINGLE_SHOT_M_CS_DIS_CMD    0x240BU
#define MEASUREMENT_SINGLE_SHOT_L_CS_DIS_CMD    0x2416U

/* Single shot mode with clock stretching enabled */
#define MEASUREMENT_SINGLE_SHOT_H_CS_EN_CMD     0x2C06U
#define MEASUREMENT_SINGLE_SHOT_M_CS_EN_CMD     0x2C0DU
#define MEASUREMENT_SINGLE_SHOT_L_CS_EN_CMD     0x2C10U

/* Periodic mode with 0.5Hz sampling */
#define MEASUREMENT_PERIODIC_0_5HZ_H_CS_DIS_CMD 0x2032U
#define MEASUREMENT_PERIODIC_0_5HZ_M_CS_DIS_CMD 0x2024U
#define MEASUREMENT_PERIODIC_0_5HZ_L_CS_DIS_CMD 0x202FU

/* Periodic mode with 1Hz sampling */
#define MEASUREMENT_PERIODIC_1_HZ_H_CS_DIS_CMD  0x2130U
#define MEASUREMENT_PERIODIC_1_HZ_M_CS_DIS_CMD  0x2126U
#define MEASUREMENT_PERIODIC_1_HZ_L_CS_DIS_CMD  0x212DU

/* Periodic mode with 2Hz sampling */
#define MEASUREMENT_PERIODIC_2_HZ_H_CS_DIS_CMD  0x2236U
#define MEASUREMENT_PERIODIC_2_HZ_M_CS_DIS_CMD  0x2220U
#define MEASUREMENT_PERIODIC_2_HZ_L_CS_DIS_CMD  0x222BU

/* Periodic mode with 4Hz sampling */
#define MEASUREMENT_PERIODIC_4_HZ_H_CS_DIS_CMD  0x2334U
#define MEASUREMENT_PERIODIC_4_HZ_M_CS_DIS_CMD  0x2322U
#define MEASUREMENT_PERIODIC_4_HZ_L_CS_DIS_CMD  0x2329U

/* Periodic mode with 10Hz sampling */
#define MEASUREMENT_PERIODIC_10_HZ_H_CS_DIS_CMD 0x2737U
#define MEASUREMENT_PERIODIC_10_HZ_M_CS_DIS_CMD 0x2721U
#define MEASUREMENT_PERIODIC_10_HZ_L_CS_DIS_CMD 0x272AU

/* Fetch data command */
#define FETCH_DATA_CMD                          0xE000U

/* ART command */
#define ART_CMD                                 0x2B32U

/* Stop periodic measurement (takes 1ms to complete)*/
#define STOP_PERIODIC_MEASUREMENT_CMD           0x3093U

/* Soft reset */
#define SOFT_RESET_CMD                         0x30A2U

/* Heater enable */
#define HEATER_ENABLE_CMD                      0x306DU

/* Heater disable */
#define HEATER_DISABLE_CMD                     0x3066U

/* Read status register */
#define READ_STATUS_REG_CMD                    0xF32DU

#define ALERT_PENDING_STATUS_BIT               (1 << 15)
#define HEATER_ON_BIT                          (1 << 13)
#define HUMIDITY_ALERT_BIT                     (1 << 11)
#define TEMPERATURE_ALERT_BIT                  (1 << 10)
#define SYSTEM_RESET_BIT                       (1 << 4)
#define COMMAND_STATUS_BIT                     (1 << 1)
#define CHECKSUM_STATUS_BIT                    (1 << 0)

/* Clear status register */
#define CLEAR_STATUS_REG_CMD                   0x3041U

#define CRC8_POLYNOMIAL                        0x31

#define GET_TEMPERATURE_C(x)                   (((x * 175)/65535) - 45)
#define GET_TEMPERATURE_F(x)                   (((x * 315)/65535) - 49)
#define GET_HUMIDITY_RH(x)                     ((x * 100)/65535)

static uint8_t calculate_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (size_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ CRC8_POLYNOMIAL;
            } else {
                crc = (crc << 1);
            }
        }
    }
    return crc;
}

static esp_err_t sht30_send_command(int i2c_port, uint16_t cmd)
{
    uint8_t cmd_bytes[2] = {cmd >> 8, cmd & 0xFF};
    return i2c_master_write_to_device(i2c_port, 0x44, cmd_bytes, sizeof(cmd_bytes), -1);

}

static esp_err_t sht30_read_measurement(int i2c_port, float *temperature, float *humidity) {
    uint8_t data[6];
    i2c_master_read_from_device(i2c_port, 0x44, data, 6, -1);
    // Verify CRC for temperature and humidity
    if (calculate_crc8(&data[0], 2) != data[2] || calculate_crc8(&data[3], 2) != data[5]) {
        printf("%s: CRC check failed\n", TAG);
        return ESP_ERR_INVALID_CRC;
    }
    uint16_t temp_raw = (data[0] << 8) | data[1];
    uint16_t hum_raw = (data[3] << 8) | data[4];
    if (temperature) {
        *temperature = GET_TEMPERATURE_C((float)temp_raw);
    }
    if (humidity) {
        *humidity = GET_HUMIDITY_RH(hum_raw);
    }
    return ESP_OK;
}

int temperature_sensor_sht30_init(int i2c_port)
{
    esp_err_t err = sht30_send_command(i2c_port, SOFT_RESET_CMD);
    if (err != ESP_OK) {
        printf("%s: Failed to reset sensor: %d\n", TAG, err);
        return ESP_FAIL;
    }

    return ESP_OK;
}

int temperature_sensor_sht30_get_celsius(int i2c_port, float *temperature)
{
    if (!temperature) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err;

    // Start a single shot measurement
    err = sht30_send_command(i2c_port, MEASUREMENT_SINGLE_SHOT_L_CS_DIS_CMD);
    if (err != ESP_OK) {
        return err;
    }

    ulp_lp_core_delay_cycles(15);

    return sht30_read_measurement(i2c_port, temperature, NULL);
}

int temperature_sensor_sht30_get_humidity(int i2c_port, float *humidity)
{
    if (!humidity) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err;

    // Start a single shot measurement
    err = sht30_send_command(i2c_port, MEASUREMENT_SINGLE_SHOT_L_CS_DIS_CMD);
    if (err != ESP_OK) {
        return err;
    }

    ulp_lp_core_delay_cycles(15);

    return sht30_read_measurement(i2c_port, NULL, humidity);
}

int temperature_sensor_sht30_get_celsius_and_humidity(int i2c_port, float *temperature, float *humidity)
{
    if (!temperature && !humidity) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err;

    // Start a single shot measurement
    err = sht30_send_command(i2c_port, MEASUREMENT_SINGLE_SHOT_L_CS_DIS_CMD);
    if (err != ESP_OK) {
        return err;
    }

    ulp_lp_core_delay_cycles(15);

    return sht30_read_measurement(i2c_port, temperature, humidity);
}
