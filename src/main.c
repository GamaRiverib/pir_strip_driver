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
#include "nvk_nodes.h"
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

static mgos_timer_id effect_timer = MGOS_INVALID_TIMER_ID;

static neopixel_effect_data s_neopixel_effect_data = { 0 };

const char RPC_DEVICE_STATE_JSON_FMT[] = "{id:\"%s\",mode:%d}";
const char RPC_SUCCESS_RESPONSE_JSON_FMT[] = "{success:true}";
const char EFFECTS_LIST[][16] = {
  "default",
  "strobe",
  "cylon",
  "rainbow",
  "rainbow cycle",
  "fade",
  "flash",
  "RGB loop",
  "twinkle",
  "twinkle random",
  "fire",
  "snow",
  "meteor"
};

static void clear_timers() {
  if(effect_timer != MGOS_INVALID_TIMER_ID) {
    mgos_clear_timer(effect_timer);
    effect_timer = MGOS_INVALID_TIMER_ID;
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

static void start_effect() {
  clear_timers();
  int effect = mgos_sys_config_get_strip_effect();
  int speed = mgos_sys_config_get_strip_speed();
  switch(effect) {
    case 0:
      effect_timer = mgos_set_timer(speed / 4 * 3, MGOS_TIMER_REPEAT, first_effect, NULL);
      break;
    case 1:
      s_neopixel_effect_data.color = mgos_sys_config_get_strip_color();
      effect_timer = mgos_set_timer(speed, MGOS_TIMER_REPEAT, strobe_effect, &s_neopixel_effect_data);
      break;
    case 2:
      s_neopixel_effect_data.color = mgos_sys_config_get_strip_color();
      effect_timer = mgos_set_timer(speed / 4 * 3, MGOS_TIMER_REPEAT, cylon_effect, &s_neopixel_effect_data);
      break;
    case 3:
      effect_timer = mgos_set_timer(speed, MGOS_TIMER_REPEAT, rainbow_effect, NULL);
      break;
    case 4:
      effect_timer = mgos_set_timer(speed, MGOS_TIMER_REPEAT, rainbow_cycle_effect, NULL);
      break;
    case 5:
      effect_timer = mgos_set_timer(speed / 4, MGOS_TIMER_REPEAT, fade_effect, NULL);
      break;
    case 6:
      effect_timer = mgos_set_timer(speed * 3, MGOS_TIMER_REPEAT, flash_effect, NULL);
      break;
    case 7:
      effect_timer = mgos_set_timer(speed / 5, MGOS_TIMER_REPEAT, rbg_loop_effect, NULL);
      break;
    case 8:
      node_neopixel_set_all_pixels(get_rgb_color(0));
      s_neopixel_effect_data.color = mgos_sys_config_get_strip_color();
      effect_timer = mgos_set_timer(speed, MGOS_TIMER_REPEAT, twinkle_effect, &s_neopixel_effect_data);
      break;
    case 9:
      node_neopixel_set_all_pixels(get_rgb_color(0));
      effect_timer = mgos_set_timer(speed, MGOS_TIMER_REPEAT, twinkle_random_effect, NULL);
      break;
    case 10:
      effect_timer = mgos_set_timer(speed / 10, MGOS_TIMER_REPEAT, fire_effect, NULL);
      break;
    case 11:
      snow_effect();
      break;
    case 12:
      effect_timer = mgos_set_timer(speed / 7, MGOS_TIMER_REPEAT, meteor_effect, NULL);
      break;
    default:
      LOG(LL_INFO, ("Bad effect: %d", effect));
      strip_turn_off();
      return;
  }
  LOG(LL_INFO, ("Starting %s effect...", EFFECTS_LIST[effect]));
}

static void rpc_get_device_state(struct mg_rpc_request_info *ri, const char *args,
                           const char *src, void *user_data) {
  const char* id = mgos_sys_config_get_device_id();
  int mode = mgos_sys_config_get_app_mode();
  mg_rpc_send_responsef(ri, RPC_DEVICE_STATE_JSON_FMT, id, mode);
  (void) args;
  (void) src;
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
  if (mgos_sys_config_get_app_mode() == MODE_EFFECT) {
    e++;
    if (e >= TOTAL_EFFECTS) {
      e = 0;
    }
    mgos_sys_config_set_strip_effect(e);
  } else {
    mgos_sys_config_set_app_mode(MODE_EFFECT);
  }
  start_effect();
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
  if(mgos_sys_config_get_app_mode() == MODE_OFF) {
    strip_turn_on();
  }
  mg_rpc_send_responsef(ri, RPC_SUCCESS_RESPONSE_JSON_FMT);
  (void) args;
  (void) src;
  (void) user_data;
}

enum mgos_app_init_result mgos_app_init(void) {

  if(mgos_nodes_init()) {
    LOG(LL_INFO, ("Initializing nodes done!"));
  } else {
    LOG(LL_ERROR, ("Error initializing the nodes"));
  }

  // Configure RPC interface
  mgos_rpc_add_handler("Driver.State", rpc_get_device_state, NULL);
  mgos_rpc_add_handler("Driver.On", rpc_turn_on_cb, NULL);
  mgos_rpc_add_handler("Driver.Off", rpc_turn_off_cb, NULL);
  mgos_rpc_add_handler("Driver.Effect", rpc_set_effect_cb, NULL);
  mgos_rpc_add_handler("Driver.Next", rpc_set_next_effect_cb, NULL);
  mgos_rpc_add_handler("Driver.Color", rpc_set_color_cb, NULL);

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
    default:
      LOG(LL_INFO, ("Bad mode %d", mode));
      mgos_sys_config_set_app_mode(0);
      // TODO: Save config and reboot
  }  

  return MGOS_APP_INIT_SUCCESS;
}
