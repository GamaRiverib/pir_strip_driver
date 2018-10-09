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
#include "mgos_rpc.h"

#ifndef NVK_LIBS_NODES_INCLUDE_NVK_NODES_DHT_H_
#define NVK_LIBS_NODES_INCLUDE_NVK_NODES_DHT_H_

#ifdef __cplusplus
extern "C" {
#endif

bool node_dht_init();
void node_dht_sampling_handler(void *dht);
void node_dht_tele_handler(void *dht);
void node_dht_rpc_handler(struct mg_rpc_request_info *ri, const char *args, const char *src, void *dht);
void node_dht_set_temp_on_range_handler(node_on_range_handler_t func);
void node_dht_set_temp_out_range_handler(node_out_range_handler_t func);
void node_dht_set_humd_on_range_handler(node_on_range_handler_t func);
void node_dht_set_humd_out_range_handler(node_out_range_handler_t func);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NVK_LIBS_NODES_INCLUDE_NVK_NODES_DHT_H_ */