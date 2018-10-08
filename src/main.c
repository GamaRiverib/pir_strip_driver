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
#include "math.h"
#include "mgos.h"
#include "mgos_utils.h"
#include "mgos_time.h"
#include "mgos_timers.h"
#include "mgos_gpio.h"
#include "mgos_adc.h"
#include "mgos_rpc.h"
#include "mgos_mqtt.h"
#include "mgos_neopixel.h"
#include "mgos_blynk.h"
#include "nvk_nodes.h"
#include "nvk_nodes_pir.h"

#define ORDER MGOS_NEOPIXEL_ORDER_GRB
#define CYLON_SIZE 1
#define TOTAL_EFFECTS 12

#define SNOW_EFFECT_INDEX 11

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

static struct mgos_neopixel *s_strip = NULL;

static mgos_timer_id effect_timer = MGOS_INVALID_TIMER_ID;
static mgos_timer_id smooth_timer = MGOS_INVALID_TIMER_ID;
static mgos_timer_id alert_timer = MGOS_INVALID_TIMER_ID;
static float last_motion_time = 0;
static int smooth_brightness = 0;

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

static void set_strip_rgb_color(rgb_color c) {
  int num_pixels = mgos_sys_config_get_strip_pixels();
  mgos_neopixel_clear(s_strip);
  for(int p = 0; p < num_pixels; p++) {
    mgos_neopixel_set(s_strip, p, c.red, c.green, c.blue);
  }
  mgos_neopixel_show(s_strip);
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

static void fade_effect() {
  static int s_fade_effect_counter = 0;
  static int s_fade_effect_iteration = 0;
  static bool s_fade_effect_dir = true;

  if(s_fade_effect_counter >= 3) {
    s_fade_effect_counter = 0;
  }

  if(s_fade_effect_dir) {
    s_fade_effect_dir = s_fade_effect_iteration < 254;
    s_fade_effect_iteration++;
  } else {
    s_fade_effect_dir = s_fade_effect_iteration < 2;
    s_fade_effect_iteration--;
    if(s_fade_effect_dir) {
      s_fade_effect_counter++;
    }
  }
  int color = s_fade_effect_iteration & 255;
  switch(s_fade_effect_counter) {
    case 0:
      color = color << 16;
      break;
    case 1:
      color = color << 8;
      break;
  }
  rgb_color c = get_rgb_color(color);
  set_strip_rgb_color(c);
}

static void flash_effect() {
  static int s_flash_effect_counter = 0;
  static rgb_color s_flash_effect_colors[] = {
    { 0x00, 0xFF, 0x00 }, { 0x00, 0x00, 0xFF }, { 0xFF, 0x00, 0x00 },
    { 0xFF, 0xFF, 0x00 }, { 0x00, 0xFF, 0xFF }, { 0x71, 0xDC, 0x94 },
    { 0xFF, 0x7F, 0x00 }, { 0x94, 0xDC, 0x71 }, { 0xC0, 0xD9, 0xD9 },
    { 0xFF, 0x80, 0x00 }, { 0xBD, 0x8F, 0x8F }, { 0x7F, 0xFF, 0x00 },
    { 0x50, 0x30, 0x50 }, { 0x8D, 0x78, 0x24 }, { 0xFF, 0x6E, 0xC7 },
    { 0x4D, 0x4D, 0xFF }, { 0xF5, 0xCC, 0xB0 }, { 0x8D, 0x17, 0x17 },
    { 0xDE, 0x94, 0xFA }, { 0x8F, 0x24, 0x24 }, { 0xCC, 0x7F, 0x32 },
    { 0x9F, 0x9F, 0x5F }, { 0x32, 0x32, 0xCC }, { 0x71, 0xDC, 0xDC },
    { 0x6F, 0x43, 0x43 }, { 0x24, 0x6C, 0x8F }, { 0xD9, 0xD9, 0xC0 },
    { 0xB8, 0x73, 0x33 }, { 0xE3, 0x78, 0x33 }, { 0x85, 0x64, 0x64 }
  };

  if(s_flash_effect_counter >= 30)
  {
    s_flash_effect_counter = 0;
  }
  set_strip_rgb_color(s_flash_effect_colors[s_flash_effect_counter++]);
}

static void rbg_loop_effect() {
  static int s_rgb_loop_effect_counter = 0;
  static int s_rbg_loop_effect_iteration = 0;
  static bool s_rgb_loop_effect_dir = true;

  if (s_rbg_loop_effect_iteration > 3) {
    s_rbg_loop_effect_iteration = 0;
  }

  uint8_t k = s_rgb_loop_effect_counter & 255;
  if (s_rgb_loop_effect_dir)
  {
    s_rgb_loop_effect_dir = k < 254;
    s_rgb_loop_effect_counter++;
  } else {
    s_rgb_loop_effect_dir = k < 2;
    s_rgb_loop_effect_counter--;
    if(s_rgb_loop_effect_dir)
    {
      s_rbg_loop_effect_iteration++;
    }
  }
  int color = 0xFF0000;
  switch(s_rbg_loop_effect_iteration)
  {
    case 1:
      color = color >> 8;
      break;
    case 2:
      color = color >> 16;
      break;
  }
  rgb_color c = get_rgb_color(color);
  set_strip_rgb_color(c);
}

static void twinkle_effect() {
  static int s_twinkle_effect_counter = 0;

  int num_pixels = mgos_sys_config_get_strip_pixels();
  int color = mgos_sys_config_get_strip_color();
  rgb_color c = get_rgb_color(color);

  if (s_twinkle_effect_counter < num_pixels / 3) {
    int p = (int) mgos_rand_range(0, num_pixels - 1);
    mgos_neopixel_set(s_strip, p, c.red, c.green, c.blue);
    mgos_neopixel_show(s_strip);
    s_twinkle_effect_counter++;
  } else {
    s_twinkle_effect_counter = 0;
    rgb_color c = get_rgb_color(0);
    set_strip_rgb_color(c);
  }
}

static void twinkle_random_effect() {
  static int s_twinkle_random_effect_counter = 0;
  int num_pixels = mgos_sys_config_get_strip_pixels();

  if (s_twinkle_random_effect_counter < num_pixels / 3) {
    int p = (int) mgos_rand_range(0, num_pixels - 1);
    int r = (int) mgos_rand_range(0, 254);
    int g = (int) mgos_rand_range(0, 254);
    int b = (int) mgos_rand_range(0, 254);
    mgos_neopixel_set(s_strip, p, r, g, b);
    mgos_neopixel_show(s_strip);
    s_twinkle_random_effect_counter++;
  } else {
    s_twinkle_random_effect_counter = 0;
    rgb_color c = get_rgb_color(0);
    set_strip_rgb_color(c);
  }
}

static void fire_effect() {
  static int heat[30]; // TODO
  static int cooling = 90;
  static int sparking = 120;
  int cooldown;

  int num_pixels = mgos_sys_config_get_strip_pixels();

  for(int i = 0; i < num_pixels; i++) {
    cooldown = (int) mgos_rand_range(0, ((cooling * 10) / num_pixels) + 2);
    if(cooldown > heat[i]) {
      heat[i] = 0;
    } else {
      heat[i] = heat[i] - cooldown;
    }
  }

  for(int k = num_pixels - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  if((int) mgos_rand_range(0, 254) < sparking) {
    int y = (int) mgos_rand_range(0, 6);
    heat[y] = heat[y] + (int) mgos_rand_range(159, 254);
  }

  mgos_neopixel_clear(s_strip);
  for(int j = 0; j < num_pixels; j++) {
    int t192 = round((heat[j] / 255.0) * 191);
    int heatramp = t192 & 0x3F;
    heatramp <<= 2;
    heatramp = heatramp & 255;
    if(t192 > 0x80) {
      mgos_neopixel_set(s_strip, j, 255, 255, heatramp);
    } else if(t192 > 0x40) {
      mgos_neopixel_set(s_strip, j, 255, heatramp, 0);
    } else {
      mgos_neopixel_set(s_strip, j, heatramp, 0, 0);
    }
  }

  mgos_neopixel_show(s_strip);
}

static void snow_effect_cb() {
  rgb_color c = get_rgb_color(0x101010);
  set_strip_rgb_color(c);
  mgos_neopixel_show(s_strip);
}

static void snow_effect() {
  if(mgos_sys_config_get_strip_effect() == SNOW_EFFECT_INDEX && mgos_sys_config_get_app_mode() == MODE_EFFECT) {
    rgb_color c = get_rgb_color(0x101010);
    set_strip_rgb_color(c);
    int num_pixels = mgos_sys_config_get_strip_pixels();
    int p = (int) mgos_rand_range(0, num_pixels - 1);
    mgos_neopixel_set(s_strip, p, 0xFF, 0xFF, 0xFF);
    mgos_neopixel_show(s_strip);
    mgos_set_timer(20, false, snow_effect_cb, NULL);
    int d = (int) mgos_rand_range(120, 1000);
    mgos_set_timer(d, false, snow_effect, NULL);
  }
}

static void start_effect() {
  clear_timers();
  int effect = mgos_sys_config_get_strip_effect();
  int speed = 200; // TODO: config speed
  switch(effect) {
    case 0:
      effect_timer = mgos_set_timer(speed / 4 * 3, MGOS_TIMER_REPEAT, first_effect, NULL);
      LOG(LL_INFO, ("Starting first effect..."));
      break;
    case 1:
      effect_timer = mgos_set_timer(speed, MGOS_TIMER_REPEAT, strobe_effect, NULL);
      LOG(LL_INFO, ("Starting strobe effect..."));
      break;
    case 2:
      effect_timer = mgos_set_timer(speed / 4 * 3, MGOS_TIMER_REPEAT, cylon_effect, NULL);
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
    case 5:
      effect_timer = mgos_set_timer(speed / 4, MGOS_TIMER_REPEAT, fade_effect, NULL);
      LOG(LL_INFO, ("Starting fade effect..."));
      break;
    case 6:
      effect_timer = mgos_set_timer(speed * 3, MGOS_TIMER_REPEAT, flash_effect, NULL);
      LOG(LL_INFO, ("Starting flash effect..."));
      break;
    case 7:
      effect_timer = mgos_set_timer(speed / 5, MGOS_TIMER_REPEAT, rbg_loop_effect, NULL);
      LOG(LL_INFO, ("Starting RGB loop effect..."));
      break;
    case 8:
      set_strip_rgb_color(get_rgb_color(0));
      effect_timer = mgos_set_timer(speed, MGOS_TIMER_REPEAT, twinkle_effect, NULL);
      LOG(LL_INFO, ("Starting twinkle effect..."));
      break;
    case 9:
      set_strip_rgb_color(get_rgb_color(0));
      effect_timer = mgos_set_timer(speed, MGOS_TIMER_REPEAT, twinkle_random_effect, NULL);
      LOG(LL_INFO, ("Starting twinkle random effect..."));
      break;
    case 10:
      effect_timer = mgos_set_timer(speed / 10, MGOS_TIMER_REPEAT, fire_effect, NULL);
      LOG(LL_INFO, ("Starting fire effect..."));
      break;
    case 11:
      snow_effect();
      LOG(LL_INFO, ("Starting snow effect..."));
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
  int speed = 100; // TODO
  effect_timer = mgos_set_timer(speed, MGOS_TIMER_REPEAT, cylon_effect, NULL);
  mgos_sys_config_set_app_mode(MODE_VIGILANCE);
}

/*
static void bled_timer_cb(void *args) {
  mgos_gpio_toggle(mgos_sys_config_get_pins_bled());
  (void) args;
}
*/

static void led_strip_brightness(int brightness) {
  brightness = brightness & 255;
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

void node_pir_toggle_handler(int value, void *user_data) {
    if(mgos_sys_config_get_pir_indicator()) {
      mgos_gpio_write(mgos_sys_config_get_pins_led(), value);
    }
    if(value) {
      motion_handler();
    }
    (void) user_data;
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

static void custom_blynk_handler(struct mg_connection *c, const char *cmd, int pin, int val, int id, void *user_data) {
  LOG(LL_INFO, ("custom_blynk_handler"));
  if (strcmp(cmd, "vr") == 0) {
    /*if (pin == s_read_virtual_pin) {
      blynk_virtual_write(c, id, s_read_virtual_pin,
                          (float) mgos_get_free_heap_size() / 1024);
    }*/
    switch(pin) {
      case 0:
        LOG(LL_INFO, ("Blynk::vr::Temp"));
        break;
      case 1:
        LOG(LL_INFO, ("Blynk::vr::Humd"));
        break;
      case 2:
        LOG(LL_INFO, ("Blynk::vr::Lum"));
        break;
      default:
        LOG(LL_INFO, ("Bad virtual read pin"));
    }
  } else if (strcmp(cmd, "vw") == 0) {
    /*if (pin == s_write_virtual_pin) {
      mgos_gpio_set_mode(s_led_pin, MGOS_GPIO_MODE_OUTPUT);
      mgos_gpio_write(s_led_pin, val);
    }*/
    switch(pin) {
      case 3:
        LOG(LL_INFO, ("Blynk::vw::Led"));
        break;
      case 4:
        LOG(LL_INFO, ("Blynk::vw::Mode"));
        break;
      case 5:
        LOG(LL_INFO, ("Blynk::vw::OnOff"));
        break;
      case 6:
        LOG(LL_INFO, ("Blynk::vw::Effect"));
        break;
      case 7:
        LOG(LL_INFO, ("Blynk::vw::Color"));
        break;
      default:
        LOG(LL_INFO, ("Bad virtual write pin"));
    }
  }
  (void) c;
  (void) val;
  (void) id;
  (void) user_data;
}

enum mgos_app_init_result mgos_app_init(void) {

  if(nodes_init()) {
    LOG(LL_INFO, ("Nodes init succesfully."));

    node_pir_set_pir_toggle_handler(node_pir_toggle_handler, NULL);
  } else {
    LOG(LL_INFO, ("Nodes init error."));
  }

  // Configure Built in LED
  /*mgos_gpio_set_mode(mgos_sys_config_get_pins_bled(), MGOS_GPIO_MODE_OUTPUT);
  mgos_set_timer(1000, MGOS_TIMER_REPEAT, bled_timer_cb, NULL);*/

  // Configure LED indicator for PIR sensor 
  mgos_gpio_set_mode(mgos_sys_config_get_pins_led(), MGOS_GPIO_MODE_OUTPUT);

  // Configure Led Strip WS2812
  int num_pixels = mgos_sys_config_get_strip_pixels();
  s_strip = mgos_neopixel_create(mgos_sys_config_get_pins_strip(), num_pixels, ORDER);

  // Configure RPC interface
  mgos_rpc_add_handler("Driver.On", rpc_turn_on_cb, NULL);
  mgos_rpc_add_handler("Driver.Off", rpc_turn_off_cb, NULL);
  mgos_rpc_add_handler("Driver.Effect", rpc_set_effect_cb, NULL);
  mgos_rpc_add_handler("Driver.Next", rpc_set_next_effect_cb, NULL);
  mgos_rpc_add_handler("Driver.Night", rpc_set_night_light_cb, NULL);
  mgos_rpc_add_handler("Driver.Vigilance", rpc_set_vigilance_cb, NULL);

  if (mgos_blynk_init()) {
    LOG(LL_INFO, ("Blynk init success"));
    blynk_set_handler(custom_blynk_handler, NULL);
  } else {
    LOG(LL_INFO, ("Failed to init Blynk"));
  }

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
