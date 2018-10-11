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

#ifndef NVK_INCLUDE_EFFECT_FLASH_H_
#define NVK_INCLUDE_EFFECT_FLASH_H_

#ifdef __cplusplus
extern "C" {
#endif

static int s_flash_effect_counter = 0;
static rgb_color s_flash_effect_colors[] = {
    { 0x00, 0xFF, 0x00 }, { 0x00, 0x00, 0xFF }, { 0xFF, 0x00, 0x00 },
    { 0xFF, 0x7F, 0x00 }, { 0x50, 0x30, 0x50 }, { 0x4D, 0x4D, 0xFF },
    { 0x8F, 0x24, 0x24 }, { 0x32, 0x32, 0xCC }, { 0x24, 0x6C, 0x8F },
    { 0xFF, 0xFF, 0x00 }, { 0x00, 0xFF, 0xFF }, { 0xBD, 0x8F, 0x8F },
    { 0x7F, 0xFF, 0x00 }, { 0x8D, 0x78, 0x24 }, { 0xFF, 0x6E, 0xC7 },
    { 0xDE, 0x94, 0xFA }, { 0x9F, 0x9F, 0x5F }, { 0x6F, 0x43, 0x43 }
};

static int s_flash_effect_colors_size = sizeof(s_flash_effect_colors) / sizeof(rgb_color);

void flash_effect(void *args) {
    (void) args;
    if(s_flash_effect_counter >= s_flash_effect_colors_size)
    {
        s_flash_effect_counter = 0;
    }
    node_neopixel_set_all_pixels(s_flash_effect_colors[s_flash_effect_counter++]);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NVK_INCLUDE_EFFECT_FLASH_H_ */