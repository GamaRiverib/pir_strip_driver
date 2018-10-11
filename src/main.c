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
#include "mgos_rpc.h"
#include "mgos_mqtt.h"
#include "mgos_blynk.h"
#include "nvk_nodes.h"
#include "nvk_nodes_pir.h"
#include "nvk_nodes_photoresistor.h"
#include "nvk_nodes_neopixel.h"
#include "effect_default.h"
#include "effect_strobe.h"
#include "effect_cylon.h"
#include "effect_rainbow.h"
#include "effect_fade.h"
#include "effect_flash.h"
#include "effect_rgb_loop.h"
#include "effect_twinkle.h"
#include "effect_fire.h"
#include "effect_snow.h"
#include "effect_meteor.h"

#define TOTAL_EFFECTS 13

#define MODE_OFF 0
#define MODE_ON 1
#define MODE_EFFECT 2
#define MODE_NIGHT 3
#define MODE_VIGILANCE 4

static mgos_timer_id effect_timer = MGOS_INVALID_TIMER_ID;
static mgos_timer_id smooth_timer = MGOS_INVALID_TIMER_ID;
static mgos_timer_id alert_timer = MGOS_INVALID_TIMER_ID;
static float last_motion_time = 0;
static int smooth_brightness = 0;

static neopixel_effect_data s_neopixel_effect_data = { 0 };

const char MOTION_ALERT_JSON_FMT[] = "{uptime:%f}";

const char RPC_SUCCESS_RESPONSE_JSON_FMT[] = "{success:true}";

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
  node_neopixel_turn_off();
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
  node_neopixel_turn_on(c);
  mgos_sys_config_set_app_mode(MODE_ON);
  LOG(LL_INFO, ("Led Strip Turn ON"));
}

static bool is_dark() {
  return node_photoresistor_get_luminosity() <= mgos_sys_config_get_pir_threshold();
}

static void start_effect() {
  clear_timers();
  int effect = mgos_sys_config_get_strip_effect();
  int speed = mgos_sys_config_get_strip_speed();
  char *effect_name;
  switch(effect) {
    case 0:
      effect_name = "default";
      effect_timer = mgos_set_timer(speed / 4 * 3, MGOS_TIMER_REPEAT, first_effect, NULL);
      break;
    case 1:
      effect_name = "strobe";
      s_neopixel_effect_data.color = mgos_sys_config_get_strip_color();
      effect_timer = mgos_set_timer(speed, MGOS_TIMER_REPEAT, strobe_effect, &s_neopixel_effect_data);
      break;
    case 2:
      effect_name = "cylon";
      s_neopixel_effect_data.color = mgos_sys_config_get_strip_color();
      effect_timer = mgos_set_timer(speed / 4 * 3, MGOS_TIMER_REPEAT, cylon_effect, &s_neopixel_effect_data);
      break;
    case 3:
      effect_name = "rainbow";
      effect_timer = mgos_set_timer(speed, MGOS_TIMER_REPEAT, rainbow_effect, NULL);
      break;
    case 4:
      effect_name = "rainbow cycle";
      effect_timer = mgos_set_timer(speed, MGOS_TIMER_REPEAT, rainbow_cycle_effect, NULL);
      break;
    case 5:
      effect_name = "fade";
      effect_timer = mgos_set_timer(speed / 4, MGOS_TIMER_REPEAT, fade_effect, NULL);
      break;
    case 6:
      effect_name = "flash";
      effect_timer = mgos_set_timer(speed * 3, MGOS_TIMER_REPEAT, flash_effect, NULL);
      break;
    case 7:
      effect_name = "RGB loop";
      effect_timer = mgos_set_timer(speed / 5, MGOS_TIMER_REPEAT, rbg_loop_effect, NULL);
      break;
    case 8:
      effect_name = "twinkle";
      node_neopixel_set_all_pixels(get_rgb_color(0));
      s_neopixel_effect_data.color = mgos_sys_config_get_strip_color();
      effect_timer = mgos_set_timer(speed, MGOS_TIMER_REPEAT, twinkle_effect, &s_neopixel_effect_data);
      break;
    case 9:
      effect_name = "twinkle random";
      node_neopixel_set_all_pixels(get_rgb_color(0));
      effect_timer = mgos_set_timer(speed, MGOS_TIMER_REPEAT, twinkle_random_effect, NULL);
      break;
    case 10:
      effect_name = "fire";
      effect_timer = mgos_set_timer(speed / 10, MGOS_TIMER_REPEAT, fire_effect, NULL);
      break;
    case 11:
      effect_name = "snow";
      snow_effect();
      break;
    case 12:
      effect_name = "meteor";
      effect_timer = mgos_set_timer(speed / 7, MGOS_TIMER_REPEAT, meteor_effect, NULL);
      break;
    default:
      LOG(LL_INFO, ("Bad effect: %d", effect));
      strip_turn_off();
      return;
  }
  LOG(LL_INFO, ("Starting %s effect...", effect_name));
}

static void start_night_light() {
  clear_timers();
  node_neopixel_turn_off();
  mgos_sys_config_set_app_mode(MODE_NIGHT);
  LOG(LL_INFO, ("Starting night light"));
}

static void start_vigilance() {
  clear_timers();
  int speed = mgos_sys_config_get_strip_speed() / 2;
  s_neopixel_effect_data.color = mgos_sys_config_get_strip_color();
  effect_timer = mgos_set_timer(speed, MGOS_TIMER_REPEAT, cylon_effect, &s_neopixel_effect_data);
  mgos_sys_config_set_app_mode(MODE_VIGILANCE);
}

/*
static void bled_timer_cb(void *args) {
  mgos_gpio_toggle(mgos_sys_config_get_pins_bled());
  (void) args;
}
*/

