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

#pragma once

#include <stdio.h>
#include <cstdint>

// TODO: Remove this error list from here and create low_code_utils which has all the standard error
/* Definitions for error constants. */
#define ESP_OK          0       /*!< esp_err_t value indicating success (no error) */
#define ESP_FAIL        -1      /*!< Generic esp_err_t code indicating failure */

#define ESP_ERR_NO_MEM              0x101   /*!< Out of memory */
#define ESP_ERR_INVALID_ARG         0x102   /*!< Invalid argument */
#define ESP_ERR_INVALID_STATE       0x103   /*!< Invalid state */
#define ESP_ERR_INVALID_SIZE        0x104   /*!< Invalid size */
#define ESP_ERR_NOT_FOUND           0x105   /*!< Requested resource not found */
#define ESP_ERR_NOT_SUPPORTED       0x106   /*!< Operation or feature not supported */
#define ESP_ERR_TIMEOUT             0x107   /*!< Operation timed out */
#define ESP_ERR_INVALID_RESPONSE    0x108   /*!< Received response was invalid */
#define ESP_ERR_INVALID_CRC         0x109   /*!< CRC or checksum was invalid */
#define ESP_ERR_INVALID_VERSION     0x10A   /*!< Version was invalid */
#define ESP_ERR_INVALID_MAC         0x10B   /*!< MAC address was invalid */
#define ESP_ERR_NOT_FINISHED        0x10C   /*!< Operation has not fully completed */
#define ESP_ERR_NOT_ALLOWED         0x10D   /*!< Operation is not allowed */


#define ESP_ERR_WIFI_BASE           0x3000  /*!< Starting number of WiFi error codes */
#define ESP_ERR_MESH_BASE           0x4000  /*!< Starting number of MESH error codes */
#define ESP_ERR_FLASH_BASE          0x6000  /*!< Starting number of flash error codes */
#define ESP_ERR_HW_CRYPTO_BASE      0xc000  /*!< Starting number of HW cryptography module error codes */
#define ESP_ERR_MEMPROT_BASE        0xd000  /*!< Starting number of Memory Protection API error codes */

/**
 * @brief Feature IDs supported by the low code framework
 *
 * These IDs uniquely identify different features that can be controlled through
 * the low code framework. Each feature represents a specific functionality
 * that can be accessed and manipulated.
 */
typedef enum {
    LOW_CODE_FEATURE_ID_UNHANDLED = 0,      /*!< Unhandled feature ID */
    LOW_CODE_FEATURE_ID_POWER = 1001,       /*!< Power control feature */
    LOW_CODE_FEATURE_ID_BRIGHTNESS = 1002,   /*!< Brightness control feature */
    LOW_CODE_FEATURE_ID_COLOR_TEMPERATURE = 1003, /*!< Color temperature control */
    LOW_CODE_FEATURE_ID_HUE = 1004,         /*!< Hue control feature */
    LOW_CODE_FEATURE_ID_SATURATION = 1005,  /*!< Saturation control feature */
    LOW_CODE_FEATURE_ID_TEMPERATURE = 4004,  /*!< Temperature sensing/control */
    LOW_CODE_FEATURE_ID_COOLING_SETPOINT = 4005, /*!< Cooling setpoint control */
    LOW_CODE_FEATURE_ID_HEATING_SETPOINT = 4006, /*!< Heating setpoint control */
    LOW_CODE_FEATURE_ID_TEMPERATURE_SENSOR_VALUE = 5001,  /*!< Temperature sensor */
    LOW_CODE_FEATURE_ID_OCCUPANCY_SENSOR_VALUE = 6001,  /*!< Occupancy sensor */
    LOW_CODE_FEATURE_ID_HUMIDITY_SENSOR_VALUE = 7001,  /*!< Humidity sensor */
    LOW_CODE_FEATURE_ID_MAX = UINT32_MAX    /*!< Maximum feature ID value */
} low_code_feature_id_t;

/**
 * @brief Value types supported for feature data
 *
 * Defines the different data types that can be used to represent
 * feature values in the low code framework.
 */
