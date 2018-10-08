/*
 * Copyright (c) 2018 Novutek S.C.
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
#include "nvk_nodes.h"
#include "nvk_nodes_dht.h"
#include "mgos_time.h"
#include "mgos_dht.h"
#include "mgos_mqtt.h"
#include "mgos_rpc.h"

static struct mgos_dht *s_node_dht = NULL;
static node_on_range_handler_t s_node_temp_on_range_handler = NULL;
static node_out_range_handler_t s_node_temp_out_range_handler = NULL;
static node_on_range_handler_t s_node_humd_on_range_handler = NULL;
static node_out_range_handler_t s_node_humd_out_range_handler = NULL;
static void *s_node_temp_on_range_user_data = NULL;
static void *s_node_temp_out_range_user_data = NULL;
static void *s_node_humd_on_range_user_data = NULL;
static void *s_node_humd_out_range_user_data = NULL;

const char DHT_TELE_JSON_FMT[] = "{dht:{temperature:%d,humidity:%d,uptime:%f}}";
const char DHT_RPC_STAT_METHOD_NAME[] = "Nodes.DHT.stat";
const char DHT_RPC_SET_RANGE_MIN_METHOD_NAME[] = "Nodes.DHT.Range.Min";
const char DHT_RPC_SET_RANGE_MAX_METHOD_NAME[] = "Nodes.DHT.Range.Max";
const char DHT_RPC_SET_SAMPLING_INTERVAL_METHOD_NAME[] = "Nodes.DHT.Sampling.Interval";
const char DHT_RPC_SET_TELE_INTERVAL_METHOD_NAME[] = "Nodes.DHT.Tele.Interval";

static mgos_timer_id node_dht_samp_int_timer_id = MGOS_INVALID_TIMER_ID;
static mgos_timer_id node_dht_tele_int_timer_id = MGOS_INVALID_TIMER_ID;

void node_dht_set_temp_on_range_handler(node_on_range_handler_t func, void *user_data) {
    s_node_temp_on_range_handler = func;
    s_node_temp_on_range_user_data = user_data;
}

void node_dht_set_temp_out_range_handler(node_out_range_handler_t func, void *user_data) {
    s_node_temp_out_range_handler = func;
    s_node_temp_out_range_user_data = user_data;
}

void node_dht_set_humd_on_range_handler(node_on_range_handler_t func, void *user_data) {
    s_node_humd_on_range_handler = func;
    s_node_humd_on_range_user_data = user_data;
}

void node_dht_set_humd_out_range_handler(node_out_range_handler_t func, void *user_data) {
    s_node_humd_out_range_handler = func;
    s_node_humd_out_range_user_data = user_data;
}

void default_node_on_range_handler(struct node_range_values *values, void *user_data) {
    LOG(LL_INFO, ("ON RANGE: current=%d min=%d max=%d", values->current, values->min, values->max));
    (void) user_data;
}

void default_node_out_range_handler(enum node_out_range_event_type ev, struct node_range_values *values, void *user_data) {
    switch(ev) {
        case ABOVE_OF_RANGE:
            LOG(LL_INFO, ("ABOVE OF RANGE: current=%d min=%d max=%d", values->current, values->min, values->max));
            break;
        case BELOW_OF_RANGE:
            LOG(LL_INFO, ("BELOW OF RANGE: current=%d min=%d max=%d", values->current, values->min, values->max));
            break;
    }
    (void) user_data;
}

void node_dht_sampling_handler(void *dht) {
    float t = mgos_dht_get_temp(dht);
    float h = mgos_dht_get_humidity(dht);
    if (isnan(h) || isnan(t)) {
        LOG(LL_INFO, ("Failed to read data from sensor\n"));
        return;
    }

    int temp_min = mgos_sys_config_get_nodes_dht_props_temp_range_min();
    int temp_max = mgos_sys_config_get_nodes_dht_props_temp_range_max();
    int humd_min = mgos_sys_config_get_nodes_dht_props_humd_range_min();
    int humd_max = mgos_sys_config_get_nodes_dht_props_humd_range_max();

    struct node_range_values temp_values = { .current = t, .min = temp_min, .max = temp_max };
    if (t < temp_min || t > temp_max) {
        enum node_out_range_event_type ev;
        if (t < temp_min) {
            ev = BELOW_OF_RANGE;
        } else {
            ev = ABOVE_OF_RANGE;
        }
        s_node_temp_out_range_handler(ev, &temp_values, s_node_temp_out_range_user_data);
    } else {
        s_node_temp_on_range_handler(&temp_values, s_node_temp_on_range_user_data);
    }

    struct node_range_values humd_values = { .current = h, .min = humd_min, .max = humd_max };
    if(h < humd_min || h > humd_max) {
        enum node_out_range_event_type ev;
        if ( h < humd_min) {
            ev = BELOW_OF_RANGE;
        } else {
            ev = ABOVE_OF_RANGE;
        }
        s_node_humd_out_range_handler(ev, &humd_values, s_node_humd_out_range_user_data);
    } else {
        s_node_humd_on_range_handler(&humd_values, s_node_humd_on_range_user_data);
    }
    
    LOG(LL_INFO, ("Temperature: %d*C \tHumidity: %d%%", (int)t, (int)h));
    
    (void) dht;
}

void node_dht_tele_handler(void *dht) {
    float t = mgos_dht_get_temp(dht);
    float h = mgos_dht_get_humidity(dht);
    if (isnan(h) || isnan(t)) {
        LOG(LL_INFO, ("Failed to read data from sensor\n"));
        return;
    }

    const char *topic = mgos_sys_config_get_nodes_dht_tele_topic();
    mgos_mqtt_pubf(topic, 1, false, DHT_TELE_JSON_FMT, (int)t, (int)h, mgos_uptime());

}

void node_dht_rpc_stat_handler(struct mg_rpc_request_info *ri, const char *args,
                           const char *src, void *dht) {
  float t = mgos_dht_get_temp(dht);
  float h = mgos_dht_get_humidity(dht);
  if(isnan(h) || isnan(t)) {
    mg_rpc_send_errorf(ri, -1, "{error: \"Failed to read data from sensor\"}");
  } else {
    LOG(LL_INFO, ("Temperature: %d*C \tHumidity: %d%%", (int)t, (int)h));
    const char *topic = mgos_sys_config_get_nodes_dht_tele_topic();
    mgos_mqtt_pubf(topic, 1, false, DHT_TELE_JSON_FMT, (int)t, (int)h, mgos_uptime());
    mg_rpc_send_responsef(ri, DHT_TELE_JSON_FMT, (int)t, (int)h, mgos_uptime());
  }
  (void) args;
  (void) src;
}

void node_dht_rpc_set_range_min_handler(struct mg_rpc_request_info *ri, const char *args,
                           const char *src, void *dht) {
    LOG(LL_INFO, (args));
    mg_rpc_send_responsef(ri, "{success:true}");
    (void) args;
    (void) src;
    (void) dht;
}

void node_dht_rpc_set_range_max_handler(struct mg_rpc_request_info *ri, const char *args,
                           const char *src, void *dht) {
    LOG(LL_INFO, (args));
    mg_rpc_send_responsef(ri, "{success:true}");
    (void) args;
    (void) src;
    (void) dht;
}

void node_dht_rpc_set_sampling_interval_handler(struct mg_rpc_request_info *ri, const char *args,
                           const char *src, void *dht) {
    LOG(LL_INFO, (args));
    mg_rpc_send_responsef(ri, "{success:true}");
    (void) args;
    (void) src;
    (void) dht;
}

void node_dht_rpc_set_tele_interval_handler(struct mg_rpc_request_info *ri, const char *args,
                           const char *src, void *dht) {
    LOG(LL_INFO, (args));
    mg_rpc_send_responsef(ri, "{success:true}");
    (void) args;
    (void) src;
    (void) dht;
}

bool node_dht_init() {
    bool enabled = mgos_sys_config_get_nodes_dht_enable();
    if(enabled) {
        int pin = mgos_sys_config_get_nodes_dht_pin();
        int type = mgos_sys_config_get_nodes_dht_type();
        int sampling_interval = mgos_sys_config_get_nodes_dht_sampling_interval();
        int tele_interval = mgos_sys_config_get_nodes_dht_tele_interval();

        s_node_dht = mgos_dht_create(pin, type);
        node_dht_samp_int_timer_id = mgos_set_timer(sampling_interval, MGOS_TIMER_REPEAT, node_dht_sampling_handler, s_node_dht);
        node_dht_tele_int_timer_id = mgos_set_timer(tele_interval, MGOS_TIMER_REPEAT, node_dht_tele_handler, s_node_dht);
        mgos_rpc_add_handler(DHT_RPC_STAT_METHOD_NAME, node_dht_rpc_stat_handler, s_node_dht);
        mgos_rpc_add_handler(DHT_RPC_SET_RANGE_MIN_METHOD_NAME, node_dht_rpc_set_range_min_handler, s_node_dht);
        mgos_rpc_add_handler(DHT_RPC_SET_RANGE_MAX_METHOD_NAME, node_dht_rpc_set_range_max_handler, s_node_dht);
        mgos_rpc_add_handler(DHT_RPC_SET_SAMPLING_INTERVAL_METHOD_NAME, node_dht_rpc_set_sampling_interval_handler, s_node_dht);
        mgos_rpc_add_handler(DHT_RPC_SET_TELE_INTERVAL_METHOD_NAME, node_dht_rpc_set_tele_interval_handler, s_node_dht);

        node_dht_set_temp_on_range_handler(default_node_on_range_handler, NULL);
        node_dht_set_temp_out_range_handler(default_node_out_range_handler, NULL);
        node_dht_set_humd_on_range_handler(default_node_on_range_handler, NULL);
        node_dht_set_humd_out_range_handler(default_node_out_range_handler, NULL);
    }
    return enabled;
}