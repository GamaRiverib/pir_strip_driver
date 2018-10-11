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

#ifndef NVK_INCLUDE_EFFECT_DEFAULT_H_
#define NVK_INCLUDE_EFFECT_DEFAULT_H_

#ifdef __cplusplus
extern "C" {
#endif

static int s_first_effect_counter = 0;

void first_effect() {
    int num_pixels = mgos_sys_config_get_nodes_neopixel_pixels();
    int p = (s_first_effect_counter++) % num_pixels;
    int r = s_first_effect_counter % 255;
    int g = (s_first_effect_counter * 2) % 255;
    int b = (s_first_effect_counter * s_first_effect_counter) % 255;
    int h = get_hex_color(r, g, b);
    rgb_color c = get_rgb_color(h);
    node_neopixel_set_pixel(p, c);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NVK_INCLUDE_EFFECT_DEFAULT_H_ */