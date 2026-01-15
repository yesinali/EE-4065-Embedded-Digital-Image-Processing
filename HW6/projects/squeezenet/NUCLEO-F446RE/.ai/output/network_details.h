/**
  ******************************************************************************
  * @file    network.h
  * @date    2026-01-15T18:04:57+0000
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
  .tensors = (const stai_tensor[24]) {
   { .size_bytes = 784, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 28, 28, 1}}, .scale = {1, (const float[1]){0.003921568859368563}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "serving_default_input0_output" },
   { .size_bytes = 6272, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 32}}, .scale = {1, (const float[1]){0.006514836568385363}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_0_output" },
   { .size_bytes = 1568, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 8}}, .scale = {1, (const float[1]){0.030764872208237648}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_2_output" },
   { .size_bytes = 3136, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 16}}, .scale = {1, (const float[1]){0.05573930963873863}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_4_output" },
   { .size_bytes = 3136, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 16}}, .scale = {1, (const float[1]){0.05573930963873863}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_3_output" },
   { .size_bytes = 6272, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 32}}, .scale = {1, (const float[1]){0.05573930963873863}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "concat_5_output" },
   { .size_bytes = 1568, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 8}}, .scale = {1, (const float[1]){0.04966450110077858}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_6_output" },
   { .size_bytes = 3136, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 16}}, .scale = {1, (const float[1]){0.11481409519910812}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_8_output" },
   { .size_bytes = 3136, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 16}}, .scale = {1, (const float[1]){0.11481409519910812}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_7_output" },
   { .size_bytes = 6272, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 32}}, .scale = {1, (const float[1]){0.11481409519910812}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "concat_9_output" },
   { .size_bytes = 1568, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 32}}, .scale = {1, (const float[1]){0.11481409519910812}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "pool_10_output" },
   { .size_bytes = 784, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 16}}, .scale = {1, (const float[1]){0.14174887537956238}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_11_output" },
   { .size_bytes = 1296, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 9, 9, 16}}, .scale = {1, (const float[1]){0.14174887537956238}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_13_pad_before_output" },
   { .size_bytes = 1568, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 32}}, .scale = {1, (const float[1]){0.3459133505821228}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_13_output" },
   { .size_bytes = 1568, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 32}}, .scale = {1, (const float[1]){0.3459133505821228}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_12_output" },
   { .size_bytes = 3136, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 64}}, .scale = {1, (const float[1]){0.3459133505821228}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "concat_14_output" },
   { .size_bytes = 784, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 16}}, .scale = {1, (const float[1]){0.41464847326278687}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_15_output" },
   { .size_bytes = 1296, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 9, 9, 16}}, .scale = {1, (const float[1]){0.41464847326278687}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_17_pad_before_output" },
   { .size_bytes = 1568, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 32}}, .scale = {1, (const float[1]){0.5669575333595276}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_17_output" },
   { .size_bytes = 1568, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 32}}, .scale = {1, (const float[1]){0.5669575333595276}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_16_output" },
   { .size_bytes = 3136, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 64}}, .scale = {1, (const float[1]){0.5669575333595276}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "concat_18_output" },
   { .size_bytes = 490, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 10}}, .scale = {1, (const float[1]){0.6213035583496094}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_19_output" },
   { .size_bytes = 10, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 10}}, .scale = {1, (const float[1]){0.17450441420078278}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "pool_20_output" },
   { .size_bytes = 10, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {2, (const int32_t[2]){1, 10}}, .scale = {1, (const float[1]){0.00390625}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "nl_21_output" }
  },
  .nodes = (const stai_node_details[23]){
    {.id = 1, .type = AI_LAYER_OPTIMIZED_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){0}}, .output_tensors = {1, (const int32_t[1]){1}} }, /* conv2d_0 */
    {.id = 2, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){1}}, .output_tensors = {1, (const int32_t[1]){2}} }, /* conv2d_2 */
    {.id = 4, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){2}}, .output_tensors = {1, (const int32_t[1]){3}} }, /* conv2d_4 */
    {.id = 3, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){2}}, .output_tensors = {1, (const int32_t[1]){4}} }, /* conv2d_3 */
    {.id = 5, .type = AI_LAYER_CONCAT_TYPE, .input_tensors = {2, (const int32_t[2]){4, 3}}, .output_tensors = {1, (const int32_t[1]){5}} }, /* concat_5 */
    {.id = 6, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){5}}, .output_tensors = {1, (const int32_t[1]){6}} }, /* conv2d_6 */
    {.id = 8, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){6}}, .output_tensors = {1, (const int32_t[1]){7}} }, /* conv2d_8 */
    {.id = 7, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){6}}, .output_tensors = {1, (const int32_t[1]){8}} }, /* conv2d_7 */
    {.id = 9, .type = AI_LAYER_CONCAT_TYPE, .input_tensors = {2, (const int32_t[2]){8, 7}}, .output_tensors = {1, (const int32_t[1]){9}} }, /* concat_9 */
    {.id = 10, .type = AI_LAYER_POOL_TYPE, .input_tensors = {1, (const int32_t[1]){9}}, .output_tensors = {1, (const int32_t[1]){10}} }, /* pool_10 */
    {.id = 11, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){10}}, .output_tensors = {1, (const int32_t[1]){11}} }, /* conv2d_11 */
    {.id = 13, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){11}}, .output_tensors = {1, (const int32_t[1]){12}} }, /* conv2d_13_pad_before */
    {.id = 13, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){12}}, .output_tensors = {1, (const int32_t[1]){13}} }, /* conv2d_13 */
    {.id = 12, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){11}}, .output_tensors = {1, (const int32_t[1]){14}} }, /* conv2d_12 */
    {.id = 14, .type = AI_LAYER_CONCAT_TYPE, .input_tensors = {2, (const int32_t[2]){14, 13}}, .output_tensors = {1, (const int32_t[1]){15}} }, /* concat_14 */
    {.id = 15, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){15}}, .output_tensors = {1, (const int32_t[1]){16}} }, /* conv2d_15 */
    {.id = 17, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){16}}, .output_tensors = {1, (const int32_t[1]){17}} }, /* conv2d_17_pad_before */
    {.id = 17, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){17}}, .output_tensors = {1, (const int32_t[1]){18}} }, /* conv2d_17 */
    {.id = 16, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){16}}, .output_tensors = {1, (const int32_t[1]){19}} }, /* conv2d_16 */
    {.id = 18, .type = AI_LAYER_CONCAT_TYPE, .input_tensors = {2, (const int32_t[2]){19, 18}}, .output_tensors = {1, (const int32_t[1]){20}} }, /* concat_18 */
    {.id = 19, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){20}}, .output_tensors = {1, (const int32_t[1]){21}} }, /* conv2d_19 */
    {.id = 20, .type = AI_LAYER_POOL_TYPE, .input_tensors = {1, (const int32_t[1]){21}}, .output_tensors = {1, (const int32_t[1]){22}} }, /* pool_20 */
    {.id = 21, .type = AI_LAYER_SM_TYPE, .input_tensors = {1, (const int32_t[1]){22}}, .output_tensors = {1, (const int32_t[1]){23}} } /* nl_21 */
  },
  .n_nodes = 23
};
#endif

