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

#include <stdint.h>
#include "nvk_nodes.h"

#ifndef NVK_LIBS_NODES_INCLUDE_NVK_NODES_NEOPIXEL_H_
#define NVK_LIBS_NODES_INCLUDE_NVK_NODES_NEOPIXEL_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rgb_color {
    int red;
    int green;
    int blue;
} rgb_color;

rgb_color get_rgb_color(int);

int get_hex_color(int, int, int);

void node_neopixel_turn_off();

void node_neopixel_turn_on(rgb_color);

void node_neopixel_set_all_pixels(rgb_color);

void node_neopixel_set_pixel(int, rgb_color);

void node_neopixel_set_brightness(int);

void node_neopixel_clear();

void node_neopixel_set(int, int, int, int);

void node_neopixe_set_all(int, int, int);

void node_neopixel_show();

bool node_neopixel_init();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NVK_LIBS_NODES_INCLUDE_NVK_NODES_NEOPIXEL_H_ */