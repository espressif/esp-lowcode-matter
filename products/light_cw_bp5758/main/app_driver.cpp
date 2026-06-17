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

#include <light_driver.h>

#include "app_priv.h"

#define I2C_SDA_IO ((gpio_num_t)6)
#define I2C_SCL_IO ((gpio_num_t)7)

static const char *TAG = "app_driver";

int app_driver_init()
{
    printf("%s: Initializing light driver\n", TAG);
    light_driver_config_t cfg = {
        .device_type = LIGHT_DEVICE_TYPE_BP5758,
        .channel_comb = LIGHT_CHANNEL_COMB_2CH_CW,
        .io_conf = {
            .bp5758_io = {
                .sda_io = I2C_SDA_IO,
                .scl_io = I2C_SCL_IO,
            },
        },
        .min_brightness = 0,
        .max_brightness = 100,
    };
    light_driver_init(&cfg);
    light_driver_set_temperature(4000);
    light_driver_set_brightness(100);
    light_driver_set_power(true);
    return 0;
}

int app_driver_set_light_state(bool state)
{
    printf("%s: Setting light state: %s\n", TAG, state ? "ON" : "OFF");
    return light_driver_set_power(state);
}

int app_driver_set_light_brightness(uint8_t brightness)
{
    brightness = (brightness * 100 + 127) / 255;
    printf("%s: Setting light brightness: %d\n", TAG, brightness);
    return light_driver_set_brightness(brightness);
}

int app_driver_set_light_temperature(uint16_t temperature)
{
    temperature = 1000000 / temperature;
    printf("%s: Setting light temperature: %d\n", TAG, temperature);
    return light_driver_set_temperature(temperature);
}

int app_driver_event_handler(low_code_event_t *event)
{
    /* Get the events. Appropriate indicators should be shown to the user based on the event. */
    printf("%s: Received event: %d\n", TAG, event->event_type);
    light_effect_config_t effect_config = {
        .type = LIGHT_EFFECT_INVALID,
        .mode = LIGHT_WORK_MODE_WHITE,
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
