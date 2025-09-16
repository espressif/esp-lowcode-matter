#include <stdbool.h>
#include <ulp_lp_core_gpio.h>
#include <soc/gpio_num.h>

#include "relay_driver.h"

void relay_driver_init(int gpio_num)
{ 
    ulp_lp_core_gpio_init(gpio_num);
    ulp_lp_core_gpio_output_enable(gpio_num);
    ulp_lp_core_gpio_input_disable(gpio_num);
    ulp_lp_core_gpio_set_output_mode(gpio_num, RTCIO_LL_OUTPUT_NORMAL);
 
}

void relay_driver_set_power(int gpio_num, bool power)
{
    ulp_lp_core_gpio_set_level(gpio_num, power);
}

