/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mgos.h"
#include "mgos_time.h"
#include "mgos_timers.h"
#include "mgos_gpio.h"
#include "mgos_dht.h"
#include "mgos_adc.h"
#include "mgos_rpc.h"
#include "mgos_mqtt.h"
#include "mgos_neopixel.h"

#define ORDER MGOS_NEOPIXEL_ORDER_GRB
#define CYLON_SIZE 1
#define TOTAL_EFFECTS 5

#define MODE_OFF 0
#define MODE_ON 1
#define MODE_EFFECT 2
#define MODE_NIGHT 3
#define MODE_VIGILANCE 4

typedef struct {
  int red;
  int green;
  int blue;
} rgb_color;

static struct mgos_dht *s_dht = NULL;
static struct mgos_neopixel *s_strip = NULL;

static mgos_timer_id effect_timer = MGOS_INVALID_TIMER_ID;
static mgos_timer_id smooth_timer = MGOS_INVALID_TIMER_ID;
static mgos_timer_id alert_timer = MGOS_INVALID_TIMER_ID;
static float last_motion_time = 0;
static int smooth_brightness = 0;

const char WEATHER_JSON_FMT[] = "{temperature:%d,humidity:%d,luminosity:%d,uptime:%f}";
const char MOTION_ALERT_JSON_FMT[] = "{uptime:%f}";

// TODO: static void get_weather(char *)

static rgb_color get_rgb_color(int color) {
  rgb_color rgb = { 
    (color >> 16) & 0xFF, 
    (color >> 8) & 0xFF,
    color & 0xFF };
  return rgb;
}

static void clear_timers() {
  if(effect_timer != MGOS_INVALID_TIMER_ID) {
    mgos_clear_timer(effect_timer);
    effect_timer = MGOS_INVALID_TIMER_ID;
  }
  if(smooth_timer != MGOS_INVALID_TIMER_ID) {
    mgos_clear_timer(smooth_timer);
    smooth_timer = MGOS_INVALID_TIMER_ID;
  }
  if(alert_timer != MGOS_INVALID_TIMER_ID) {
    mgos_clear_timer(alert_timer);
    alert_timer = MGOS_INVALID_TIMER_ID;
  }
}

static void strip_turn_off() {
  clear_timers();
  mgos_neopixel_clear(s_strip);
  int num_pixels = mgos_sys_config_get_strip_pixels();
  for(int p = 0; p < num_pixels; p++) {
    mgos_neopixel_set(s_strip, p, 0, 0, 0);
  }
  mgos_neopixel_show(s_strip);
  mgos_sys_config_set_app_mode(MODE_OFF);
  LOG(LL_INFO, ("Led Strip Turn OFF"));
}

static void strip_turn_on() {
  clear_timers();
  int color = mgos_sys_config_get_strip_color();
  if(color == 0) {
    color = 0xFFFFFF;
  }
  rgb_color c = get_rgb_color(color);
  mgos_neopixel_clear(s_strip);
  int num_pixels = mgos_sys_config_get_strip_pixels();
  for(int p = 0; p < num_pixels; p++) {
    mgos_neopixel_set(s_strip, p, c.red, c.green, c.blue);
  }
  mgos_neopixel_show(s_strip);
  mgos_sys_config_set_app_mode(MODE_ON);
  LOG(LL_INFO, ("Led Strip Turn ON"));
}

static int get_luminosity() {
  if(mgos_adc_enable(0)) {
    return mgos_adc_read(0);
  }
  LOG(LL_INFO, ("ADC not available"));
  return 0;
}

static bool is_dark() {
  return get_luminosity() <= mgos_sys_config_get_pir_threshold();
}

static void first_effect() {
  static int s_first_effect_counter = 0;
  int num_pixels = mgos_sys_config_get_strip_pixels();
  int p = (s_first_effect_counter++) % num_pixels;
  int r = s_first_effect_counter % 255;
  int g = (s_first_effect_counter * 2) % 255;
  int b = (s_first_effect_counter * s_first_effect_counter) % 255;
  mgos_neopixel_clear(s_strip);
  mgos_neopixel_set(s_strip, p, r, g, b);
  mgos_neopixel_show(s_strip);
}

