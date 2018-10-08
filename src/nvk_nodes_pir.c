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
#include "nvk_nodes_pir.h"
#include "mgos_time.h"
#include "mgos_mqtt.h"

static mgos_timer_id node_pir_samp_int_timer_id = MGOS_INVALID_TIMER_ID;
static node_switch_handler_t s_node_pir_toggle_handler = NULL;
static void *s_node_pir_toggle_user_data = NULL;

static int s_node_pir_state = 0;

const char PIR_STAT_JSON_FMT[] = "{pir:{state:%d,uptime:%f}}";

void default_node_pir_toggle_handler(int value, void *user_data) {
    LOG(LL_INFO, ("PIR: %d", value));
    (void) user_data;
}

void node_pir_set_pir_toggle_handler(node_switch_handler_t func, void *user_data) {
    s_node_pir_toggle_handler = func;
    s_node_pir_toggle_user_data = user_data;
}

void node_pir_sampling_handler(void *args) {
    bool state = mgos_gpio_read(mgos_sys_config_get_nodes_pir_pin());
    if (state != s_node_pir_state) {
        s_node_pir_toggle_handler(state, s_node_pir_toggle_user_data);
        const char *topic = mgos_sys_config_get_nodes_pir_stat_topic();
        mgos_mqtt_pubf(topic, 1, false, PIR_STAT_JSON_FMT, state, mgos_uptime());
    }
    s_node_pir_state = state;

    (void) args;
}

bool node_pir_init() {
    bool enabled = mgos_sys_config_get_nodes_pir_enable();
    if(enabled) {
        int pin = mgos_sys_config_get_nodes_pir_pin();
        int sampling_interval = mgos_sys_config_get_nodes_pir_sampling_interval();
        mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_INPUT);
        node_pir_samp_int_timer_id = mgos_set_timer(sampling_interval, MGOS_TIMER_REPEAT, node_pir_sampling_handler, NULL);
        node_pir_set_pir_toggle_handler(default_node_pir_toggle_handler, NULL);
    }
    return enabled;
}