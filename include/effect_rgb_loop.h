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

#ifndef NVK_INCLUDE_EFFECT_RGB_LOOP_H_
#define NVK_INCLUDE_EFFECT_RGB_LOOP_H_

#ifdef __cplusplus
extern "C" {
#endif

static int s_rgb_loop_effect_counter = 0;
static int s_rbg_loop_effect_iteration = 0;
static bool s_rgb_loop_effect_dir = true;

void rbg_loop_effect(void *args) {
    (void) args;
    if (s_rbg_loop_effect_iteration > 3) {
        s_rbg_loop_effect_iteration = 0;
    }

    uint8_t k = s_rgb_loop_effect_counter & 255;
    if (s_rgb_loop_effect_dir)
    {
        s_rgb_loop_effect_dir = k < 254;
        s_rgb_loop_effect_counter++;
    } else {
        s_rgb_loop_effect_dir = k < 2;
        s_rgb_loop_effect_counter--;
        if(s_rgb_loop_effect_dir)
        {
            s_rbg_loop_effect_iteration++;
        }
    }
    int color = 0xFF0000;
    switch(s_rbg_loop_effect_iteration)
    {
        case 1:
        color = color >> 8;
        break;
        case 2:
        color = color >> 16;
        break;
    }
    rgb_color c = get_rgb_color(color);
    node_neopixel_set_all_pixels(c);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NVK_INCLUDE_EFFECT_RGB_LOOP_H_ */