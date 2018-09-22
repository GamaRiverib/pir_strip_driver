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

static struct mgos_dht *s_dht = NULL;
// static mgos_timer_id effect_timer = MGOS_INVALID_TIMER_ID;
static mgos_timer_id smooth_timer = MGOS_INVALID_TIMER_ID;
static float last_motion_time = 0;
static int smooth_brightness = 0;

// TODO: static void get_weather(char *)

/*
static void clear_effect_timer() {
  if(effect_timer != MGOS_INVALID_TIMER_ID) {
    mgos_clear_timer(effect_timer);
    effect_timer = MGOS_INVALID_TIMER_ID;
  }
}
*/

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

static void bled_timer_cb(void *args) {
  mgos_gpio_toggle(mgos_sys_config_get_pins_bled());
  (void) args;
}

static void dht_timer_cb(void *dht) {
  float t = mgos_dht_get_temp(dht);
  float h = mgos_dht_get_humidity(dht);
  int l = get_luminosity();

  if (isnan(h) || isnan(t)) {
    LOG(LL_INFO, ("Failed to read data from sensor\n"));
    return;
  }
  LOG(LL_INFO, ("\nTemperature: %d *C \nHumidity: %d %%\nLuminosity: %d lux", (int)t, (int)h, l));

  mgos_mqtt_pubf("Weather", 1, false, "{temp:%d,hum:%d,lum:%d,uptime:%f}", (int)t, (int)h, l, mgos_uptime());

  (void) dht;
}

static void led_strip_brightness(int brightness) {
  LOG(LL_INFO, ("Set Led Strip Brightness: %d", brightness));
}

static void smooth_turn_off() {
  smooth_brightness -= 50;
  if(smooth_brightness > 0) {
    led_strip_brightness(smooth_brightness);
  } else {
    if(smooth_timer != MGOS_INVALID_TIMER_ID) {
      mgos_clear_timer(smooth_timer);
      smooth_timer = MGOS_INVALID_TIMER_ID;
    }
    // TODO: turn_off
  }
}

static void check_last_motion_time() {
  LOG(LL_INFO, ("Check last motion time"));
  int diff = mgos_uptime() - last_motion_time;
  if (diff < mgos_sys_config_get_pir_keep()) {
    int wait = mgos_sys_config_get_pir_keep() - diff;
    mgos_set_timer(wait, false, check_last_motion_time, NULL);
  } else {
    if(smooth_timer != MGOS_INVALID_TIMER_ID) {
      mgos_clear_timer(smooth_timer);
    }
    smooth_timer = mgos_set_timer(1000, true, smooth_turn_off, NULL);
  }
}

static void smooth_turn_on() {
  LOG(LL_INFO, ("Smooth turn ON"));
  smooth_brightness += 50;
  if(smooth_brightness <= 250) {
    led_strip_brightness(smooth_brightness);
  } else {
    if (smooth_timer != MGOS_INVALID_TIMER_ID) {
      mgos_clear_timer(smooth_timer);
      smooth_timer = MGOS_INVALID_TIMER_ID;
    }
    int wait = mgos_sys_config_get_pir_keep() * 1000;
    smooth_timer = mgos_set_timer(wait, true, check_last_motion_time, NULL);
  }
}

static void motion_handler() {
  last_motion_time = mgos_uptime();
  switch(mgos_sys_config_get_app_mode()) {
    case 3:
      if(is_dark() && smooth_timer == MGOS_INVALID_TIMER_ID) {
        smooth_timer = mgos_set_timer(1000, true, smooth_turn_on, NULL);
      }
      break;
    case 4:
      LOG(LL_INFO, ("ALERT"));
      break;
    default:
      LOG(LL_INFO, ("Ignore motion"));
  }
}

static void pir_timer_cb(void *args) {
  bool motion = mgos_gpio_read(mgos_sys_config_get_pins_pir());
  if(mgos_sys_config_get_pir_indicator()) {
    mgos_gpio_write(mgos_sys_config_get_pins_led(), motion);
  }
  if(motion) {
    LOG(LL_INFO, ("[%f] Motion detected", mgos_uptime()));
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
    mg_rpc_send_errorf(ri, -1, "{error: \"Failed to read data from sensor\"");
  } else {
    LOG(LL_INFO, ("\nTemperature: %d *C \nHumidity: %d %%\nLuminosity: %d lux", (int)t, (int)h, l));
    mg_rpc_send_responsef(ri, "{temp:%d,hum:%d,lum:%d,uptime:%f}", (int)t, (int)h, l, mgos_uptime());
  }
  (void) src;
  (void) args;
}

enum mgos_app_init_result mgos_app_init(void) {

  // Configure Built in LED
  mgos_gpio_set_mode(mgos_sys_config_get_pins_bled(), MGOS_GPIO_MODE_OUTPUT);
  mgos_set_timer(1000, true, bled_timer_cb, NULL);

  // Configure DHT sensor
  s_dht = mgos_dht_create(mgos_sys_config_get_pins_dht(), DHT11);
  mgos_set_timer(mgos_sys_config_get_app_tele() * 1000, true, dht_timer_cb, s_dht);

  // Configure PIR sensor
  mgos_gpio_set_mode(mgos_sys_config_get_pins_pir(), MGOS_GPIO_MODE_INPUT); // return bool
  mgos_gpio_set_mode(mgos_sys_config_get_pins_led(), MGOS_GPIO_MODE_OUTPUT);
  mgos_set_timer(500, true, pir_timer_cb, NULL);

  // Configure RPC interface
  mgos_rpc_add_handler("Driver.GetWeather", rpc_weather_cb, s_dht);

  return MGOS_APP_INIT_SUCCESS;
}
