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

#ifndef NVK_INCLUDE_EFFECT_FADE_H_
#define NVK_INCLUDE_EFFECT_FADE_H_

#ifdef __cplusplus
extern "C" {
#endif

static int s_fade_effect_counter = 0;
static int s_fade_effect_iteration = 0;
static bool s_fade_effect_dir = true;

void fade_effect(void *args) {
    (void) args;
    if(s_fade_effect_counter >= 3) {
        s_fade_effect_counter = 0;
      }

    if(s_fade_effect_dir) {
        s_fade_effect_dir = s_fade_effect_iteration < 254;
        s_fade_effect_iteration++;
    } else {
        s_fade_effect_dir = s_fade_effect_iteration < 2;
        s_fade_effect_iteration--;
        if(s_fade_effect_dir) {
        s_fade_effect_counter++;
        }
    }
    int color = s_fade_effect_iteration & 255;
    switch(s_fade_effect_counter) {
        case 0:
        color = color << 16;
        break;
        case 1:
        color = color << 8;
        break;
    }
    rgb_color c = get_rgb_color(color);
    node_neopixel_set_all_pixels(c);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NVK_INCLUDE_EFFECT_FADE_H_ */