typedef enum {
    LOW_CODE_VALUE_TYPE_INVALID = 0,        /*!< Invalid value type */
    LOW_CODE_VALUE_TYPE_BOOLEAN,            /*!< Boolean value type */
    LOW_CODE_VALUE_TYPE_INTEGER,            /*!< Signed integer value type */
    LOW_CODE_VALUE_TYPE_UNSIGNED_INTEGER,   /*!< Unsigned integer value type */
    LOW_CODE_VALUE_TYPE_FLOAT,              /*!< Float value type */
    LOW_CODE_VALUE_TYPE_STRING,             /*!< String value type */
    LOW_CODE_VALUE_TYPE_OCTET_STRING,       /*!< Octet string value type */
    LOW_CODE_VALUE_TYPE_ARRAY,              /*!< Array value type */
    LOW_CODE_VALUE_TYPE_CUSTOM,             /*!< Custom value type */
} low_code_feature_value_type_t;

/**
 * @brief Structure to hold feature value data
 */
typedef struct low_code_feature_value {
    low_code_feature_value_type_t type;     /*!< Type of the value */
    int value_len;                          /*!< Length of the value data */
    uint8_t *value;                         /*!< Pointer to the value data */
} low_code_feature_value_t;

/**
 * @brief Structure containing feature details
 */
typedef struct low_code_feature_details {
    uint16_t endpoint_id;                   /*!< Endpoint identifier */
    low_code_feature_id_t feature_id;       /*!< Feature identifier */
    union {
        struct {
            uint32_t cluster_id;            /*!< Matter cluster ID */
            uint32_t attribute_id;          /*!< Matter attribute ID */
            uint32_t command_id;            /*!< Matter command ID */
        } matter;                           /*!< Matter specific details */
        struct {
            char *name;                     /*!< Rainmaker parameter name */
            char *type;                     /*!< Rainmaker parameter type */
        } rainmaker;                        /*!< Rainmaker specific details */
    } low_level;                           /*!< Solution specific details */
} low_code_feature_details_t;

/**
 * @brief Complete feature data structure
 */
typedef struct low_code_feature_data {
    low_code_feature_details_t details;     /*!< Feature details */
    low_code_feature_value_t value;         /*!< Feature value */
    void *priv_data;                        /*!< Private data pointer */
} low_code_feature_data_t;

/**
 * @brief Event types supported by the low code framework
 */
typedef enum {
    LOW_CODE_EVENT_INVALID = 0,             /*!< Invalid event */
    LOW_CODE_EVENT_SETUP_MODE_START,        /*!< Setup mode started */
    LOW_CODE_EVENT_SETUP_MODE_END,          /*!< Setup mode ended */
    LOW_CODE_EVENT_SETUP_DEVICE_CONNECTED,  /*!< Device connected during setup */
    LOW_CODE_EVENT_SETUP_STARTED,           /*!< Device setup process started */
    LOW_CODE_EVENT_SETUP_SUCCESSFUL,        /*!< Device setup completed successfully */
    LOW_CODE_EVENT_SETUP_FAILED,            /*!< Device setup failed */
    LOW_CODE_EVENT_NETWORK_CONNECTED,       /*!< Device connected to network */
    LOW_CODE_EVENT_NETWORK_DISCONNECTED,    /*!< Device disconnected from network */
    LOW_CODE_EVENT_OTA_STARTED,             /*!< Over-the-air update started */
    LOW_CODE_EVENT_OTA_STOPPED,             /*!< Over-the-air update stopped */
    LOW_CODE_EVENT_READY,                   /*!< Device is ready for operation */
    LOW_CODE_EVENT_IDENTIFICATION_START,     /*!< Start device identification process */
    LOW_CODE_EVENT_IDENTIFICATION_STOP,      /*!< Stop device identification process */
    LOW_CODE_EVENT_IDENTIFICATION_BLINK,     /*!< Blink identification pattern */
    LOW_CODE_EVENT_IDENTIFICATION_BREATHE,   /*!< Breathe identification pattern */
    LOW_CODE_EVENT_IDENTIFICATION_OKAY,      /*!< Identification okay pattern */
    LOW_CODE_EVENT_IDENTIFICATION_CHANNEL_CHANGE, /*!< Channel change identification */
    LOW_CODE_EVENT_IDENTIFICATION_FINISH_EFFECT, /*!< Finish identification effect */
    LOW_CODE_EVENT_IDENTIFICATION_STOP_EFFECT,   /*!< Stop identification effect */
    LOW_CODE_EVENT_TEST_MODE_LOW_CODE,      /*!< Enter low code test mode */
    LOW_CODE_EVENT_TEST_MODE_COMMON,        /*!< Enter common test mode */
    LOW_CODE_EVENT_TEST_MODE_BLE,           /*!< Enter BLE test mode */
    LOW_CODE_EVENT_TEST_MODE_SNIFFER,       /*!< Enter sniffer test mode */
    LOW_CODE_EVENT_FACTORY_RESET,           /*!< Perform factory reset */
    LOW_CODE_EVENT_FORCED_ROLLBACK,         /*!< Force firmware rollback */
    LOW_CODE_EVENT_BLE_ADVERTISE,           /*!< Start BLE advertisement */
} low_code_event_type_t;

