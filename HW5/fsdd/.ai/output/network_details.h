/**
  ******************************************************************************
  * @file    network.h
  * @date    2026-01-01T20:54:05+0000
  * @brief   ST.AI Tool Automatic Code Generator for Embedded NN computing
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  ******************************************************************************
  */
#ifndef STAI_NETWORK_DETAILS_H
#define STAI_NETWORK_DETAILS_H

#include "stai.h"
#include "layers.h"

const stai_network_details g_network_details = {
  .tensors = (const stai_tensor[7]) {
   { .size_bytes = 104, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {2, (const int32_t[2]){1, 26}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "input_0_output" },
   { .size_bytes = 400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {2, (const int32_t[2]){1, 100}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "dense_dense_output" },
   { .size_bytes = 400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {2, (const int32_t[2]){1, 100}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "dense_output" },
   { .size_bytes = 400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {2, (const int32_t[2]){1, 100}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "dense_1_dense_output" },
   { .size_bytes = 400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {2, (const int32_t[2]){1, 100}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "dense_1_output" },
   { .size_bytes = 40, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {2, (const int32_t[2]){1, 10}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "dense_2_dense_output" },
   { .size_bytes = 40, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {2, (const int32_t[2]){1, 10}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "dense_2_output" }
  },
  .nodes = (const stai_node_details[6]){
    {.id = 0, .type = AI_LAYER_DENSE_TYPE, .input_tensors = {1, (const int32_t[1]){0}}, .output_tensors = {1, (const int32_t[1]){1}} }, /* dense_dense */
    {.id = 0, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){1}}, .output_tensors = {1, (const int32_t[1]){2}} }, /* dense */
    {.id = 1, .type = AI_LAYER_DENSE_TYPE, .input_tensors = {1, (const int32_t[1]){2}}, .output_tensors = {1, (const int32_t[1]){3}} }, /* dense_1_dense */
    {.id = 1, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){3}}, .output_tensors = {1, (const int32_t[1]){4}} }, /* dense_1 */
    {.id = 2, .type = AI_LAYER_DENSE_TYPE, .input_tensors = {1, (const int32_t[1]){4}}, .output_tensors = {1, (const int32_t[1]){5}} }, /* dense_2_dense */
    {.id = 2, .type = AI_LAYER_SM_TYPE, .input_tensors = {1, (const int32_t[1]){5}}, .output_tensors = {1, (const int32_t[1]){6}} } /* dense_2 */
  },
  .n_nodes = 6
};
#endif

