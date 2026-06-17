#include "stdint.h"
#include "stdlib.h"
#include "stdbool.h"

#include "sdkconfig.h"
#include "sw_timer.h"
#include "led_driver.h"
#include "ws2812_driver.h"
#include "bp5758_driver.h"
#include "light_driver.h"
#include "color_format.h"
#include "ulp_lp_core_print.h"

typedef  int (* light_dev_init_t) (void);
typedef void (* light_dev_deinit_t) (void);
typedef  int (* light_dev_set_channel_t) (uint8_t channel, uint8_t val); /* write(channel, val) */
typedef  int (* light_dev_get_channel_t) (uint8_t channel, uint8_t *val); /* not implemented */
typedef  int (* light_dev_update_channels_t) (void);    /* use device-specific internal buffer to update the status of device */
typedef  int (* light_dev_regist_channel_t) (uint8_t channel, gpio_num_t gpio);

// this is the common handle among ledc, iic or ws2812
typedef void *light_dev_handle_t;

typedef struct {
    light_dev_handle_t handle;
    light_dev_init_t init;
    light_dev_deinit_t deinit;
    light_dev_set_channel_t set_channel;
    light_dev_get_channel_t get_channel;
    light_dev_update_channels_t update_channels;
    light_dev_regist_channel_t regist_channel;
} light_dev_if_t;

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t cold;
    uint8_t warm;
    uint8_t brightness;
} light_channel_t;

typedef struct {
    light_effect_type_t type; /**< type of light effect */
    light_work_mode_t mode; /**< working mode of this effect */
    RGB_color_t RGB; /**< color for the effect */
    uint32_t cct; /**< color temperature of the effect */
    int8_t target_brightness; /**< target brightness */
    int8_t current_brightness;   /** current brightness */
    int8_t offset_brightness; /**< brightness offset */
    int8_t delta_brightness; /** brightness delta*/
    int speed;
    int total_ms;
    int rounds; /* used by effect timer handler */
    int effectStepTime;
    sw_timer_handle_t timer; /* timer */
} light_effect_t;

typedef struct {
    light_device_type_t dev_type;
    light_dev_if_t dev;
    light_work_mode_t work_mode;
    light_channel_comb_t channel_comb;
    light_channel_t channel;
    uint8_t min_brightness;
    uint8_t max_brightness;
    uint8_t cur_brightness;
    uint8_t cur_level;
    HS_color_t cur_hs; /* current hue & saturation */
    uint32_t cur_cct; /* current temperature */
    light_effect_t cur_effect; /* current effect */
} light_driver_t;

static light_driver_t g_light;
static light_effect_config_t g_light_effect_config;

#ifdef CONFIG_USE_LIGHT_DEVICE_TYPE_LED
static void light_driver_device_led_init(void)
{
    g_light.dev.init = led_driver_init;
    g_light.dev.deinit = led_driver_deinit;
    g_light.dev.set_channel = led_driver_set_channel;
    g_light.dev.get_channel = (light_dev_get_channel_t)(NULL);
    g_light.dev.update_channels = led_driver_update_channels;
    g_light.dev.regist_channel = led_driver_regist_channel;
}
#endif

#ifdef CONFIG_USE_LIGHT_DEVICE_TYPE_WS2812
static void light_driver_device_ws2812_init(void) {
    g_light.dev.init = ws2812_driver_init;
    g_light.dev.deinit = ws2812_driver_deinit;
    g_light.dev.set_channel = ws2812_driver_set_channel;
    g_light.dev.get_channel = (light_dev_get_channel_t)(NULL);
    g_light.dev.update_channels = ws2812_driver_update_channels;
    g_light.dev.regist_channel = ws2812_driver_regist_channel;
}
#endif

#ifdef CONFIG_USE_LIGHT_DEVICE_TYPE_BP5758
static void light_driver_device_bp5758_init(void) {
    g_light.dev.init = bp5758_driver_init;
    g_light.dev.deinit = bp5758_driver_deinit;
    g_light.dev.set_channel = bp5758_driver_set_channel;
    g_light.dev.get_channel = (light_dev_get_channel_t)(NULL);
    g_light.dev.update_channels = bp5758_driver_update_channels;
    g_light.dev.regist_channel = bp5758_driver_regist_channel;
}
#endif