/**
 * @brief Structure to hold event data
 */
typedef struct low_code_event {
    low_code_event_type_t event_type;       /*!< Type of the event */
    int event_data_size;                    /*!< Size of event data */
    void *event_data;                       /*!< Pointer to event data */
} low_code_event_t;

/**
 * @brief Event callback function type
 * @param[in] event Pointer to the event structure
 * @return ESP_OK on success, appropriate error code otherwise
 */
typedef int (*low_code_event_callback_t)(low_code_event_t *event);

/**
 * @brief Feature update callback function type
 * @param[in] data Pointer to the feature data structure
 * @return ESP_OK on success, appropriate error code otherwise
 */
typedef int (*low_code_feature_update_callback_t)(low_code_feature_data_t *data);

/* Internal callback types */
typedef int (*low_code_get_event_from_system_t)();
typedef int (*low_code_get_feature_update_from_system_t)();

/**
 * @brief Structure containing all callback functions
 */
typedef struct low_code_callback_list {
    low_code_event_callback_t event_cb;     /*!< Event callback */
    low_code_feature_update_callback_t feature_update_cb; /*!< Feature update callback */
    low_code_get_event_from_system_t get_event; /*!< Get event callback */
    low_code_get_feature_update_from_system_t get_feature_update; /*!< Get feature update callback */
} __attribute__((packed)) low_code_callback_list_t;

/**
 * @brief Register transport layer callbacks
 * @param[in] callbacks Pointer to the callback list structure
 * @return ESP_OK on success, appropriate error code otherwise
 */
int low_code_register_transport_callbacks(low_code_callback_list_t *callbacks);

/**
 * @brief Process feature update from transport layer
 * @param[in] data Pointer to the feature data structure
 * @return ESP_OK on success, appropriate error code otherwise
 */
int low_code_feature_update_from_transport(low_code_feature_data_t *data);

/**
 * @brief Process event from transport layer
 * @param[in] event Pointer to the event structure
 * @return ESP_OK on success, appropriate error code otherwise
 */
int low_code_event_from_transport(low_code_event_t *event);

/**
 * @brief Register application callbacks
 * @param[in] feature_update_from_system Feature update callback function
 * @param[in] event_from_system Event callback function
 * @return ESP_OK on success, appropriate error code otherwise
 */
int low_code_register_callbacks(low_code_feature_update_callback_t feature_update_from_system, low_code_event_callback_t event_from_system);

/**
 * @brief Send feature update to system
 * @param[in] feature Pointer to the feature data structure
 * @return ESP_OK on success, appropriate error code otherwise
 */
int low_code_feature_update_to_system(low_code_feature_data_t *feature);

/**
 * @brief Send event to system
 * @param[in] event Pointer to the event structure
 * @return ESP_OK on success, appropriate error code otherwise
 */
int low_code_event_to_system(low_code_event_t *event);

/**
 * @brief Get feature update from system
 * @return ESP_OK on success, appropriate error code otherwise
 */
int low_code_get_feature_update_from_system();

/**
 * @brief Get event from system
 * @return ESP_OK on success, appropriate error code otherwise
 */
int low_code_get_event_from_system();
