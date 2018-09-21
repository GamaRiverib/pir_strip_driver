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
#include "mgos_dht.h"
#include "mgos_adc.h"
#include "mgos_rpc.h"

static struct mgos_dht *s_dht = NULL;

static void dht_timer_cb(void *dht) {
  int t = mgos_dht_get_temp(dht);
  int h = mgos_dht_get_humidity(dht);
  int l = mgos_adc_read(0);

  if (isnan(h) || isnan(t)) {
    LOG(LL_INFO, ("Failed to read data from sensor\n"));
    return;
  }
  LOG(LL_INFO, ("\nTemperature: %d *C \nHumidity: %d %%\nLuminosity: %d lux", t, h, l));
  (void) dht;
}

static void rpc_weather_cb(struct mg_rpc_request_info *ri, const char *args,
                           const char *src, void *dht) {
  int t = mgos_dht_get_temp(dht);
  int h = mgos_dht_get_humidity(dht);
  int l = mgos_adc_read(0);
  if(isnan(h) || isnan(t)) {
    mg_rpc_send_errorf(ri, -1, "{error: \"Failed to read data from sensor\"");
  } else {
    LOG(LL_INFO, ("\nTemperature: %d *C \nHumidity: %d %%\nLuminosity: %d lux", t, h, l));
    mg_rpc_send_responsef(ri, "{temp:%d,hum:%d,lum:%d}", t, h, l);
  }
  (void) src;
  (void) args;
}

enum mgos_app_init_result mgos_app_init(void) {
  
  // Configure DHT sensor
  s_dht = mgos_dht_create(mgos_sys_config_get_pins_dht(), DHT11);
  mgos_set_timer(mgos_sys_config_get_app_tele() * 1000, true, dht_timer_cb, s_dht);

  // Configure RPC interface
  mgos_rpc_add_handler("Weather", rpc_weather_cb, s_dht);

  return MGOS_APP_INIT_SUCCESS;
}
