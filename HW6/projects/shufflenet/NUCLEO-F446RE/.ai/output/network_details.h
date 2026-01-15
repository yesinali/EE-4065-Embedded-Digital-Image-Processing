/**
  ******************************************************************************
  * @file    network.h
  * @date    2026-01-15T18:50:43+0000
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
   { .size_bytes = 18816, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 28, 28, 24}}, .scale = {1, (const float[1]){0.05027042329311371}}, .zeropoint = {1, (const int16_t[1]){-1}}, .name = "conv2d_0_output" },
   { .size_bytes = 18816, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 28, 28, 24}}, .scale = {1, (const float[1]){0.05027042329311371}}, .zeropoint = {1, (const int16_t[1]){-1}}, .name = "nl_0_nl_output" },
   { .size_bytes = 4704, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 24}}, .scale = {1, (const float[1]){0.05027042329311371}}, .zeropoint = {1, (const int16_t[1]){-1}}, .name = "pool_13_output" },
   { .size_bytes = 9408, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 28, 28, 12}}, .scale = {1, (const float[1]){0.02701232023537159}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_1_output" },
   { .size_bytes = 9408, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {5, (const int32_t[5]){1, 28, 28, 6, 2}}, .scale = {1, (const float[1]){0.02701232023537159}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "transpose_6_output" },
   { .size_bytes = 10800, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 30, 30, 12}}, .scale = {1, (const float[1]){0.02701232023537159}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_11_pad_before_output" },
   { .size_bytes = 2352, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 12}}, .scale = {1, (const float[1]){0.057651445269584656}}, .zeropoint = {1, (const int16_t[1]){11}}, .name = "conv2d_11_output" },
   { .size_bytes = 4704, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 24}}, .scale = {1, (const float[1]){0.05027042329311371}}, .zeropoint = {1, (const int16_t[1]){-1}}, .name = "conv2d_12_output" },
   { .size_bytes = 9408, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 48}}, .scale = {1, (const float[1]){0.05027042329311371}}, .zeropoint = {1, (const int16_t[1]){-1}}, .name = "concat_14_output" },
   { .size_bytes = 9408, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 48}}, .scale = {1, (const float[1]){0.025288520380854607}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "nl_15_output" },
   { .size_bytes = 2352, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 12}}, .scale = {1, (const float[1]){0.025270722806453705}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_16_output" },
   { .size_bytes = 2352, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {5, (const int32_t[5]){1, 14, 14, 6, 2}}, .scale = {1, (const float[1]){0.025270722806453705}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "transpose_21_output" },
   { .size_bytes = 3072, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 16, 16, 12}}, .scale = {1, (const float[1]){0.025270722806453705}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_26_pad_before_output" },
   { .size_bytes = 2352, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 12}}, .scale = {1, (const float[1]){0.04860549420118332}}, .zeropoint = {1, (const int16_t[1]){11}}, .name = "conv2d_26_output" },
   { .size_bytes = 9408, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 48}}, .scale = {1, (const float[1]){0.06529568135738373}}, .zeropoint = {1, (const int16_t[1]){-10}}, .name = "conv2d_27_output" },
   { .size_bytes = 9408, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 48}}, .scale = {1, (const float[1]){0.045860446989536285}}, .zeropoint = {1, (const int16_t[1]){14}}, .name = "eltwise_28_output" },
   { .size_bytes = 9408, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 48}}, .scale = {1, (const float[1]){0.045860446989536285}}, .zeropoint = {1, (const int16_t[1]){14}}, .name = "nl_28_nl_output" },
   { .size_bytes = 2352, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 48}}, .scale = {1, (const float[1]){0.045860446989536285}}, .zeropoint = {1, (const int16_t[1]){14}}, .name = "pool_41_output" },
   { .size_bytes = 4704, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 24}}, .scale = {1, (const float[1]){0.02582520991563797}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_29_output" },
   { .size_bytes = 4704, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {5, (const int32_t[5]){1, 14, 14, 12, 2}}, .scale = {1, (const float[1]){0.02582520991563797}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "transpose_34_output" },
   { .size_bytes = 6144, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 16, 16, 24}}, .scale = {1, (const float[1]){0.02582520991563797}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_39_pad_before_output" },
   { .size_bytes = 1176, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 24}}, .scale = {1, (const float[1]){0.05677644908428192}}, .zeropoint = {1, (const int16_t[1]){11}}, .name = "conv2d_39_output" },
   { .size_bytes = 2352, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 48}}, .scale = {1, (const float[1]){0.045860446989536285}}, .zeropoint = {1, (const int16_t[1]){14}}, .name = "conv2d_40_output" },
   { .size_bytes = 4704, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 96}}, .scale = {1, (const float[1]){0.045860446989536285}}, .zeropoint = {1, (const int16_t[1]){14}}, .name = "concat_42_output" },
   { .size_bytes = 4704, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 96}}, .scale = {1, (const float[1]){0.020261414349079132}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "nl_43_output" },
   { .size_bytes = 1176, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 24}}, .scale = {1, (const float[1]){0.023730019107460976}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_44_output" },
   { .size_bytes = 1176, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {5, (const int32_t[5]){1, 7, 7, 12, 2}}, .scale = {1, (const float[1]){0.023730019107460976}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "transpose_49_output" },
   { .size_bytes = 1944, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 9, 9, 24}}, .scale = {1, (const float[1]){0.023730019107460976}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_54_pad_before_output" },
   { .size_bytes = 1176, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 24}}, .scale = {1, (const float[1]){0.05613403022289276}}, .zeropoint = {1, (const int16_t[1]){0}}, .name = "conv2d_54_output" },
   { .size_bytes = 4704, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 96}}, .scale = {1, (const float[1]){0.06528100371360779}}, .zeropoint = {1, (const int16_t[1]){4}}, .name = "conv2d_55_output" },
   { .size_bytes = 4704, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 96}}, .scale = {1, (const float[1]){0.03139876574277878}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "eltwise_56_output" },
   { .size_bytes = 6272, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 128}}, .scale = {1, (const float[1]){0.04918375983834267}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_57_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 128}}, .scale = {1, (const float[1]){0.011819638311862946}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "pool_58_output" },
   { .size_bytes = 10, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {2, (const int32_t[2]){1, 10}}, .scale = {1, (const float[1]){0.15971285104751587}}, .zeropoint = {1, (const int16_t[1]){4}}, .name = "gemm_59_output" },
   { .size_bytes = 10, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {2, (const int32_t[2]){1, 10}}, .scale = {1, (const float[1]){0.00390625}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "nl_60_output" }
  },
  .nodes = (const stai_node_details[35]){
    {.id = 0, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){0}}, .output_tensors = {1, (const int32_t[1]){1}} }, /* conv2d_0 */
    {.id = 0, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){1}}, .output_tensors = {1, (const int32_t[1]){2}} }, /* nl_0_nl */
    {.id = 13, .type = AI_LAYER_POOL_TYPE, .input_tensors = {1, (const int32_t[1]){2}}, .output_tensors = {1, (const int32_t[1]){3}} }, /* pool_13 */
    {.id = 1, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){2}}, .output_tensors = {1, (const int32_t[1]){4}} }, /* conv2d_1 */
    {.id = 6, .type = AI_LAYER_TRANSPOSE_TYPE, .input_tensors = {1, (const int32_t[1]){4}}, .output_tensors = {1, (const int32_t[1]){5}} }, /* transpose_6 */
    {.id = 11, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){5}}, .output_tensors = {1, (const int32_t[1]){6}} }, /* conv2d_11_pad_before */
    {.id = 11, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){6}}, .output_tensors = {1, (const int32_t[1]){7}} }, /* conv2d_11 */
    {.id = 12, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){7}}, .output_tensors = {1, (const int32_t[1]){8}} }, /* conv2d_12 */
    {.id = 14, .type = AI_LAYER_CONCAT_TYPE, .input_tensors = {2, (const int32_t[2]){3, 8}}, .output_tensors = {1, (const int32_t[1]){9}} }, /* concat_14 */
    {.id = 15, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){9}}, .output_tensors = {1, (const int32_t[1]){10}} }, /* nl_15 */
    {.id = 16, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){10}}, .output_tensors = {1, (const int32_t[1]){11}} }, /* conv2d_16 */
    {.id = 21, .type = AI_LAYER_TRANSPOSE_TYPE, .input_tensors = {1, (const int32_t[1]){11}}, .output_tensors = {1, (const int32_t[1]){12}} }, /* transpose_21 */
    {.id = 26, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){12}}, .output_tensors = {1, (const int32_t[1]){13}} }, /* conv2d_26_pad_before */
    {.id = 26, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){13}}, .output_tensors = {1, (const int32_t[1]){14}} }, /* conv2d_26 */
    {.id = 27, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){14}}, .output_tensors = {1, (const int32_t[1]){15}} }, /* conv2d_27 */
    {.id = 28, .type = AI_LAYER_ELTWISE_INTEGER_TYPE, .input_tensors = {2, (const int32_t[2]){10, 15}}, .output_tensors = {1, (const int32_t[1]){16}} }, /* eltwise_28 */
    {.id = 28, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){16}}, .output_tensors = {1, (const int32_t[1]){17}} }, /* nl_28_nl */
    {.id = 41, .type = AI_LAYER_POOL_TYPE, .input_tensors = {1, (const int32_t[1]){17}}, .output_tensors = {1, (const int32_t[1]){18}} }, /* pool_41 */
    {.id = 29, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){17}}, .output_tensors = {1, (const int32_t[1]){19}} }, /* conv2d_29 */
    {.id = 34, .type = AI_LAYER_TRANSPOSE_TYPE, .input_tensors = {1, (const int32_t[1]){19}}, .output_tensors = {1, (const int32_t[1]){20}} }, /* transpose_34 */
    {.id = 39, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){20}}, .output_tensors = {1, (const int32_t[1]){21}} }, /* conv2d_39_pad_before */
    {.id = 39, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){21}}, .output_tensors = {1, (const int32_t[1]){22}} }, /* conv2d_39 */
    {.id = 40, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){22}}, .output_tensors = {1, (const int32_t[1]){23}} }, /* conv2d_40 */
    {.id = 42, .type = AI_LAYER_CONCAT_TYPE, .input_tensors = {2, (const int32_t[2]){18, 23}}, .output_tensors = {1, (const int32_t[1]){24}} }, /* concat_42 */
    {.id = 43, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){24}}, .output_tensors = {1, (const int32_t[1]){25}} }, /* nl_43 */
    {.id = 44, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){25}}, .output_tensors = {1, (const int32_t[1]){26}} }, /* conv2d_44 */
    {.id = 49, .type = AI_LAYER_TRANSPOSE_TYPE, .input_tensors = {1, (const int32_t[1]){26}}, .output_tensors = {1, (const int32_t[1]){27}} }, /* transpose_49 */
    {.id = 54, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){27}}, .output_tensors = {1, (const int32_t[1]){28}} }, /* conv2d_54_pad_before */
    {.id = 54, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){28}}, .output_tensors = {1, (const int32_t[1]){29}} }, /* conv2d_54 */
    {.id = 55, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){29}}, .output_tensors = {1, (const int32_t[1]){30}} }, /* conv2d_55 */
    {.id = 56, .type = AI_LAYER_ELTWISE_INTEGER_TYPE, .input_tensors = {2, (const int32_t[2]){25, 30}}, .output_tensors = {1, (const int32_t[1]){31}} }, /* eltwise_56 */
    {.id = 57, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){31}}, .output_tensors = {1, (const int32_t[1]){32}} }, /* conv2d_57 */
    {.id = 58, .type = AI_LAYER_POOL_TYPE, .input_tensors = {1, (const int32_t[1]){32}}, .output_tensors = {1, (const int32_t[1]){33}} }, /* pool_58 */
    {.id = 59, .type = AI_LAYER_DENSE_TYPE, .input_tensors = {1, (const int32_t[1]){33}}, .output_tensors = {1, (const int32_t[1]){34}} }, /* gemm_59 */
    {.id = 60, .type = AI_LAYER_SM_TYPE, .input_tensors = {1, (const int32_t[1]){34}}, .output_tensors = {1, (const int32_t[1]){35}} } /* nl_60 */
  },
  .n_nodes = 35
};
#endif