int light_driver_init(light_driver_config_t *config)
{
    if (config->max_brightness > 100 || config->min_brightness < 0 || config->min_brightness > config->max_brightness) {
        printf("Invalid brightness: max=%d, min=%d\n", config->max_brightness, config->min_brightness);
        return -1;
    }

    if (config->channel_comb <= LIGHT_CHANNEL_COMB_INVALID || config->channel_comb >= LIGHT_CHANNEL_COMB_MAX) {
        printf("Invalid light channel combination: %d\n", config->channel_comb);
        return -1;
    }

    g_light.channel_comb = config->channel_comb;
    g_light.dev_type = config->device_type;
    g_light.max_brightness = config->max_brightness;
    g_light.min_brightness = config->min_brightness;
    g_light.cur_effect.timer = NULL;

   /* guess from CH settings */
    switch (config->channel_comb) {
    case LIGHT_CHANNEL_COMB_1CH_C:
    case LIGHT_CHANNEL_COMB_1CH_W:
    case LIGHT_CHANNEL_COMB_2CH_CW:
        g_light.work_mode = LIGHT_WORK_MODE_WHITE;
        break;
    case LIGHT_CHANNEL_COMB_3CH_RGB:
    case LIGHT_CHANNEL_COMB_5CH_RGBCW:
        g_light.work_mode = LIGHT_WORK_MODE_COLOR;
        break;
    default:
        break;
    }

    switch (config->device_type) {
    #if CONFIG_USE_LIGHT_DEVICE_TYPE_WS2812
    case LIGHT_DEVICE_TYPE_WS2812:
        printf("Light: WS2812\n");
        light_driver_device_ws2812_init();
        if (g_light.dev.init() != 0) {
            printf("Failed to init device\n");
            g_light.dev.deinit();
        }
        g_light.dev.regist_channel(LIGHT_CHANNEL_COMB_INVALID, config->io_conf.ws2812_io.ctrl_io);
        g_light.channel_comb = LIGHT_CHANNEL_COMB_3CH_RGB;
        g_light.channel.red = WS2812_CHANNEL_RED;
        g_light.channel.green = WS2812_CHANNEL_GREEN;
        g_light.channel.blue = WS2812_CHANNEL_BLUE;
        g_light.channel.cold = WS2812_CHANNEL_COLD;
        g_light.channel.warm = WS2812_CHANNEL_WARM;
        g_light.channel.brightness = WS2812_CHANNEL_BRIGHTNESS;
    break;
    #endif
    #ifdef CONFIG_USE_LIGHT_DEVICE_TYPE_LED
    case LIGHT_DEVICE_TYPE_LED:
        printf("Light: LED\n");
        light_driver_device_led_init();

        if (g_light.dev.init() != 0) {
            printf("Failed to init device\n");
            g_light.dev.deinit();
        }

        switch (config->channel_comb) {
        case LIGHT_CHANNEL_COMB_1CH_C:
            g_light.channel.cold = LED_CHANNEL_COLD;
            g_light.dev.regist_channel(LED_CHANNEL_COLD, config->io_conf.led_io.cold);
            break;
        case LIGHT_CHANNEL_COMB_1CH_W:
            g_light.channel.warm = LED_CHANNEL_WARM;
            g_light.dev.regist_channel(LED_CHANNEL_WARM, config->io_conf.led_io.warm);
            break;
        case LIGHT_CHANNEL_COMB_2CH_CW:
            g_light.channel.cold = LED_CHANNEL_COLD;
            g_light.channel.warm = LED_CHANNEL_WARM;
            g_light.dev.regist_channel(LED_CHANNEL_COLD, config->io_conf.led_io.cold);
            g_light.dev.regist_channel(LED_CHANNEL_WARM, config->io_conf.led_io.warm);
            break;
        case LIGHT_CHANNEL_COMB_3CH_RGB:
            g_light.channel.red = LED_CHANNEL_RED;
            g_light.channel.green = LED_CHANNEL_GREEN;
            g_light.channel.blue = LED_CHANNEL_BLUE;
            g_light.dev.regist_channel(LED_CHANNEL_RED, config->io_conf.led_io.red);
            g_light.dev.regist_channel(LED_CHANNEL_GREEN, config->io_conf.led_io.green);
            g_light.dev.regist_channel(LED_CHANNEL_BLUE, config->io_conf.led_io.blue);
            break;
        case LIGHT_CHANNEL_COMB_5CH_RGBCW:
            g_light.channel.red = LED_CHANNEL_RED;
            g_light.channel.green = LED_CHANNEL_GREEN;
            g_light.channel.blue = LED_CHANNEL_BLUE;
            g_light.channel.cold = LED_CHANNEL_COLD;
            g_light.channel.warm = LED_CHANNEL_WARM;
            g_light.dev.regist_channel(LED_CHANNEL_RED, config->io_conf.led_io.red);
            g_light.dev.regist_channel(LED_CHANNEL_GREEN, config->io_conf.led_io.green);
            g_light.dev.regist_channel(LED_CHANNEL_BLUE, config->io_conf.led_io.blue);
            g_light.dev.regist_channel(LED_CHANNEL_COLD, config->io_conf.led_io.cold);
            g_light.dev.regist_channel(LED_CHANNEL_WARM, config->io_conf.led_io.warm);
            break;
        default:
            printf("Unsupported channel setting\n");
            break;
        }
        break;
    #endif
    #ifdef CONFIG_USE_LIGHT_DEVICE_TYPE_BP5758
    case LIGHT_DEVICE_TYPE_BP5758:
        printf("Light: BP5758\n");
        light_driver_device_bp5758_init();

        /* Register I2C pins first before init */
        g_light.dev.regist_channel(0, config->io_conf.bp5758_io.sda_io);
        g_light.dev.regist_channel(1, config->io_conf.bp5758_io.scl_io);

        if (g_light.dev.init() != 0) {
            printf("Failed to init device\n");
            g_light.dev.deinit();
        }

        switch (config->channel_comb) {
        case LIGHT_CHANNEL_COMB_1CH_C:
            g_light.channel.cold = BP5758_CHANNEL_COLD;
            break;
        case LIGHT_CHANNEL_COMB_1CH_W:
            g_light.channel.warm = BP5758_CHANNEL_WARM;
            break;
        case LIGHT_CHANNEL_COMB_2CH_CW:
            g_light.channel.cold = BP5758_CHANNEL_COLD;
            g_light.channel.warm = BP5758_CHANNEL_WARM;
            break;
        case LIGHT_CHANNEL_COMB_3CH_RGB:
            g_light.channel.red = BP5758_CHANNEL_RED;
            g_light.channel.green = BP5758_CHANNEL_GREEN;
            g_light.channel.blue = BP5758_CHANNEL_BLUE;
            break;
        case LIGHT_CHANNEL_COMB_5CH_RGBCW:
            g_light.channel.red = BP5758_CHANNEL_RED;
            g_light.channel.green = BP5758_CHANNEL_GREEN;
            g_light.channel.blue = BP5758_CHANNEL_BLUE;
            g_light.channel.cold = BP5758_CHANNEL_COLD;
            g_light.channel.warm = BP5758_CHANNEL_WARM;
            break;
        default:
            printf("Unsupported channel setting\n");
            break;
        }
        break;
    #endif
    default:
        printf("Invalid device\n");
        break;
    }

    return 0;
}