static void strobe_effect() {
  static bool s_strobe_effect_state = true;
  int num_pixels = mgos_sys_config_get_strip_pixels();
  int color = mgos_sys_config_get_strip_color();
  rgb_color c = get_rgb_color(color);
  mgos_neopixel_clear(s_strip);
  int p = 0;
  if(s_strobe_effect_state) {
    for(; p < num_pixels; p++) {
      mgos_neopixel_set(s_strip, p, c.red, c.green, c.blue);
    }
    s_strobe_effect_state = false;
  } else {
    for(;p < num_pixels; p++) {
      mgos_neopixel_set(s_strip, p, 0, 0, 0);
    }
    s_strobe_effect_state = true;
  }
  mgos_neopixel_show(s_strip);
}

static void cylon_effect() {
  static bool s_cylon_effect_dir = true;
  static int s_cylon_effect_counter = 0;
  int num_pixels = mgos_sys_config_get_strip_pixels();
  int color = mgos_sys_config_get_strip_color();
  rgb_color c = get_rgb_color(color);
  mgos_neopixel_clear(s_strip);
  int p = s_cylon_effect_counter;
  if(s_cylon_effect_dir) {
    s_cylon_effect_counter++;
    s_cylon_effect_dir = p < (num_pixels - CYLON_SIZE - 3); // TODO: 2?
  } else {
    s_cylon_effect_counter--;
    s_cylon_effect_dir = p <= 1;
  }
  for(int i = 0; i < num_pixels; i++) {
    mgos_neopixel_set(s_strip, i, 0, 0, 0);
  }
  mgos_neopixel_set(s_strip, p, c.red / 10, c.green / 10, c.blue / 10);
  for(int i = 1; i <= CYLON_SIZE; i++) {
    mgos_neopixel_set(s_strip, p + i, c.red, c.green, c.blue);
  }
  mgos_neopixel_set(s_strip, p + CYLON_SIZE + 1, c.red / 10, c.green / 10, c.blue / 10);
  mgos_neopixel_show(s_strip);
}

static int get_hex_color(int r, int g, int b) {
  int h = 0x000000;
  h |= r << 16;
  h |= g << 8;
  h |= b;
  return h;
}

static int wheel(int p) {
  p = 255 - p;
  if (p < 85) {
    return get_hex_color((255 - p * 3), 0, (p * 3));
  }
  if (p < 170) {
    p = p - 85;
    return get_hex_color(0, (p * 3), (255 - p * 3));
  }
  p = p - 170;
  return get_hex_color((p * 3), (255 - p * 3), 0);
}

static void rainbow_effect() {
  static int s_rainbow_effect_counter = 0;
  if(s_rainbow_effect_counter < 256) {
    int i = s_rainbow_effect_counter++;
    int num_pixels = mgos_sys_config_get_strip_pixels();
    mgos_neopixel_clear(s_strip);
    for(int p = 0; p < num_pixels; p++) {
      int color = wheel((p + i) & 255);
      rgb_color c = get_rgb_color(color);
      mgos_neopixel_set(s_strip, p, c.red, c.green, c.blue);
    }
    mgos_neopixel_show(s_strip);
  } else {
    s_rainbow_effect_counter = 0;
  }
}

static void rainbow_cycle_effect() {
  static int s_rainbow_cycle_effect_counter = 0;
  if(s_rainbow_cycle_effect_counter < 256 * 5) {
    int i = s_rainbow_cycle_effect_counter++;
    int num_pixels = mgos_sys_config_get_strip_pixels();
    mgos_neopixel_clear(s_strip);
    for(int p = 0; p < num_pixels; p++) {
      int k = p * 256 / num_pixels;
      int color = wheel((k + i) & 255);
      rgb_color c = get_rgb_color(color);
      mgos_neopixel_set(s_strip, p, c.red, c.green, c.blue);
    }
    mgos_neopixel_show(s_strip);
  } else {
    s_rainbow_cycle_effect_counter = 0;
  }
}

