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

#ifndef NVK_INCLUDE_EFFECT_FIRE_H_
#define NVK_INCLUDE_EFFECT_FIRE_H_

#ifdef __cplusplus
extern "C" {
#endif

static int heat[30]; // TODO

void fire_effect(void *args) {
    (void) args;
    int cooldown;
    int num_pixels = mgos_sys_config_get_nodes_neopixel_pixels();
    int cooling = mgos_sys_config_get_effects_fire_cooling();
    int sparking = mgos_sys_config_get_effects_fire_sparking();

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

    node_neopixel_clear();
    for(int j = 0; j < num_pixels; j++) {
        int t192 = round((heat[j] / 255.0) * 191);
        int heatramp = t192 & 0x3F;
        heatramp <<= 0x02;
        heatramp = heatramp & 0xFF;
        if(t192 > 0x80) {
            node_neopixel_set(j, 0xFF, 0xFF, heatramp);
        } else if(t192 > 0x40) {
            node_neopixel_set(j, 0xFF, heatramp, 0x00);
        } else {
            node_neopixel_set(j, heatramp, 0x00, 0x00);
        }
    }

    node_neopixel_show();
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NVK_INCLUDE_EFFECT_FIRE_H_ */