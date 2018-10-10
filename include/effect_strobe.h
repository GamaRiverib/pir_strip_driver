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

#ifndef NVK_INCLUDE_EFFECT_STROBE_H_
#define NVK_INCLUDE_EFFECT_STROBE_H_

#ifdef __cplusplus
extern "C" {
#endif

static bool s_strobe_effect_state = true;
static rgb_color rgb_color_black = { 0, 0, 0 };

typedef struct strobe_data {
    int color;
} strobe_data;

void strobe_effect(void *args) {
    strobe_data *user_strobe_data = (strobe_data*) args;
    rgb_color c = get_rgb_color(user_strobe_data->color);
    if(s_strobe_effect_state) {
        node_neopixel_set_all_pixels(c);
    } else {
        node_neopixel_set_all_pixels(rgb_color_black);
    }
    s_strobe_effect_state = !s_strobe_effect_state;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NVK_INCLUDE_EFFECT_STROBE_H_ */