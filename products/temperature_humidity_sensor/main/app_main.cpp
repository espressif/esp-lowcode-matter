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

#include <system.h>
#include <low_code.h>

#include "app_priv.h"

static const char *TAG = "app_main";

static void setup()
{
    /* Register callbacks */
    low_code_register_callbacks(feature_update_from_system, event_from_system);

    /* Initialize driver */
    app_driver_init();
}

static void loop()
{
    /* The corresponding callbacks are called if data is received from system */
    low_code_get_feature_update_from_system();
    low_code_get_event_from_system();
}

int feature_update_from_system(low_code_feature_data_t *data)
{
    uint16_t endpoint_id = data->details.endpoint_id;
    uint32_t feature_id = data->details.feature_id;

    printf("%s: Feature update: endpoint: %u, feature: %lu\n", TAG, endpoint_id, feature_id);
    return app_driver_feature_update();
}

int event_from_system(low_code_event_t *event)
{
    /* Handle the events from low_code_event_type_t */
    return app_driver_event_handler(event);
}

extern "C" int main()
{
    printf("%s: Starting low code\n", TAG);

    /* Pre-Initializations: This should be called first and should always be present */
    system_setup();
    setup();

    /* Loop */
    while (1) {
        system_loop();
        loop();
    }
    return 0;
}