void light_driver_deinit(void)
{
    g_light.dev.deinit();
}

/**
 * @brief
 * when switching from color mode to white mode number of LED beads changes from 3 to 2, the total intensity is lower
 * followings are very simple placeholder functions to scale down the channel output to avoid intensity jitter when
 * work mode switch from COLOR to WHITE
 *
 * @param: scale [0, 100] light intensity
*/
/*
 * Notice, the scale of number of LED beads has been moved to the place where these two functions are called so that WS2812 and LED driver can both work as expected.
*/
static void process_color_limit(RGB_color_t *RGB_dst, RGB_color_t *RGB_src, uint8_t scale)
{
    /* NOTE: RGB range: [0, 255] */
    RGB_dst->red = RGB_src->red * scale / 255;
    RGB_dst->blue = RGB_src->blue * scale / 255;
    RGB_dst->green = RGB_src->green * scale / 255;
}

static void process_cct_limit(CW_white_t *CW_dst, CW_white_t *CW_src, uint8_t scale)
{
    /* NOTE: CW range: [0, 100] */
    CW_dst->cold = CW_src->cold * scale / 100;
    CW_dst->warm = CW_src->warm * scale / 100;
}

/**
 * light_driver_update() is the single place to take effect of previous changes to g_light.cur_xxx
 * light_driver_set_xxx() function simply change g_light.cur_xxx, but light_driver_update() update the underlying device
 * g_light.cur_xxx are un-normalized values, and light_driver_update() should process channel limits first and then write to device
*/
static int light_driver_update(void)
{
    switch (g_light.work_mode) {
        case LIGHT_WORK_MODE_COLOR:
            switch (g_light.channel_comb) {
                case LIGHT_CHANNEL_COMB_3CH_RGB:
                case LIGHT_CHANNEL_COMB_5CH_RGBCW:
                    RGB_color_t RGB = {0};
                    if (g_light.cur_level) {
                        // NOTE: hsv_to_rgb has scale down the RGB based on cur_brightness, process_color_limit should not do again
                        hsv_to_rgb(g_light.cur_hs, g_light.cur_brightness, &RGB);
#ifdef CONFIG_USE_LIGHT_DEVICE_TYPE_LED
                        process_color_limit(&RGB, &RGB, 100 / 3);
#endif
                    }
                    /* write to device */
                    g_light.dev.set_channel(g_light.channel.red, RGB.red);
                    g_light.dev.set_channel(g_light.channel.green, RGB.green);
                    g_light.dev.set_channel(g_light.channel.blue, RGB.blue);
                    break;
                default:
                    printf("%s:%d: Incompatible work mode %d with channel comb %d\n", __func__, __LINE__, g_light.work_mode, g_light.channel_comb);
                    break;
            }
            break;
        case LIGHT_WORK_MODE_WHITE:
            int brightness = 0;
            switch (g_light.channel_comb) {
                case LIGHT_CHANNEL_COMB_1CH_C:
                    if (g_light.cur_level) {
                        brightness = g_light.cur_brightness;
                    }
#ifdef CONFIG_USE_LIGHT_DEVICE_TYPE_WS2812
                g_light.dev.set_channel(g_light.channel.brightness, brightness);
#endif
                g_light.dev.set_channel(g_light.channel.cold, brightness);
                break;
                case LIGHT_CHANNEL_COMB_1CH_W:
                    if (g_light.cur_level) {
                        brightness = g_light.cur_brightness;
                    }
#ifdef CONFIG_USE_LIGHT_DEVICE_TYPE_WS2812
                g_light.dev.set_channel(g_light.channel.brightness, brightness);
#endif
                g_light.dev.set_channel(g_light.channel.warm, brightness);
                break;
                case LIGHT_CHANNEL_COMB_2CH_CW:
                case LIGHT_CHANNEL_COMB_5CH_RGBCW:
                    CW_white_t CW = {0};
                    if (g_light.cur_level) {
                        temp_to_cw(g_light.cur_cct, &CW);
                        brightness = g_light.cur_brightness;
#ifdef CONFIG_USE_LIGHT_DEVICE_TYPE_LED
                        process_cct_limit(&CW, &CW, g_light.cur_brightness / 2);
#endif
                    }
#ifdef CONFIG_USE_LIGHT_DEVICE_TYPE_WS2812
                g_light.dev.set_channel(g_light.channel.brightness, brightness);
#endif
                g_light.dev.set_channel(g_light.channel.cold, CW.cold);
                g_light.dev.set_channel(g_light.channel.warm, CW.warm);
                break;
                default:
                    printf("%s:%d: Incompatible work mode %d with channel comb %d\n", __func__, __LINE__, g_light.work_mode, g_light.channel_comb);
                    break;
            }
            break;
        default:
            break;
    }
    return g_light.dev.update_channels();
}