static void start_effect() {
  clear_timers();
  int effect = mgos_sys_config_get_strip_effect();
  int speed = 200; // TODO: config speed
  switch(effect) {
    case 0:
      effect_timer = mgos_set_timer(speed, MGOS_TIMER_REPEAT, first_effect, NULL);
      LOG(LL_INFO, ("Starting first effect..."));
      break;
    case 1:
      effect_timer = mgos_set_timer(speed, MGOS_TIMER_REPEAT, strobe_effect, NULL);
      LOG(LL_INFO, ("Starting strobe effect..."));
      break;
    case 2:
      effect_timer = mgos_set_timer(speed, MGOS_TIMER_REPEAT, cylon_effect, NULL);
      LOG(LL_INFO, ("Starting cylon effect..."));
      break;
    case 3:
      effect_timer = mgos_set_timer(speed, MGOS_TIMER_REPEAT, rainbow_effect, NULL);
      LOG(LL_INFO, ("Starting rainbow effect..."));
      break;
    case 4:
      effect_timer = mgos_set_timer(speed, MGOS_TIMER_REPEAT, rainbow_cycle_effect, NULL);
      LOG(LL_INFO, ("Starting rainbow cycle effect..."));
      break;
    default:
      LOG(LL_INFO, ("Bad effect: %d", effect));
      strip_turn_off();
  }
}

static void start_night_light() {
  clear_timers();
  mgos_neopixel_clear(s_strip);
  int num_pixels = mgos_sys_config_get_strip_pixels();
  for(int p = 0; p < num_pixels; p++) {
    mgos_neopixel_set(s_strip, p, 0, 0, 0);
  }
  mgos_neopixel_show(s_strip);
  mgos_sys_config_set_app_mode(MODE_NIGHT);
  LOG(LL_INFO, ("Starting night light"));
}

static void start_vigilance() {
  clear_timers();
  int speed = 200; // TODO
  effect_timer = mgos_set_timer(speed, MGOS_TIMER_REPEAT, cylon_effect, NULL);
  mgos_sys_config_set_app_mode(MODE_VIGILANCE);
}

/*
static void bled_timer_cb(void *args) {
  mgos_gpio_toggle(mgos_sys_config_get_pins_bled());
  (void) args;
}
*/

static void dht_timer_cb(void *dht) {
  float t = mgos_dht_get_temp(dht);
  float h = mgos_dht_get_humidity(dht);
  int l = get_luminosity();

  if (isnan(h) || isnan(t)) {
    LOG(LL_INFO, ("Failed to read data from sensor\n"));
    return;
  }
  LOG(LL_INFO, ("\nTemperature: %d *C \nHumidity: %d %%\nLuminosity: %d lux", (int)t, (int)h, l));

  mgos_mqtt_pubf("Weather", 1, false, WEATHER_JSON_FMT, (int)t, (int)h, l, mgos_uptime());

  (void) dht;
}

static void led_strip_brightness(int brightness) {
  LOG(LL_INFO, ("Set Led Strip Brightness: %d", brightness));
  mgos_neopixel_clear(s_strip);
  int num_pixels = mgos_sys_config_get_strip_pixels();
  for(int p = 0; p < num_pixels; p++) {
    mgos_neopixel_set(s_strip, p, brightness, brightness, brightness);
  }
  mgos_neopixel_show(s_strip);
}

static void smooth_turn_off() {
  smooth_brightness -= 50;
  if(smooth_brightness > 0) {
    led_strip_brightness(smooth_brightness);
  } else {
    smooth_brightness = 0;
    led_strip_brightness(0);
    clear_timers();
  }
}

static void check_last_motion_time() {
  int diff = mgos_uptime() - last_motion_time;
  if (diff < mgos_sys_config_get_pir_keep()) {
    int wait = mgos_sys_config_get_pir_keep() - diff;
    mgos_set_timer(wait, false, check_last_motion_time, NULL);
  } else {
    clear_timers();
    smooth_timer = mgos_set_timer(1000, MGOS_TIMER_REPEAT, smooth_turn_off, NULL);
  }
}

