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

#include <stdbool.h>
#include "mgos.h"
#include "nvk_nodes.h"
#include "nvk_nodes_dht.h"
#include "nvk_nodes_pir.h"
#include "nvk_nodes_photoresistor.h"
#include "nvk_nodes_neopixel.h"

/* Initialize Nodes */
bool nodes_init() {
    node_dht_init(); // TODO
    node_pir_init();
    node_photoresistor_init();
    node_neopixel_init();
    return true;
}