int light_driver_set_brightness(uint8_t val)
{
    printf("%s(%d)\n", __func__, val);

    g_light.cur_brightness = val;
    return light_driver_update();
}

int light_driver_set_power(uint8_t val)
{
    printf("%s:(%d)\n", __func__, val);
    g_light.cur_level = val;
    return light_driver_update();
}

int light_driver_set_hue(uint16_t val)
{
    printf("%s(%d)\n", __func__, val);

    if (g_light.channel_comb == LIGHT_CHANNEL_COMB_1CH_C
                || g_light.channel_comb == LIGHT_CHANNEL_COMB_1CH_W
                || g_light.channel_comb == LIGHT_CHANNEL_COMB_2CH_CW) {
        printf("%s: hue not supported by %d\n", __func__, g_light.channel_comb);
        return -1;
    }

    g_light.cur_hs.hue = val;
    return light_driver_update();
}

int light_driver_set_saturation(uint8_t val)
{
    printf("%s(%d)\n", __func__, val);
    if (g_light.channel_comb == LIGHT_CHANNEL_COMB_1CH_C
                || g_light.channel_comb == LIGHT_CHANNEL_COMB_1CH_W
                || g_light.channel_comb == LIGHT_CHANNEL_COMB_2CH_CW) {
        printf("%s: saturation not supported by %d\n", __func__, g_light.channel_comb);
        return -1;
    }

    g_light.cur_hs.saturation = val;
    return light_driver_update();
}