static void smooth_turn_on() {
  smooth_brightness += 50;
  if(smooth_brightness <= 250) {
    led_strip_brightness(smooth_brightness);
  } else {
    smooth_brightness = 250;
    led_strip_brightness(255);
    clear_timers();
    int wait = mgos_sys_config_get_pir_keep() * 1000;
    mgos_set_timer(wait, false, check_last_motion_time, NULL);
  }
}

static void motion_handler() {
  if (mgos_uptime() - last_motion_time > 4) {
    last_motion_time = mgos_uptime();
    LOG(LL_INFO, ("[%f] Motion detected", last_motion_time));
    switch(mgos_sys_config_get_app_mode()) {
      case MODE_NIGHT:
        if(is_dark() && smooth_timer == MGOS_INVALID_TIMER_ID) {
          smooth_turn_on();
          smooth_timer = mgos_set_timer(1000, MGOS_TIMER_REPEAT, smooth_turn_on, NULL);
        }
        break;
      case MODE_VIGILANCE:
        LOG(LL_INFO, ("ALERT"));
        if(alert_timer == MGOS_INVALID_TIMER_ID) {
          clear_timers();
          effect_timer = mgos_set_timer(100, MGOS_TIMER_REPEAT, strobe_effect, NULL);
          alert_timer = mgos_set_timer(15000, false, start_vigilance, NULL);
          mgos_mqtt_pubf("alert/motion", 1, false, MOTION_ALERT_JSON_FMT, mgos_uptime());
        }
        break;
      default:
        LOG(LL_INFO, ("Ignore motion"));
    }
  }
}

static void pir_timer_cb(void *args) {
  bool motion = mgos_gpio_read(mgos_sys_config_get_pins_pir());
  if(mgos_sys_config_get_pir_indicator()) {
    mgos_gpio_write(mgos_sys_config_get_pins_led(), motion);
  }
  if(motion) {
    motion_handler();
  }
  (void) args;
}

static void rpc_weather_cb(struct mg_rpc_request_info *ri, const char *args,
                           const char *src, void *dht) {
  float t = mgos_dht_get_temp(dht);
  float h = mgos_dht_get_humidity(dht);
  int l = get_luminosity();
  if(isnan(h) || isnan(t)) {
    mg_rpc_send_errorf(ri, -1, "{error: \"Failed to read data from sensor\"}");
  } else {
    LOG(LL_INFO, ("\nTemperature: %d *C \nHumidity: %d %%\nLuminosity: %d lux", (int)t, (int)h, l));
    mgos_mqtt_pubf("Weather", 1, false, WEATHER_JSON_FMT, (int)t, (int)h, l, mgos_uptime());
    mg_rpc_send_responsef(ri, WEATHER_JSON_FMT, (int)t, (int)h, l, mgos_uptime());
  }
  (void) args;
  (void) src;
}

static void rpc_turn_on_cb(struct mg_rpc_request_info *ri, const char *args,
                           const char *src, void *user_data) {
  strip_turn_on();
  mg_rpc_send_responsef(ri, "{success:true}");
  (void) args;
  (void) src;
  (void) user_data;
}

static void rpc_turn_off_cb(struct mg_rpc_request_info *ri, const char *args,
                           const char *src, void *user_data) {
  strip_turn_off();
  mg_rpc_send_responsef(ri, "{success:true}");
  (void) args;
  (void) src;
  (void) user_data;
}

static void rpc_set_effect_cb(struct mg_rpc_request_info *ri, const char *args,
                           const char *src, void *user_data) {
  int e = 0;
  LOG(LL_INFO, (args));
  if(json_scanf(args, strlen(args), ri->args_fmt, &e) == 1) {
    mgos_sys_config_set_strip_effect(e);
    mgos_sys_config_set_app_mode(MODE_EFFECT);
    start_effect();
    mg_rpc_send_responsef(ri, "{success:true, effect:%d}", e);
  } else {
    mg_rpc_send_errorf(ri, -1, "{success:false, message:\"Missing parameter\"}");
    LOG(LL_INFO, ("Bad parameter: %s", args));
  }
  (void) src;
  (void) user_data;
}

