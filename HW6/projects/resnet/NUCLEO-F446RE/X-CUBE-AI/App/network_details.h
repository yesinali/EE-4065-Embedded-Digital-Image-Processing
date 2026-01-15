/**
  ******************************************************************************
  * @file    network.h
  * @date    2026-01-15T18:41:25+0000
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
  .tensors = (const stai_tensor[36]) {
   { .size_bytes = 784, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 28, 28, 1}}, .scale = {1, (const float[1]){0.003921568859368563}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "serving_default_input0_output" },
   { .size_bytes = 12544, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 28, 28, 16}}, .scale = {1, (const float[1]){0.022536076605319977}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_0_output" },
   { .size_bytes = 14400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 30, 30, 16}}, .scale = {1, (const float[1]){0.022536076605319977}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_1_pad_before_output" },
   { .size_bytes = 12544, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 28, 28, 16}}, .scale = {1, (const float[1]){0.031599391251802444}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_1_output" },
   { .size_bytes = 14400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 30, 30, 16}}, .scale = {1, (const float[1]){0.031599391251802444}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_2_pad_before_output" },
   { .size_bytes = 12544, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 28, 28, 16}}, .scale = {1, (const float[1]){0.08001790195703506}}, .zeropoint = {1, (const int16_t[1]){-24}}, .name = "conv2d_2_output" },
   { .size_bytes = 12544, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 28, 28, 16}}, .scale = {1, (const float[1]){0.04742404446005821}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "eltwise_3_output" },
   { .size_bytes = 14400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 30, 30, 16}}, .scale = {1, (const float[1]){0.04742404446005821}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_4_pad_before_output" },
   { .size_bytes = 12544, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 28, 28, 16}}, .scale = {1, (const float[1]){0.03659798204898834}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_4_output" },
   { .size_bytes = 14400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 30, 30, 16}}, .scale = {1, (const float[1]){0.03659798204898834}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_5_pad_before_output" },
   { .size_bytes = 12544, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 28, 28, 16}}, .scale = {1, (const float[1]){0.09247681498527527}}, .zeropoint = {1, (const int16_t[1]){-42}}, .name = "conv2d_5_output" },
   { .size_bytes = 12544, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 28, 28, 16}}, .scale = {1, (const float[1]){0.06131451204419136}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "eltwise_6_output" },
   { .size_bytes = 6272, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 32}}, .scale = {1, (const float[1]){0.0602550134062767}}, .zeropoint = {1, (const int16_t[1]){-5}}, .name = "conv2d_9_output" },
   { .size_bytes = 6272, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 32}}, .scale = {1, (const float[1]){0.03198391944169998}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_7_output" },
   { .size_bytes = 8192, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 16, 16, 32}}, .scale = {1, (const float[1]){0.03198391944169998}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_8_pad_before_output" },
   { .size_bytes = 6272, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 32}}, .scale = {1, (const float[1]){0.0719042494893074}}, .zeropoint = {1, (const int16_t[1]){-23}}, .name = "conv2d_8_output" },
   { .size_bytes = 6272, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 32}}, .scale = {1, (const float[1]){0.04220288619399071}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "eltwise_10_output" },
   { .size_bytes = 8192, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 16, 16, 32}}, .scale = {1, (const float[1]){0.04220288619399071}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_11_pad_before_output" },
   { .size_bytes = 6272, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 32}}, .scale = {1, (const float[1]){0.029274458065629005}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_11_output" },
   { .size_bytes = 8192, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 16, 16, 32}}, .scale = {1, (const float[1]){0.029274458065629005}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_12_pad_before_output" },
   { .size_bytes = 6272, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 32}}, .scale = {1, (const float[1]){0.08221554756164551}}, .zeropoint = {1, (const int16_t[1]){-32}}, .name = "conv2d_12_output" },
   { .size_bytes = 6272, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 32}}, .scale = {1, (const float[1]){0.0511508472263813}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "eltwise_13_output" },
   { .size_bytes = 8192, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 16, 16, 32}}, .scale = {1, (const float[1]){0.0511508472263813}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_14_pad_before_output" },
   { .size_bytes = 3136, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 64}}, .scale = {1, (const float[1]){0.026751190423965454}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_14_output" },
   { .size_bytes = 5184, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 9, 9, 64}}, .scale = {1, (const float[1]){0.026751190423965454}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_15_pad_before_output" },
   { .size_bytes = 3136, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 64}}, .scale = {1, (const float[1]){0.06859904527664185}}, .zeropoint = {1, (const int16_t[1]){-25}}, .name = "conv2d_15_output" },
   { .size_bytes = 3136, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 64}}, .scale = {1, (const float[1]){0.05399082973599434}}, .zeropoint = {1, (const int16_t[1]){12}}, .name = "conv2d_16_output" },
   { .size_bytes = 3136, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 64}}, .scale = {1, (const float[1]){0.040968943387269974}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "eltwise_17_output" },
   { .size_bytes = 5184, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 9, 9, 64}}, .scale = {1, (const float[1]){0.040968943387269974}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_18_pad_before_output" },
   { .size_bytes = 3136, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 64}}, .scale = {1, (const float[1]){0.02445421740412712}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_18_output" },
   { .size_bytes = 5184, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 9, 9, 64}}, .scale = {1, (const float[1]){0.02445421740412712}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_19_pad_before_output" },
   { .size_bytes = 3136, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 64}}, .scale = {1, (const float[1]){0.08927126228809357}}, .zeropoint = {1, (const int16_t[1]){-44}}, .name = "conv2d_19_output" },
   { .size_bytes = 3136, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 64}}, .scale = {1, (const float[1]){0.05983635410666466}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "eltwise_20_output" },
   { .size_bytes = 64, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 64}}, .scale = {1, (const float[1]){0.018581321462988853}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "pool_21_output" },
   { .size_bytes = 10, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {2, (const int32_t[2]){1, 10}}, .scale = {1, (const float[1]){0.11622951924800873}}, .zeropoint = {1, (const int16_t[1]){-45}}, .name = "gemm_22_output" },
   { .size_bytes = 10, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {2, (const int32_t[2]){1, 10}}, .scale = {1, (const float[1]){0.00390625}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "nl_23_output" }
  },
  .nodes = (const stai_node_details[35]){
    {.id = 0, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){0}}, .output_tensors = {1, (const int32_t[1]){1}} }, /* conv2d_0 */
    {.id = 1, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){1}}, .output_tensors = {1, (const int32_t[1]){2}} }, /* conv2d_1_pad_before */
    {.id = 1, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){2}}, .output_tensors = {1, (const int32_t[1]){3}} }, /* conv2d_1 */
    {.id = 2, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){3}}, .output_tensors = {1, (const int32_t[1]){4}} }, /* conv2d_2_pad_before */
    {.id = 2, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){4}}, .output_tensors = {1, (const int32_t[1]){5}} }, /* conv2d_2 */
    {.id = 3, .type = AI_LAYER_ELTWISE_INTEGER_TYPE, .input_tensors = {2, (const int32_t[2]){5, 1}}, .output_tensors = {1, (const int32_t[1]){6}} }, /* eltwise_3 */
    {.id = 4, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){6}}, .output_tensors = {1, (const int32_t[1]){7}} }, /* conv2d_4_pad_before */
    {.id = 4, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){7}}, .output_tensors = {1, (const int32_t[1]){8}} }, /* conv2d_4 */
    {.id = 5, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){8}}, .output_tensors = {1, (const int32_t[1]){9}} }, /* conv2d_5_pad_before */
    {.id = 5, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){9}}, .output_tensors = {1, (const int32_t[1]){10}} }, /* conv2d_5 */
    {.id = 6, .type = AI_LAYER_ELTWISE_INTEGER_TYPE, .input_tensors = {2, (const int32_t[2]){10, 6}}, .output_tensors = {1, (const int32_t[1]){11}} }, /* eltwise_6 */
    {.id = 9, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){11}}, .output_tensors = {1, (const int32_t[1]){12}} }, /* conv2d_9 */
    {.id = 7, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){11}}, .output_tensors = {1, (const int32_t[1]){13}} }, /* conv2d_7 */
    {.id = 8, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){13}}, .output_tensors = {1, (const int32_t[1]){14}} }, /* conv2d_8_pad_before */
    {.id = 8, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){14}}, .output_tensors = {1, (const int32_t[1]){15}} }, /* conv2d_8 */
    {.id = 10, .type = AI_LAYER_ELTWISE_INTEGER_TYPE, .input_tensors = {2, (const int32_t[2]){15, 12}}, .output_tensors = {1, (const int32_t[1]){16}} }, /* eltwise_10 */
    {.id = 11, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){16}}, .output_tensors = {1, (const int32_t[1]){17}} }, /* conv2d_11_pad_before */
    {.id = 11, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){17}}, .output_tensors = {1, (const int32_t[1]){18}} }, /* conv2d_11 */
    {.id = 12, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){18}}, .output_tensors = {1, (const int32_t[1]){19}} }, /* conv2d_12_pad_before */
    {.id = 12, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){19}}, .output_tensors = {1, (const int32_t[1]){20}} }, /* conv2d_12 */
    {.id = 13, .type = AI_LAYER_ELTWISE_INTEGER_TYPE, .input_tensors = {2, (const int32_t[2]){20, 16}}, .output_tensors = {1, (const int32_t[1]){21}} }, /* eltwise_13 */
    {.id = 14, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){21}}, .output_tensors = {1, (const int32_t[1]){22}} }, /* conv2d_14_pad_before */
    {.id = 14, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){22}}, .output_tensors = {1, (const int32_t[1]){23}} }, /* conv2d_14 */
    {.id = 15, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){23}}, .output_tensors = {1, (const int32_t[1]){24}} }, /* conv2d_15_pad_before */
    {.id = 15, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){24}}, .output_tensors = {1, (const int32_t[1]){25}} }, /* conv2d_15 */
    {.id = 16, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){21}}, .output_tensors = {1, (const int32_t[1]){26}} }, /* conv2d_16 */
    {.id = 17, .type = AI_LAYER_ELTWISE_INTEGER_TYPE, .input_tensors = {2, (const int32_t[2]){25, 26}}, .output_tensors = {1, (const int32_t[1]){27}} }, /* eltwise_17 */
    {.id = 18, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){27}}, .output_tensors = {1, (const int32_t[1]){28}} }, /* conv2d_18_pad_before */
    {.id = 18, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){28}}, .output_tensors = {1, (const int32_t[1]){29}} }, /* conv2d_18 */
    {.id = 19, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){29}}, .output_tensors = {1, (const int32_t[1]){30}} }, /* conv2d_19_pad_before */
    {.id = 19, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){30}}, .output_tensors = {1, (const int32_t[1]){31}} }, /* conv2d_19 */
    {.id = 20, .type = AI_LAYER_ELTWISE_INTEGER_TYPE, .input_tensors = {2, (const int32_t[2]){31, 27}}, .output_tensors = {1, (const int32_t[1]){32}} }, /* eltwise_20 */
    {.id = 21, .type = AI_LAYER_POOL_TYPE, .input_tensors = {1, (const int32_t[1]){32}}, .output_tensors = {1, (const int32_t[1]){33}} }, /* pool_21 */
    {.id = 22, .type = AI_LAYER_DENSE_TYPE, .input_tensors = {1, (const int32_t[1]){33}}, .output_tensors = {1, (const int32_t[1]){34}} }, /* gemm_22 */
    {.id = 23, .type = AI_LAYER_SM_TYPE, .input_tensors = {1, (const int32_t[1]){34}}, .output_tensors = {1, (const int32_t[1]){35}} } /* nl_23 */
  },
  .n_nodes = 35
};
#endif