int light_driver_set_temperature(uint32_t val)
{
    if (g_light.channel_comb == LIGHT_CHANNEL_COMB_1CH_C
                || g_light.channel_comb == LIGHT_CHANNEL_COMB_1CH_W) {
        printf("%s: temp not supported by %d\n", __func__, g_light.channel_comb);
        return -1;
    }

    g_light.cur_cct = val;
    return light_driver_update();
}

int light_driver_set_color_mode(uint8_t val)
{
    int ret = 0;
    printf("%s(%d)\n", __func__, val);

    if (val != LIGHT_WORK_MODE_COLOR && val != LIGHT_WORK_MODE_WHITE) {
        printf("%s: Unrecognized work mode\n", __func__);
        return -1;
    }

    if (val == LIGHT_WORK_MODE_COLOR) {
        if (g_light.channel_comb == LIGHT_CHANNEL_COMB_1CH_C
                    || g_light.channel_comb == LIGHT_CHANNEL_COMB_1CH_W
                    || g_light.channel_comb == LIGHT_CHANNEL_COMB_2CH_CW) {
            printf("%s: temp not supported by work mode %d\n", __func__, g_light.channel_comb);
            return -1;
        }
    }

    if (val == LIGHT_WORK_MODE_WHITE) {
        if (g_light.channel_comb == LIGHT_CHANNEL_COMB_1CH_C
                    || g_light.channel_comb == LIGHT_CHANNEL_COMB_1CH_W) {
            printf("%s: temp not supported by work mode %d\n", __func__, g_light.channel_comb);
            return -1;
        }
    }

    /* first turn off light */
    uint8_t old_level = g_light.cur_level;
    g_light.cur_level = 0;
    ret |= light_driver_update();

    /* then change color mode and turn on light */
    g_light.work_mode = val;
    g_light.cur_level = old_level;
    ret |= light_driver_update();

    return ret;
}

static void light_effect_handler(sw_timer_handle_t timer_handle, void *arg)
{
    switch (g_light.cur_effect.type) {
    case LIGHT_EFFECT_BLINK:
        g_light.cur_brightness = g_light.cur_effect.rounds % 2 ? g_light.cur_effect.target_brightness + g_light.cur_effect.offset_brightness : g_light.cur_effect.offset_brightness;
        break;
    case LIGHT_EFFECT_BREATHE:
        g_light.cur_brightness = abs(g_light.cur_effect.current_brightness) + g_light.cur_effect.offset_brightness;
        if (g_light.cur_effect.current_brightness == g_light.cur_effect.target_brightness) {
            g_light.cur_effect.current_brightness = -g_light.cur_effect.target_brightness;
        } else {
            g_light.cur_effect.current_brightness += g_light.cur_effect.delta_brightness;
        }

        break;
    default:
        break;
    }
    light_driver_update();

    /* increase counter & check whether delete timer */
    g_light.cur_effect.rounds++;

    /* if total_ms == 0, repeat infinitely until timer is deleted */
    if (g_light.cur_effect.total_ms == 0) {
        return;
    }

    if (g_light.cur_effect.rounds * g_light.cur_effect.effectStepTime >= g_light.cur_effect.total_ms) {
        light_driver_effect_stop();
        printf("%s: end of effect\n", __func__);
    }
}