static void rpc_set_next_effect_cb(struct mg_rpc_request_info *ri, const char *args,
                           const char *src, void *user_data) {
  int e = mgos_sys_config_get_strip_effect();
  e++;
  if (e >= TOTAL_EFFECTS) {
    e = 0;
  }
  mgos_sys_config_set_strip_effect(e);
  mgos_sys_config_set_app_mode(MODE_EFFECT);
  start_effect();
  mg_rpc_send_responsef(ri, "{success:true, effect:%d}", e);
  (void) args;
  (void) src;
  (void) user_data;
}

static void rpc_set_night_light_cb(struct mg_rpc_request_info *ri, const char *args,
                           const char *src, void *user_data) {
  start_night_light();
  mg_rpc_send_responsef(ri, "{success:true}");
  (void) args;
  (void) src;
  (void) user_data;
}

static void rpc_set_vigilance_cb(struct mg_rpc_request_info *ri, const char *args,
                           const char *src, void *user_data) {
  start_vigilance();
  mg_rpc_send_responsef(ri, "{success:true}");
  (void) args;
  (void) src;
  (void) user_data;
}

enum mgos_app_init_result mgos_app_init(void) {

  // Configure Built in LED
  /* mgos_gpio_set_mode(mgos_sys_config_get_pins_bled(), MGOS_GPIO_MODE_OUTPUT);
  mgos_set_timer(1000, MGOS_TIMER_REPEAT, bled_timer_cb, NULL); */

  // Configure DHT sensor
  s_dht = mgos_dht_create(mgos_sys_config_get_pins_dht(), DHT11);
  mgos_set_timer(mgos_sys_config_get_app_tele() * 1000, MGOS_TIMER_REPEAT, dht_timer_cb, s_dht);

  // Configure PIR sensor
  mgos_gpio_set_mode(mgos_sys_config_get_pins_pir(), MGOS_GPIO_MODE_INPUT); // return bool
  mgos_gpio_set_mode(mgos_sys_config_get_pins_led(), MGOS_GPIO_MODE_OUTPUT);
  mgos_set_timer(500, MGOS_TIMER_REPEAT, pir_timer_cb, NULL);

  // Configure Led Strip WS2812
  int num_pixels = mgos_sys_config_get_strip_pixels();
  s_strip = mgos_neopixel_create(mgos_sys_config_get_pins_strip(), num_pixels, ORDER);
  // mgos_set_timer(150, MGOS_TIMER_REPEAT, strip_timer_cb, s_strip);

  // Configure RPC interface
  mgos_rpc_add_handler("Driver.Weather", rpc_weather_cb, s_dht);
  mgos_rpc_add_handler("Driver.On", rpc_turn_on_cb, NULL);
  mgos_rpc_add_handler("Driver.Off", rpc_turn_off_cb, NULL);
  mgos_rpc_add_handler("Driver.Effect", rpc_set_effect_cb, NULL);
  mgos_rpc_add_handler("Driver.Next", rpc_set_next_effect_cb, NULL);
  mgos_rpc_add_handler("Driver.Night", rpc_set_night_light_cb, NULL);
  mgos_rpc_add_handler("Driver.Vigilance", rpc_set_vigilance_cb, NULL);

  int mode = mgos_sys_config_get_app_mode();
  switch(mode) {
    case MODE_OFF:
      strip_turn_off();
      break;
    case MODE_ON:
      strip_turn_on();
      break;
    case MODE_EFFECT: 
      start_effect();
      break;
    case MODE_NIGHT:
      start_night_light();
      break;
    case MODE_VIGILANCE:
      start_vigilance();
      break;
    default:
      LOG(LL_INFO, ("Bad mode %d", mode));
      mgos_sys_config_set_app_mode(0);
      // TODO: Save config and reboot
  }

  return MGOS_APP_INIT_SUCCESS;
}
