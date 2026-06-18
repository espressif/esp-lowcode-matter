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

#include <stdio.h>

#include <low_code.h>
#include <system.h>
#include <i2c_master.h>
#include <temperature_sensor_sht30.h>
#include <button_driver.h>
#include <light_driver.h>

#include "app_priv.h"

#define BUTTON_GPIO_NUM (gpio_num_t)9
#define WS2812_CTRL_IO (gpio_num_t)8

#define I2C_PORT   I2C_NUM_0
#define I2C_SCL_IO (gpio_num_t)1
#define I2C_SDA_IO (gpio_num_t)2

/* Endpoint 1: Temperature Measurement cluster (0x0402)
 * Endpoint 2: Relative Humidity Measurement cluster (0x0405) */
#define ENDPOINT_TEMPERATURE 1
#define ENDPOINT_HUMIDITY    2

static const char *TAG = "app_driver";

static void app_driver_trigger_factory_reset_button_callback(void *arg, void *data)
{
    /* Update by sending event */
    low_code_event_t event = {
        .event_type = LOW_CODE_EVENT_FACTORY_RESET
    };

    low_code_event_to_system(&event);
    printf("%s: Factory reset triggered\n", TAG);
}

static void app_driver_report_temperature(float temp)
{
    int16_t temperature = temp * 100;
    low_code_feature_data_t update_data = {
        .details = {
            .endpoint_id = ENDPOINT_TEMPERATURE,
            .feature_id = LOW_CODE_FEATURE_ID_TEMPERATURE_SENSOR_VALUE
        },
        .value = {
            .type = LOW_CODE_VALUE_TYPE_INTEGER,
            .value_len = sizeof(int16_t),
            .value = (uint8_t*)&temperature,
        },
    };

    low_code_feature_update_to_system(&update_data);
}

static void app_driver_report_humidity(float humidity)
{
    /* Matter represents relative humidity as hundredths of a percent (0-10000) */
    uint16_t humidity_matter = (uint16_t)(humidity * 100);
    low_code_feature_data_t update_data = {
        .details = {
            .endpoint_id = ENDPOINT_HUMIDITY,
            .feature_id = LOW_CODE_FEATURE_ID_HUMIDITY_SENSOR_VALUE
        },
        .value = {
            .type = LOW_CODE_VALUE_TYPE_UNSIGNED_INTEGER,
            .value_len = sizeof(uint16_t),
            .value = (uint8_t*)&humidity_matter,
        },
    };

    low_code_feature_update_to_system(&update_data);
}

/*
 * Read temperature and humidity from the sensor in a single measurement.
 * Report both values to the system.
 *
 * Note: The function signature must match the required timer callback format,
 * so `timer_handle` and `user_data` are included but intentionally unused.
 */
void app_driver_read_and_report_feature(system_timer_handle_t timer_handle, void *user_data)
{
    float temperature = 0.0;
    float humidity = 0.0;

    /* Read both values in one I2C transaction */
    if (temperature_sensor_sht30_get_celsius_and_humidity(I2C_PORT, &temperature, &humidity)) {
        printf("%s: Failed to read sensor\n", TAG);
        return;
    }

    system_delay_ms(100);

    app_driver_report_temperature(temperature);
    app_driver_report_humidity(humidity);
}

int app_driver_init()
{
    printf("%s: Initializing driver\n", TAG);

    /* Initialize button */
    button_config_t btn_cfg = {
        .gpio_num = BUTTON_GPIO_NUM,
        .pullup_en = 1,
        .active_level = 0,
    };
    button_handle_t btn_handle = button_driver_create(&btn_cfg);
    if (!btn_handle) {
        printf("%s: Failed to create the button\n", TAG);
        return -1;
    }

    /* Register callback to factory reset the device on button long press */
    button_driver_register_cb(btn_handle, BUTTON_LONG_PRESS_UP, app_driver_trigger_factory_reset_button_callback, NULL);

    printf("%s: Initializing light driver\n", TAG);
    light_driver_config_t cfg = {
        .device_type = LIGHT_DEVICE_TYPE_WS2812,
        .channel_comb = LIGHT_CHANNEL_COMB_3CH_RGB,
        .io_conf = {
            .ws2812_io = {
                .ctrl_io = WS2812_CTRL_IO,
            },
        },
        .min_brightness = 0,
        .max_brightness = 100,
    };
    light_driver_init(&cfg);
    light_driver_set_power(true);

    /* Initialize I2C */
    int ret = i2c_master_init(I2C_PORT, I2C_SCL_IO, I2C_SDA_IO);
    if (ret) {
        printf("%s: Failed to initialise master i2c\n", TAG);
        return -1;
    }

    /* Initialize sensor */
    ret = temperature_sensor_sht30_init(I2C_PORT);
    if (ret) {
        printf("%s: Failed to initialise sht30 sensor\n", TAG);
        return -1;
    }

    /*
     * Create a timer that will call `app_driver_read_and_report_feature`
     * periodically every 10 sec.
     */
    system_timer_handle_t timer = system_timer_create(app_driver_read_and_report_feature, NULL, 10000, true);
    if (!timer) {
        printf("%s: Failed to create timer\n", TAG);
        return -1;
    }

    system_timer_start(timer);

    return 0;
}

