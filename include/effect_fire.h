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

#define FIRE_EFFECT_COOLING 90
#define FIRE_EFFECT_SPARKING 120

static int s_fire_effect_heat[30]; // TODO

void fire_effect(void *args) {
    (void) args;
    int cooldown;
    int num_pixels = mgos_sys_config_get_nodes_neopixel_pixels();

    for(int i = 0; i < num_pixels; i++) {
        cooldown = (int) mgos_rand_range(0, ((FIRE_EFFECT_COOLING * 10) / num_pixels) + 2);
        if(cooldown > s_fire_effect_heat[i]) {
        s_fire_effect_heat[i] = 0;
        } else {
        s_fire_effect_heat[i] = s_fire_effect_heat[i] - cooldown;
        }
    }

    for(int k = num_pixels - 1; k >= 2; k--) {
        s_fire_effect_heat[k] = (s_fire_effect_heat[k - 1] + s_fire_effect_heat[k - 2] + s_fire_effect_heat[k - 2]) / 3;
    }

    if((int) mgos_rand_range(0, 254) < FIRE_EFFECT_SPARKING) {
        int y = (int) mgos_rand_range(0, 6);
        s_fire_effect_heat[y] = s_fire_effect_heat[y] + (int) mgos_rand_range(159, 254);
    }

    node_neopixel_clear();
    for(int j = 0; j < num_pixels; j++) {
        int t192 = round((s_fire_effect_heat[j] / 255.0) * 191);
        int heatramp = t192 & 0x3F;
        heatramp <<= 2;
        heatramp = heatramp & 255;
        if(t192 > 0x80) {
        node_neopixel_set(j, 255, 255, heatramp);
        } else if(t192 > 0x40) {
        node_neopixel_set(j, 255, heatramp, 0);
        } else {
        node_neopixel_set(j, heatramp, 0, 0);
        }
    }

    node_neopixel_show();
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NVK_INCLUDE_EFFECT_FIRE_H_ */