void light_driver_effect_start(light_effect_config_t *config, int speed, int total_ms)
{
    printf("%s(config, %d, %d)\n", __func__, speed, total_ms);

    g_light.cur_effect.target_brightness = abs(config->max_brightness - config->min_brightness);
    g_light.cur_effect.offset_brightness = abs(config->min_brightness);
    g_light.cur_effect.current_brightness = -g_light.cur_effect.target_brightness;
    // make sure the refresh rate is less than 40Hz, and at least 1 breath effect will be triggered
    int effectStepTime = speed / (2 * g_light.cur_effect.target_brightness);
    effectStepTime = effectStepTime >= 25 ? effectStepTime : 25;
    int effectStepCount = speed / (2 * effectStepTime);
    effectStepCount = effectStepCount ? effectStepCount : 1;
    g_light.cur_effect.delta_brightness =  g_light.cur_effect.target_brightness / effectStepCount;
    g_light.cur_effect.delta_brightness = g_light.cur_effect.delta_brightness ? g_light.cur_effect.delta_brightness : 1;

    // assign other configurations
    g_light.cur_effect.mode = config->mode;
    g_light.cur_effect.total_ms = total_ms;
    g_light.cur_effect.effectStepTime = effectStepTime;
    g_light.cur_effect.speed = speed;
    g_light.cur_effect.type = config->type;

    switch (config->mode) {
    case LIGHT_WORK_MODE_COLOR:
        g_light.cur_effect.RGB = config->color.RGB;
        // since in light_driver_update, we only use cur_hs & cur_cct to specify the color, so the conversion is necessary here.
        rgb2hs(g_light.cur_effect.RGB, &g_light.cur_hs);
        break;
    case LIGHT_WORK_MODE_WHITE:
        g_light.cur_effect.cct = config->color.cct;
        g_light.cur_cct = g_light.cur_effect.cct;
        break;
    default:
        break;
    }


    /* execute the first effect: set to max brightness */
    switch (g_light.cur_effect.type) {
    case LIGHT_EFFECT_BLINK:
        g_light.cur_brightness = 0;
        effectStepTime = speed / 2;
        // for blink, only turn on/off is required. Consequently, set step time to half of the speed
        g_light.cur_effect.effectStepTime = effectStepTime;
        break;
    case LIGHT_EFFECT_BREATHE:
        g_light.cur_brightness = 0;
        break;
    default:
        break;
    }

    // turn off the light before starting the effect
    light_driver_update();

    /* each time we start a new effect, stop the previous effect */
    sw_timer_delete(g_light.cur_effect.timer);

    printf("effect(type=%d, max=%d, min=%d, speed=%d, total_ms=%d, timer=%p)\n",
        g_light.cur_effect.type, g_light.cur_effect.target_brightness + g_light.cur_effect.offset_brightness, g_light.cur_effect.offset_brightness,
        g_light.cur_effect.speed, g_light.cur_effect.total_ms, g_light.cur_effect.timer);

    sw_timer_config_t timer_cfg = {
        .arg = NULL,
        .handler = light_effect_handler,
        .periodic = true,
        .timeout_ms = effectStepTime, /* change brightness every half period */
    };
    sw_timer_handle_t timer = sw_timer_create(&timer_cfg);

    /* timer */
    g_light.cur_effect.timer = timer;
    g_light.cur_effect.rounds = 0; /* reset effect rounds to 0 */
    g_light.cur_level = g_light.cur_level ? g_light.cur_level : 1;
    // start the effect
    sw_timer_start(g_light.cur_effect.timer);
}

void light_driver_effect_stop(void)
{
    /* set power also set brightness to cur_brightness */
    sw_timer_delete(g_light.cur_effect.timer);
    light_driver_set_power(g_light.cur_level);
}
