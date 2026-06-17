# Light (BP5758 Driver) | CW

## Description

A tunable white light featuring dual-channel LED control using the BP5758 I2C LED driver with the following capabilities:

* **I2C-Based Control**: Uses BP5758 5-channel constant current LED driver via I2C
* **Dual-Channel Control**: Independently manages cold and warm white LED channels
* **Light Management**:
  * On/Off state control
  * Brightness control
  * Adjustable color temperature
* **Device Status Indicator**: Provides system status indication through light effects
* **Matter Data Model Specification**:
  * **Device Type**: `Color Temperature Light`

## Hardware Configuration

The following hardware components are recommended for this product:

* **Devkit**: [M5Stack Nano C6 Dev Kit](https://shop.m5stack.com/products/m5stack-nanoc6-dev-kit?srsltid=AfmBOooXsbm_fgpDyK1yWqgPOwtjrL3WksxGlhmRKDZFmVj2omLLbWDX) or any ESP32-C6 board
* **LED Driver**: BP5758 5-channel I2C constant current LED driver
* **LED**: Dual-channel constant current LED (cold white + warm white)
* **Optional**: Push-button for manual control (not implemented in current version)

### About BP5758

BP5758 is a 5-channel constant current LED driver IC that communicates via I2C interface. Key features:
* I2C address: 0x15
* 5 independent output channels
* Each channel supports up to 90mA output current
* Suitable for RGB, CW, or RGBCW LED applications
* Low power consumption

### Pin Assignment

| Peripheral | GPIO Pin | Function           |
|------------|----------|-------------------|
| I2C SDA    | GPIO6    | I2C data line     |
| I2C SCL    | GPIO7    | I2C clock line    |

> **Note**: GPIO assignments can be customized by modifying the following macros in **app_driver.cpp**:
> `I2C_SDA_IO`, `I2C_SCL_IO`

### BP5758 Channel Mapping

For CW (Cold/Warm) configuration, the driver uses:
* Channel 3: Cold white LED
* Channel 4: Warm white LED

The BP5758 supports up to 5 channels, so this product can be extended to support:
* RGB mode (channels 0, 1, 2)
* RGBCW mode (all 5 channels)

> **Implementation Note**: The current driver implementation uses a simplified approach by controlling brightness through current adjustment. For production use with actual BP5758 hardware, you may need to refer to the BP5758 datasheet and adjust the register addresses and command sequences to match the specific chip variant you're using. The driver is designed to be easily adaptable to the actual hardware protocol.

## Understanding Code

### Initialization Sequence

The `app_driver_init()` function performs the following:

* Configures I2C communication with BP5758
* Initializes the light driver with:
  * Device type: `LIGHT_DEVICE_TYPE_BP5758`
  * Channel combination: `LIGHT_CHANNEL_COMB_2CH_CW`
  * Brightness range: 0-100%
* Sets default values:
  * Color temperature: 4000K
  * Brightness: 100%
  * Power state: ON

### Core Functions

* **Light Control**:
  * `app_driver_set_light_state`: Controls power state (true=ON, false=OFF)
  * `app_driver_set_light_brightness`: Sets brightness level (0-255 mapped to 0-100%)
  * `app_driver_set_light_temperature`: Adjusts color temperature (mireds value). The light driver uses Kelvin for temperature control, but the system reports the value in Mireds. Table for Kelvin to Mireds conversion:
    * **154 Mireds** → (1000000 / 154) Kelvin → **6500 K**
    * **250 Mireds** → (1000000 / 250) Kelvin → **4000 K**
    * **370 Mireds** → (1000000 / 370) Kelvin → **2700 K**

* **Visual Indicators**:
  * `LOW_CODE_EVENT_SETUP_MODE_START`: starts blinking effect, to indicate setup mode activation (2000ms interval)
  * `LOW_CODE_EVENT_SETUP_MODE_END`: stops blinking effect, to indicate setup mode has ended
  * `LOW_CODE_EVENT_READY`: displays full brightness white light to indicate device is ready

### I2C Communication

The BP5758 driver uses I2C to communicate with the LED driver chip:
* Uses the I2C master driver from the `drivers/i2c` component
* Default I2C port: I2C_NUM_0
* I2C address: 0x15 (7-bit addressing)
* Communication timeout: Set to wait indefinitely (-1)

### Extending Functionality

To add physical button functionality:

* Initialize GPIO button(s) in `app_driver_init()`
* Implement callbacks for:
  * Single press: Toggle power state
  * Long press: Factory reset the device
* Register callbacks using `button_driver_register_cb`

To extend to RGB or RGBCW mode:

* Change `channel_comb` to `LIGHT_CHANNEL_COMB_3CH_RGB` or `LIGHT_CHANNEL_COMB_5CH_RGBCW`
* Update `app_priv.h` to add hue and saturation control functions
* Implement `app_driver_set_light_hue()` and `app_driver_set_light_saturation()`
* Update `app_main.cpp` to handle RGB feature IDs

To set different light effects for different events:

* Extend `light_effect_config_t` in event handler
* Implement new effect types in `light_driver_effect_start`

### Hardware Setup

1. Connect the BP5758 chip to your ESP32-C6:
   * BP5758 SDA → ESP32-C6 GPIO6
   * BP5758 SCL → ESP32-C6 GPIO7
   * BP5758 VCC → 3.3V or 5V (depending on your setup)
   * BP5758 GND → GND

2. Connect your cold white and warm white LEDs to:
   * Cold white → BP5758 OUT4 (Channel 3)
   * Warm white → BP5758 OUT5 (Channel 4)

3. Ensure proper current limiting is configured on the BP5758

## Related Documentation

* [Light (PWM Driver) | CW](../light_cw_pwm/README.md)
* [Light (RMT Based) | RGBCW | WS2812](../light_rgbcw_ws2812/README.md)
* [Programmer's Model](../../docs/programmer_model.md)
* [Components](../../components/README.md)
* [Drivers](../../drivers/README.md)
* [Products](../README.md)