int app_driver_feature_update()
{
    printf("%s: Feature update\n", TAG);

    /* Nothing to do here. The device reports the feature data by itself. */
    return 0;
}

int app_driver_event_handler(low_code_event_t *event)
{
    /* Get the events. Approriate indicators should be shown to the user based on the event. */
    printf("%s: Received event: %d\n", TAG, event->event_type);
    light_effect_config_t effect_config = {
        .type = LIGHT_EFFECT_INVALID,
        .mode = LIGHT_WORK_MODE_COLOR,
        .max_brightness = 100,
        .min_brightness = 10
    };

    /* Handle the events */
    switch (event->event_type) {
        case LOW_CODE_EVENT_SETUP_MODE_START:
            printf("%s: Setup mode started\n", TAG);
            /* Start Indication */
            effect_config.type = LIGHT_EFFECT_BLINK;
            light_driver_effect_start(&effect_config, 2000, 120000);
            break;
        case LOW_CODE_EVENT_SETUP_MODE_END:
            printf("%s: Setup mode ended\n", TAG);
            /* Stop Indication */
            light_driver_effect_stop();
            break;
        case LOW_CODE_EVENT_SETUP_DEVICE_CONNECTED:
            printf("%s: Device connected during setup\n", TAG);
            break;
        case LOW_CODE_EVENT_SETUP_STARTED:
            printf("%s: Setup process started\n", TAG);
            break;
        case LOW_CODE_EVENT_SETUP_SUCCESSFUL:
            printf("%s: Setup process successful\n", TAG);
            break;
        case LOW_CODE_EVENT_SETUP_FAILED:
            printf("%s: Setup process failed\n", TAG);
            break;
        case LOW_CODE_EVENT_NETWORK_CONNECTED:
            printf("%s: Network connected\n", TAG);
            break;
        case LOW_CODE_EVENT_NETWORK_DISCONNECTED:
            printf("%s: Network disconnected\n", TAG);
            break;
        case LOW_CODE_EVENT_OTA_STARTED:
            printf("%s: OTA update started\n", TAG);
            break;
        case LOW_CODE_EVENT_OTA_STOPPED:
            printf("%s: OTA update stopped\n", TAG);
            break;
        case LOW_CODE_EVENT_READY:
            printf("%s: Device is ready\n", TAG);
            break;
        case LOW_CODE_EVENT_IDENTIFICATION_START:
            printf("%s: Identification started\n", TAG);
            break;
        case LOW_CODE_EVENT_IDENTIFICATION_STOP:
            printf("%s: Identification stopped\n", TAG);
            break;
        case LOW_CODE_EVENT_TEST_MODE_LOW_CODE:
            printf("%s: Low code test mode is triggered for subtype: %d\n", TAG, (int)*((int*)(event->event_data)));
            break;
        case LOW_CODE_EVENT_TEST_MODE_COMMON:
            printf("%s: common test mode triggered\n", TAG);
            break;
        case LOW_CODE_EVENT_TEST_MODE_BLE:
            printf("%s: ble test mode triggered\n", TAG);
            break;
        case LOW_CODE_EVENT_TEST_MODE_SNIFFER:
            printf("%s: sniffer test mode triggered\n", TAG);
            break;
        default:
            printf("%s: Unhandled event type: %d\n", TAG, event->event_type);
            break;
    }

    return 0;
}
