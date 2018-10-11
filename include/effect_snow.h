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

#ifndef NVK_INCLUDE_EFFECT_SNOW_H_
#define NVK_INCLUDE_EFFECT_SNOW_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SNOW_EFFECT_INDEX 11
#define MODE_EFFECT 2

static void snow_effect_cb() {
  rgb_color c = get_rgb_color(0x101010);
  node_neopixel_set_all_pixels(c);
}

void snow_effect() {
    if(mgos_sys_config_get_strip_effect() == SNOW_EFFECT_INDEX && mgos_sys_config_get_app_mode() == MODE_EFFECT) {
        rgb_color c = get_rgb_color(0x101010);
        node_neopixel_set_all_pixels(c);
        int num_pixels = mgos_sys_config_get_nodes_neopixel_pixels();
        int p = (int) mgos_rand_range(0, num_pixels - 1);
        node_neopixel_set(p, 0xFF, 0xFF, 0xFF);
        node_neopixel_show();
        mgos_set_timer(20, false, snow_effect_cb, NULL);
        int d = (int) mgos_rand_range(120, 1000);
        mgos_set_timer(d, false, snow_effect, NULL);
    }
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NVK_INCLUDE_EFFECT_SNOW_H_ */