static void smooth_turn_off() {
  smooth_brightness -= 50;
  if(smooth_brightness > 0) {
    node_neopixel_set_brightness(smooth_brightness);
  } else {
    smooth_brightness = 0;
    node_neopixel_set_brightness(0);
    clear_timers();
  }
}

static void check_last_motion_time() {
  int current_time = mgos_uptime();
  if (current_time > last_motion_time) {
    int diff = mgos_uptime() - last_motion_time;
    if (diff < mgos_sys_config_get_pir_keep()) {
      int wait = (mgos_sys_config_get_pir_keep() - diff) * 1000;
      mgos_set_timer(wait, false, check_last_motion_time, NULL);
    } else {
      clear_timers();
      smooth_timer = mgos_set_timer(1000, MGOS_TIMER_REPEAT, smooth_turn_off, NULL);
    }
  } else {
    mgos_set_timer(mgos_sys_config_get_pir_keep() * 1000, false, check_last_motion_time, NULL);
  }
}

static void smooth_turn_on() {
  smooth_brightness += 50;
  if(smooth_brightness <= 250) {
    node_neopixel_set_brightness(smooth_brightness);
  } else {
    smooth_brightness = 250;
    node_neopixel_set_brightness(255);
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
        if(alert_timer == MGOS_INVALID_TIMER_ID) {
          clear_timers();
          s_neopixel_effect_data.color = mgos_sys_config_get_strip_color();
          effect_timer = mgos_set_timer(100, MGOS_TIMER_REPEAT, strobe_effect, &s_neopixel_effect_data);
          alert_timer = mgos_set_timer(15000, false, start_vigilance, NULL);
          mgos_mqtt_pubf("alert/motion", 1, false, MOTION_ALERT_JSON_FMT, mgos_uptime());
        }
        break;
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
  mg_rpc_send_responsef(ri, RPC_SUCCESS_RESPONSE_JSON_FMT);
  (void) args;
  (void) src;
  (void) user_data;
}

static void rpc_turn_off_cb(struct mg_rpc_request_info *ri, const char *args,
                           const char *src, void *user_data) {
  strip_turn_off();
  mg_rpc_send_responsef(ri, RPC_SUCCESS_RESPONSE_JSON_FMT);
  (void) args;
  (void) src;
  (void) user_data;
}

static void rpc_set_effect_cb(struct mg_rpc_request_info *ri, const char *args,
                           const char *src, void *user_data) {
  int e = atoi(args);
  if (e < 0 || e >= TOTAL_EFFECTS) {
    mg_rpc_send_errorf(ri, -1, "{error: \"Bad effect index\"}");
    return;
  }
  mgos_sys_config_set_strip_effect(e);
  mgos_sys_config_set_app_mode(MODE_EFFECT);
  start_effect();
  mg_rpc_send_responsef(ri, RPC_SUCCESS_RESPONSE_JSON_FMT);
  (void) args;
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
  mg_rpc_send_responsef(ri, RPC_SUCCESS_RESPONSE_JSON_FMT);
  (void) args;
  (void) src;
  (void) user_data;
}

static void rpc_set_night_light_cb(struct mg_rpc_request_info *ri, const char *args,
                           const char *src, void *user_data) {
  start_night_light();
  mg_rpc_send_responsef(ri, RPC_SUCCESS_RESPONSE_JSON_FMT);
  (void) args;
  (void) src;
  (void) user_data;
}

static void rpc_set_vigilance_cb(struct mg_rpc_request_info *ri, const char *args,
                           const char *src, void *user_data) {
  start_vigilance();
  mg_rpc_send_responsef(ri, RPC_SUCCESS_RESPONSE_JSON_FMT);
  (void) args;
  (void) src;
  (void) user_data;
}

static void rpc_set_color_cb(struct mg_rpc_request_info *ri, const char *args,
                           const char *src, void *user_data) {
  int color = atoi(args);
  LOG(LL_INFO, ("Set color: %d", color & 0xFFFFFF));
  mgos_sys_config_set_strip_color(color & 0xFFFFFF);
  strip_turn_on();
  mg_rpc_send_responsef(ri, RPC_SUCCESS_RESPONSE_JSON_FMT);
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
    node_pir_set_pir_toggle_handler(node_pir_toggle_handler);
  } else {
    LOG(LL_ERROR, ("Error initializing the nodes"));
  }

  // Configure Built in LED
  /*mgos_gpio_set_mode(mgos_sys_config_get_pins_bled(), MGOS_GPIO_MODE_OUTPUT);
  mgos_set_timer(1000, MGOS_TIMER_REPEAT, bled_timer_cb, NULL);*/

  // Configure LED indicator for PIR sensor 
  mgos_gpio_set_mode(mgos_sys_config_get_pins_led(), MGOS_GPIO_MODE_OUTPUT);

  // Configure RPC interface
  mgos_rpc_add_handler("Driver.On", rpc_turn_on_cb, NULL);
  mgos_rpc_add_handler("Driver.Off", rpc_turn_off_cb, NULL);
  mgos_rpc_add_handler("Driver.Effect", rpc_set_effect_cb, NULL);
  mgos_rpc_add_handler("Driver.Next", rpc_set_next_effect_cb, NULL);
  mgos_rpc_add_handler("Driver.Night", rpc_set_night_light_cb, NULL);
  mgos_rpc_add_handler("Driver.Vigilance", rpc_set_vigilance_cb, NULL);
  mgos_rpc_add_handler("Driver.Color", rpc_set_color_cb, NULL);

  if (mgos_blynk_init()) {
    blynk_set_handler(custom_blynk_handler, NULL);
  } else {
    LOG(LL_ERROR, ("Failed to init Blynk"));
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
