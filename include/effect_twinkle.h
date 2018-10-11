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

#ifndef NVK_INCLUDE_EFFECT_TWINKLE_H_
#define NVK_INCLUDE_EFFECT_TWINKLE_H_

#ifdef __cplusplus
extern "C" {
#endif

static int s_twinkle_effect_counter = 0;
static int s_twinkle_random_effect_counter = 0;

void twinkle_effect(void *args) {
    neopixel_effect_data *user_twinkle_data = (neopixel_effect_data*) args;
    rgb_color c = get_rgb_color(user_twinkle_data->color);
    int num_pixels = mgos_sys_config_get_nodes_neopixel_pixels();

    if (s_twinkle_effect_counter < num_pixels / 3) {
        int p = (int) mgos_rand_range(0, num_pixels - 1);
        node_neopixel_set(p, c.red, c.green, c.blue);
        node_neopixel_show();
        s_twinkle_effect_counter++;
    } else {
        s_twinkle_effect_counter = 0;
        node_neopixel_turn_off();
    }

}

void twinkle_random_effect() {
  int num_pixels = mgos_sys_config_get_nodes_neopixel_pixels();

  if (s_twinkle_random_effect_counter < num_pixels / 3) {
    int p = (int) mgos_rand_range(0, num_pixels - 1);
    int r = (int) mgos_rand_range(0, 254);
    int g = (int) mgos_rand_range(0, 254);
    int b = (int) mgos_rand_range(0, 254);
    node_neopixel_set(p, r, g, b);
    node_neopixel_show();
    s_twinkle_random_effect_counter++;
  } else {
    s_twinkle_random_effect_counter = 0;
    node_neopixel_turn_off();
  }
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NVK_INCLUDE_EFFECT_TWINKLE_H_ */