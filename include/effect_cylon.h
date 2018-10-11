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

/*
 * NVK Node Lib.
 */
#include <stdbool.h>
#include "mgos.h"
#include "nvk_nodes_neopixel.h"

#ifndef NVK_INCLUDE_EFFECT_CYLON_H_
#define NVK_INCLUDE_EFFECT_CYLON_H_

#ifdef __cplusplus
extern "C" {
#endif

static bool s_cylon_effect_dir = true;
static int s_cylon_effect_counter = 0;

void cylon_effect(void *args) {
    int eye_size = mgos_sys_config_get_effects_cylon_size();
    neopixel_effect_data *user_cylon_data = (neopixel_effect_data*) args;
    rgb_color c = get_rgb_color(user_cylon_data->color);
    int r = c.red / 10;
    int g = c.green / 10;
    int b = c.blue / 10;
    int p = s_cylon_effect_counter;
    int n = mgos_sys_config_get_nodes_neopixel_pixels();
    
    if(s_cylon_effect_dir) {
        s_cylon_effect_counter++;
        s_cylon_effect_dir = p < (n - eye_size - 3);
    } else {
        s_cylon_effect_counter--;
        s_cylon_effect_dir = p <= 1;
    }

    node_neopixe_set_all(0, 0, 0);
    node_neopixel_set(p, r, g, b);
    for (int i = 1; i <= eye_size; i++) {
        node_neopixel_set(p + i, c.red, c.green, c.blue);
    }
    node_neopixel_set(p + eye_size + 1, r, g, b);
    node_neopixel_show();
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NVK_INCLUDE_EFFECT_CYLON_H_ */