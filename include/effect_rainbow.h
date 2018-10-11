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

#ifndef NVK_INCLUDE_EFFECT_RAINBOW_H_
#define NVK_INCLUDE_EFFECT_RAINBOW_H_

#ifdef __cplusplus
extern "C" {
#endif

static int s_rainbow_effect_counter = 0;
static int s_rainbow_cycle_effect_counter = 0;

static int wheel(int p) {
  p = 255 - p;
  if (p < 85) {
    return get_hex_color((255 - p * 3), 0, (p * 3));
  }
  if (p < 170) {
    p = p - 85;
    return get_hex_color(0, (p * 3), (255 - p * 3));
  }
  p = p - 170;
  return get_hex_color((p * 3), (255 - p * 3), 0);
}

void rainbow_effect(void *args) {
    (void) args;
    if(s_rainbow_effect_counter < 256) {
    int i = s_rainbow_effect_counter++;
    int num_pixels = mgos_sys_config_get_nodes_neopixel_pixels();
    node_neopixel_clear();
    for(int p = 0; p < num_pixels; p++) {
      int color = wheel((p + i) & 255);
      rgb_color c = get_rgb_color(color);
      node_neopixel_set(p, c.red, c.green, c.blue);
    }
    node_neopixel_show();
  } else {
    s_rainbow_effect_counter = 0;
  }
}

void rainbow_cycle_effect() {
  if(s_rainbow_cycle_effect_counter < 256 * 5) {
    int i = s_rainbow_cycle_effect_counter++;
    int num_pixels = mgos_sys_config_get_nodes_neopixel_pixels();
    node_neopixel_clear();
    for(int p = 0; p < num_pixels; p++) {
      int k = p * 256 / num_pixels;
      int color = wheel((k + i) & 255);
      rgb_color c = get_rgb_color(color);
      node_neopixel_set(p, c.red, c.green, c.blue);
    }
    node_neopixel_show();
  } else {
    s_rainbow_cycle_effect_counter = 0;
  }
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NVK_INCLUDE_EFFECT_RAINBOW_H_ */