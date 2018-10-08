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
#include <stdint.h>

#ifndef NVK_LIBS_NODES_INCLUDE_NVK_NODES_H_
#define NVK_LIBS_NODES_INCLUDE_NVK_NODES_H_

#ifdef __cplusplus
extern "C" {
#endif

struct node_range_values {
    int current; // Current value
    int min; // Min range value
    int max; // Max range value
};

enum node_out_range_event_type {
    ABOVE_OF_RANGE = 1,
    BELOW_OF_RANGE = 2
};

typedef void (*node_out_range_handler_t)(enum node_out_range_event_type ev, struct node_range_values *values, void *user_data);
typedef void (*node_on_range_handler_t)(struct node_range_values *values, void *user_data);

typedef void (*node_switch_handler_t)(int value, void *user_data);

/* Initialize Nodes */
bool nodes_init();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NVK_LIBS_NODES_INCLUDE_NVK_NODES_H_ */