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
#include "mgos_adc.h"
#include "nvk_nodes.h"
#include "nvk_nodes_photoresistor.h"
#include "mgos_time.h"
#include "mgos_mqtt.h"
#include "mgos_rpc.h"

static node_on_range_handler_t s_node_photoresistor_lum_on_range_handler = NULL;
static node_out_range_handler_t s_node_photoresistor_lum_out_range_handler = NULL;

const char PHOTORESISTOR_TELE_JSON_FMT[] = "{photoresistor:{luminosity:%d,uptime:%f}}";
const char PHOTORESISTOR_RPC_STAT_METHOD_NAME[] = "Nodes.Photoresistor.Stat";

static mgos_timer_id node_photoresistor_samp_int_timer_id = MGOS_INVALID_TIMER_ID;
static mgos_timer_id node_photoresistor_tele_int_timer_id = MGOS_INVALID_TIMER_ID;

void node_photoresistor_set_lum_on_range_handler(node_on_range_handler_t func) {
    s_node_photoresistor_lum_on_range_handler = func;
}

void node_photoresistor_set_lum_out_range_handler(node_out_range_handler_t func) {
    s_node_photoresistor_lum_out_range_handler = func;
}

void default_node_photoresistor_on_range_handler(struct node_range_values *values, void *user_data) {
    LOG(LL_INFO, ("%s ON RANGE: current=%d min=%d max=%d", (char *) user_data, values->current, values->min, values->max));
}

void default_node_photoresistor_out_range_handler(enum node_out_range_event_type ev, struct node_range_values *values, void *user_data) {
    switch(ev) {
        case ABOVE_OF_RANGE:
            LOG(LL_INFO, ("%s ABOVE OF RANGE: current=%d min=%d max=%d", (char *) user_data, values->current, values->min, values->max));
            break;
        case BELOW_OF_RANGE:
            LOG(LL_INFO, ("%s BELOW OF RANGE: current=%d min=%d max=%d", (char *) user_data, values->current, values->min, values->max));
            break;
    }
}

int node_photoresistor_get_luminosity() {
    if(mgos_adc_enable(0)) {
        return mgos_adc_read(0);
    }
    LOG(LL_INFO, ("ADC not available"));
    return 0;
}

void node_photoresistor_sampling_handler(void *user_data) {
    int l = node_photoresistor_get_luminosity();
    
    int lumi_min = mgos_sys_config_get_nodes_photoresistor_props_lumi_range_min();
    int lumi_max = mgos_sys_config_get_nodes_photoresistor_props_lumi_range_max();

    struct node_range_values lumi_values = { .current = l, .min = lumi_min, .max = lumi_max };
    if (l < lumi_min || l > lumi_max) {
        enum node_out_range_event_type ev;
        if (l < lumi_min) {
            ev = BELOW_OF_RANGE;
        } else {
            ev = ABOVE_OF_RANGE;
        }
        s_node_photoresistor_lum_out_range_handler(ev, &lumi_values, "Luminosity");
    } else {
        s_node_photoresistor_lum_on_range_handler(&lumi_values, "Luminosity");
    }

    LOG(LL_INFO, ("Luminosity: %d lux", l));
    
    (void) user_data;
}

void node_photoresistor_tele_handler(void *user_data) {
    int l = node_photoresistor_get_luminosity();
    const char *topic = mgos_sys_config_get_nodes_photoresistor_tele_topic();
    mgos_mqtt_pubf(topic, 1, false, PHOTORESISTOR_TELE_JSON_FMT, l, mgos_uptime());
    (void) user_data;
}

void node_photoresistor_rpc_stat_handler(struct mg_rpc_request_info *ri, const char *args, const char *src, void *user_data) {
    int l = node_photoresistor_get_luminosity();
    LOG(LL_INFO, ("Luminosity: %d lux", l));
    const char *topic = mgos_sys_config_get_nodes_photoresistor_tele_topic();
    mgos_mqtt_pubf(topic, 1, false, PHOTORESISTOR_TELE_JSON_FMT, l, mgos_uptime());
    mg_rpc_send_responsef(ri, PHOTORESISTOR_TELE_JSON_FMT, l, mgos_uptime());
    (void) args;
    (void) src;
    (void) user_data;
}

bool node_photoresistor_init() {
    int pin = mgos_sys_config_get_nodes_photoresistor_pin();
    bool enabled = mgos_sys_config_get_nodes_photoresistor_enable() && mgos_adc_enable(pin);
    if(enabled) {
        int sampling_interval = mgos_sys_config_get_nodes_photoresistor_sampling_interval();
        int tele_interval = mgos_sys_config_get_nodes_photoresistor_tele_interval();
        node_photoresistor_samp_int_timer_id = mgos_set_timer(sampling_interval, MGOS_TIMER_REPEAT, node_photoresistor_sampling_handler, NULL);
        node_photoresistor_tele_int_timer_id = mgos_set_timer(tele_interval, MGOS_TIMER_REPEAT, node_photoresistor_tele_handler, NULL);
        mgos_rpc_add_handler(PHOTORESISTOR_RPC_STAT_METHOD_NAME, node_photoresistor_rpc_stat_handler, NULL);
        node_photoresistor_set_lum_on_range_handler(default_node_photoresistor_on_range_handler);
        node_photoresistor_set_lum_out_range_handler(default_node_photoresistor_out_range_handler);
    }
    return enabled;
}