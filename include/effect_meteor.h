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

#ifndef NVK_INCLUDE_EFFECT_METEOR_H_
#define NVK_INCLUDE_EFFECT_METEOR_H_

#ifdef __cplusplus
extern "C" {
#endif

static int s_meteor_effect_counter = 0;

void fade_to_black(int pixel, int fade) {
    rgb_color color = node_neopixel_get_pixel_color(pixel);
    int f = (int)(fade / 256);
    color.red = (color.red <= 10) ? 0 : (int)(color.red - (color.red * f));
    color.green = (color.green <= 10) ? 0 : (int)(color.green - (color.green * f));
    color.blue = (color.blue <= 10) ? 0 : (int)(color.blue - (color.blue * f));
    node_neopixel_set(pixel, color.red, color.green, color.blue);
}

void meteor_effect() {
    int num_pixels = mgos_sys_config_get_nodes_neopixel_pixels();
    int meteor_size = mgos_sys_config_get_effects_meteor_size();
    bool random_decay = mgos_sys_config_get_effects_meteor_random();
    int trail_decay = mgos_sys_config_get_effects_meteor_trail();
    
    node_neopixel_turn_off();

    for(int j = 0; j < num_pixels; j++) {
        if(!(random_decay) || (int) mgos_rand_range(0, 5) > 5) {
            fade_to_black(j, trail_decay);
        }
    }
    for(int j = 0; j < meteor_size; j++) {
        int p = s_meteor_effect_counter - j;
        if((p < num_pixels) && (p >= 0)) {
            node_neopixel_set(p, 0xFF, 0xFF, 0xFF);
        }
    }

    node_neopixel_show();
    
    if (++s_meteor_effect_counter >= num_pixels * 2) {
        s_meteor_effect_counter = 0;
    }
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NVK_INCLUDE_EFFECT_METEOR_H_ */