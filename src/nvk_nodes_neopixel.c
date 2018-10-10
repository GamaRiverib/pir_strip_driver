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

#include "mgos.h"
#include "mgos_neopixel.h"
#include "nvk_nodes_neopixel.h"

static struct mgos_neopixel *s_node_neopixel = NULL;

#define NODE_NEOPIXEL_ORDER MGOS_NEOPIXEL_ORDER_GRB

rgb_color get_rgb_color(int color) {
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = color & 0xFF;
    rgb_color c = { r, g, b };
    return c;
}

int get_hex_color(int r, int g, int b) {
    int h = 0x000000;
    h |= r << 16;
    h |= g << 8;
    h |= b;
    return h;
}

void node_neopixel_set_all_pixels(rgb_color rgb) {
    mgos_neopixel_clear(s_node_neopixel);
    int num_pixels = mgos_sys_config_get_nodes_neopixel_pixels();
    for(int p = 0; p < num_pixels; p++) {
        mgos_neopixel_set(s_node_neopixel, p, rgb.red, rgb.green, rgb.blue);
    }
    mgos_neopixel_show(s_node_neopixel);
}

void node_neopixel_set_pixel(int pixel, rgb_color rgb) {
    mgos_neopixel_clear(s_node_neopixel);
    mgos_neopixel_set(s_node_neopixel, pixel, rgb.red, rgb.green, rgb.blue);
    mgos_neopixel_show(s_node_neopixel);
}

void node_neopixel_turn_off() {
    rgb_color black = get_rgb_color(0);
    node_neopixel_set_all_pixels(black);
}

void node_neopixel_turn_on(rgb_color rgb) {
    node_neopixel_set_all_pixels(rgb);
}

void node_neopixel_set_brightness(int brightness) {
    brightness = brightness & 0xFF;
    int hex = get_hex_color(brightness, brightness, brightness);
    rgb_color c = get_rgb_color(hex);
    node_neopixel_set_all_pixels(c);
}

void node_neopixel_clear() {
    mgos_neopixel_clear(s_node_neopixel);
}

void node_neopixel_set(int pixel, int r, int g, int b) {
    mgos_neopixel_set(s_node_neopixel, pixel, r, g, b);
}

void node_neopixe_set_all(int r, int g, int b) {
    int num_pixels = mgos_sys_config_get_nodes_neopixel_pixels();
    for(int p = 0; p < num_pixels; p++) {
        mgos_neopixel_set(s_node_neopixel, p, r, g, b);
    }
}

void node_neopixel_show() {
    mgos_neopixel_show(s_node_neopixel);
}

bool node_neopixel_init() {
    bool enabled = mgos_sys_config_get_nodes_neopixel_enable();
    if (enabled) {
        int num_pixels = mgos_sys_config_get_nodes_neopixel_pixels();
        int pin = mgos_sys_config_get_nodes_neopixel_pin();
        s_node_neopixel = mgos_neopixel_create(pin, num_pixels, NODE_NEOPIXEL_ORDER);
    }
    return enabled;
}