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
#include <stdint.h>

#include <system.h>
#include <low_code.h>

#include <button_driver.h>
#include <relay_driver.h>
#include <light_driver.h>

#include "app_priv.h"


#define BUTTON1_GPIO_NUM ((gpio_num_t)9)
#define BUTTON2_GPIO_NUM ((gpio_num_t)10)
#define RELAY1_GPIO_NUM ((gpio_num_t)1)
#define RELAY2_GPIO_NUM ((gpio_num_t)2)
#define INDICATOR_GPIO_NUM ((gpio_num_t)8)

static const char *TAG = "app_driver";

static bool socket_states[2] = {false, false};
static system_timer_handle_t auto_off_timers[2] = {nullptr, nullptr};

static void app_driver_report_socket_state(uint16_t endpoint_id);

static void auto_off_timer_callback(system_timer_handle_t timer_handle, void *arg)
{
    (void)timer_handle;
    uint16_t endpoint_id = static_cast<uint16_t>(reinterpret_cast<uintptr_t>(arg));
    printf("%s: Auto-off timer triggered for socket %d\n", TAG, endpoint_id);
    app_driver_set_socket_state(endpoint_id, false);
    app_driver_report_socket_state(endpoint_id);
}

static void manage_auto_off_timer(uint16_t endpoint_id, bool state)
{
    uint8_t socket_index = endpoint_id - 1;
    system_timer_handle_t timer = auto_off_timers[socket_index];
    if (!timer) {
        return;
    }

    int ret = system_timer_stop(timer);
    if (ret != 0) {
        printf("%s: Failed to stop auto-off timer for socket %d (err %d)\n", TAG, endpoint_id, ret);
    }

    if (state) {
        ret = system_timer_start(timer);
        if (ret != 0) {
            printf("%s: Failed to start auto-off timer for socket %d (err %d)\n", TAG, endpoint_id, ret);
        }
    }
}

static void app_driver_report_socket_state(uint16_t endpoint_id)
{
    uint8_t socket_index = endpoint_id - 1;
    low_code_feature_data_t update_data = {
        .details = {
            .endpoint_id = endpoint_id,
            .feature_id = LOW_CODE_FEATURE_ID_POWER
        },
        .value = {
            .type = LOW_CODE_VALUE_TYPE_BOOLEAN,
            .value_len = sizeof(bool),
            .value = reinterpret_cast<uint8_t *>(&socket_states[socket_index]),
        },
    };

    low_code_feature_update_to_system(&update_data);
}

static void app_driver_toggle_socket_state_button_callback(void *arg, void *data)
{
    uint8_t endpoint_id = (uint8_t)(uintptr_t)data;
    uint8_t socket_index = endpoint_id - 1;

    bool new_state = !socket_states[socket_index];
    printf("%s: Set socket %d state to %d\n", TAG, endpoint_id, new_state);
    app_driver_set_socket_state(endpoint_id, new_state);
    app_driver_report_socket_state(endpoint_id);
}

static void app_driver_trigger_factory_reset_button_callback(void *arg, void *data)
{
    /* Update by sending event */
    low_code_event_t event = {
        .event_type = LOW_CODE_EVENT_FACTORY_RESET
    };

    low_code_event_to_system(&event);
    printf("%s: Factory reset triggered\n", TAG);
}

int app_driver_init()
{
    /* Initialize relays */
    relay_driver_init(RELAY1_GPIO_NUM);
    relay_driver_init(RELAY2_GPIO_NUM);

    for (uint16_t endpoint_id = 1; endpoint_id <= 2; ++endpoint_id) {
        auto_off_timers[endpoint_id - 1] = system_timer_create(
            auto_off_timer_callback,
            reinterpret_cast<void *>(static_cast<uintptr_t>(endpoint_id)),
            500,
            false);
        if (!auto_off_timers[endpoint_id - 1]) {
            printf("%s: Failed to create auto-off timer for socket %d\n", TAG, endpoint_id);
            return -1;
        }
    }


    /* Initialize button 1 */
    button_config_t btn1_cfg = {
        .gpio_num = BUTTON1_GPIO_NUM,
        .pullup_en = 1,
        .active_level = 0,
    };
    button_handle_t btn1_handle = button_driver_create(&btn1_cfg);
    if (!btn1_handle) {
        printf("Failed to create button 1\n");
        return -1;
    }

    /* Register callbacks for button 1 */
    button_driver_register_cb(btn1_handle, BUTTON_SINGLE_CLICK, app_driver_toggle_socket_state_button_callback, (void*)1);
    button_driver_register_cb(btn1_handle, BUTTON_LONG_PRESS_UP, app_driver_trigger_factory_reset_button_callback, NULL);

    /* Initialize button 2 */
    button_config_t btn2_cfg = {
        .gpio_num = BUTTON2_GPIO_NUM,
        .pullup_en = 1,
        .active_level = 0,
    };
    button_handle_t btn2_handle = button_driver_create(&btn2_cfg);
    if (!btn2_handle) {
        printf("Failed to create button 2\n");
        return -1;
    }

    /* Register callbacks for button 2 */
    button_driver_register_cb(btn2_handle, BUTTON_SINGLE_CLICK, app_driver_toggle_socket_state_button_callback, (void*)2);
    button_driver_register_cb(btn2_handle, BUTTON_LONG_PRESS_UP, app_driver_trigger_factory_reset_button_callback, NULL);

    /* Initialize common indicator */
    light_driver_config_t cfg = {
        .device_type = LIGHT_DEVICE_TYPE_WS2812,
        .channel_comb = LIGHT_CHANNEL_COMB_3CH_RGB,
        .io_conf = {
            .ws2812_io = {
                .ctrl_io = INDICATOR_GPIO_NUM,
            },
        },
        .min_brightness = 0,
        .max_brightness = 100,
    };
    light_driver_init(&cfg);

    /* Set initial LED states */
    light_driver_set_power(socket_states[0] || socket_states[1]);

    printf("%s: App driver initialized\n", TAG);
    return 0;
}

int app_driver_set_socket_state(uint16_t endpoint_id, bool state)
{
    uint8_t socket_index = endpoint_id - 1;
    socket_states[socket_index] = state;
    printf("%s: Set socket %d state to %d\n", TAG, endpoint_id, state);

    /* Set appropriate relay */
    gpio_num_t relay_gpio = (endpoint_id == 1) ? RELAY1_GPIO_NUM : RELAY2_GPIO_NUM;
    relay_driver_set_power(relay_gpio, !state);

    manage_auto_off_timer(endpoint_id, state);

    bool any_socket_on = socket_states[0] || socket_states[1];
    light_driver_set_power(any_socket_on);

    return 0;
}

int app_driver_event_handler(low_code_event_t *event)
{
    /* Get the events. Approriate indicators should be shown to the user based on the event. */
    printf("%s: Received event: %d\n", TAG, event->event_type);
    light_effect_config_t effect_config = {
        .type = LIGHT_EFFECT_INVALID,
        .mode = LIGHT_WORK_MODE_COLOR, /* Since it is a single channel LED */
        .max_brightness = 100,
        .min_brightness = 10
    };

    /* Handle the events from low_code_event_type_t */
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
