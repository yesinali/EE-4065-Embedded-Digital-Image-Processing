/**
  ******************************************************************************
  * @file    network.h
  * @date    2026-01-15T18:23:15+0000
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
  .tensors = (const stai_tensor[48]) {
   { .size_bytes = 784, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 28, 28, 1}}, .scale = {1, (const float[1]){0.003921568859368563}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "serving_default_input0_output" },
   { .size_bytes = 12544, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 28, 28, 16}}, .scale = {1, (const float[1]){0.0235294122248888}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_0_output" },
   { .size_bytes = 14400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 30, 30, 16}}, .scale = {1, (const float[1]){0.0235294122248888}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_1_pad_before_output" },
   { .size_bytes = 12544, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 28, 28, 16}}, .scale = {1, (const float[1]){0.0235294122248888}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_1_output" },
   { .size_bytes = 16, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 16}}, .scale = {1, (const float[1]){0.002881338819861412}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "pool_2_output" },
   { .size_bytes = 4, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 4}}, .scale = {1, (const float[1]){0.0024960017763078213}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_7_output" },
   { .size_bytes = 16, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 16}}, .scale = {1, (const float[1]){0.004110268782824278}}, .zeropoint = {1, (const int16_t[1]){9}}, .name = "conv2d_8_output" },
   { .size_bytes = 16, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 16}}, .scale = {1, (const float[1]){0.00390625}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "nl_9_output" },
   { .size_bytes = 12544, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 28, 28, 16}}, .scale = {1, (const float[1]){0.014061874710023403}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "eltwise_10_output" },
   { .size_bytes = 12544, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 28, 28, 16}}, .scale = {1, (const float[1]){0.06521018594503403}}, .zeropoint = {1, (const int16_t[1]){0}}, .name = "conv2d_11_output" },
   { .size_bytes = 75264, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 28, 28, 96}}, .scale = {1, (const float[1]){0.0235294122248888}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_12_output" },
   { .size_bytes = 86400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 30, 30, 96}}, .scale = {1, (const float[1]){0.0235294122248888}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_13_pad_before_output" },
   { .size_bytes = 18816, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 96}}, .scale = {1, (const float[1]){0.0235294122248888}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_13_output" },
   { .size_bytes = 96, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 96}}, .scale = {1, (const float[1]){0.003947367426007986}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "pool_14_output" },
   { .size_bytes = 24, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 24}}, .scale = {1, (const float[1]){0.009633461944758892}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_19_output" },
   { .size_bytes = 96, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 96}}, .scale = {1, (const float[1]){0.011498714797198772}}, .zeropoint = {1, (const int16_t[1]){0}}, .name = "conv2d_20_output" },
   { .size_bytes = 96, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 96}}, .scale = {1, (const float[1]){0.00390625}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "nl_21_output" },
   { .size_bytes = 18816, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 96}}, .scale = {1, (const float[1]){0.017011865973472595}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "eltwise_22_output" },
   { .size_bytes = 4704, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 24}}, .scale = {1, (const float[1]){0.0646810457110405}}, .zeropoint = {1, (const int16_t[1]){-1}}, .name = "conv2d_23_output" },
   { .size_bytes = 28224, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 144}}, .scale = {1, (const float[1]){0.0235294122248888}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_24_output" },
   { .size_bytes = 36864, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 16, 16, 144}}, .scale = {1, (const float[1]){0.0235294122248888}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_25_pad_before_output" },
   { .size_bytes = 28224, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 144}}, .scale = {1, (const float[1]){0.0235294122248888}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_25_output" },
   { .size_bytes = 144, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 144}}, .scale = {1, (const float[1]){0.003610441694036126}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "pool_26_output" },
   { .size_bytes = 36, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 36}}, .scale = {1, (const float[1]){0.009359263814985752}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_31_output" },
   { .size_bytes = 144, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 144}}, .scale = {1, (const float[1]){0.012024056166410446}}, .zeropoint = {1, (const int16_t[1]){-8}}, .name = "conv2d_32_output" },
   { .size_bytes = 144, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 144}}, .scale = {1, (const float[1]){0.00390625}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "nl_33_output" },
   { .size_bytes = 28224, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 144}}, .scale = {1, (const float[1]){0.017825670540332794}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "eltwise_34_output" },
   { .size_bytes = 4704, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 24}}, .scale = {1, (const float[1]){0.06572987139225006}}, .zeropoint = {1, (const int16_t[1]){14}}, .name = "conv2d_35_output" },
   { .size_bytes = 28224, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 14, 14, 144}}, .scale = {1, (const float[1]){0.0235294122248888}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_36_output" },
   { .size_bytes = 7056, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 144}}, .scale = {1, (const float[1]){0.0235294122248888}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_37_output" },
   { .size_bytes = 144, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 144}}, .scale = {1, (const float[1]){0.004430076573044062}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "pool_38_output" },
   { .size_bytes = 36, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 36}}, .scale = {1, (const float[1]){0.005913991015404463}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_43_output" },
   { .size_bytes = 144, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 144}}, .scale = {1, (const float[1]){0.0069842878729105}}, .zeropoint = {1, (const int16_t[1]){8}}, .name = "conv2d_44_output" },
   { .size_bytes = 144, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 144}}, .scale = {1, (const float[1]){0.00390625}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "nl_45_output" },
   { .size_bytes = 7056, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 144}}, .scale = {1, (const float[1]){0.01602328196167946}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "eltwise_46_output" },
   { .size_bytes = 1960, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 40}}, .scale = {1, (const float[1]){0.049935005605220795}}, .zeropoint = {1, (const int16_t[1]){-2}}, .name = "conv2d_47_output" },
   { .size_bytes = 11760, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 240}}, .scale = {1, (const float[1]){0.0235294122248888}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_48_output" },
   { .size_bytes = 11760, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 240}}, .scale = {1, (const float[1]){0.0235294122248888}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_49_output" },
   { .size_bytes = 240, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 240}}, .scale = {1, (const float[1]){0.010451308451592922}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "pool_50_output" },
   { .size_bytes = 60, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 60}}, .scale = {1, (const float[1]){0.019685499370098114}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_55_output" },
   { .size_bytes = 240, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 240}}, .scale = {1, (const float[1]){0.050723131746053696}}, .zeropoint = {1, (const int16_t[1]){30}}, .name = "conv2d_56_output" },
   { .size_bytes = 240, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 240}}, .scale = {1, (const float[1]){0.00390625}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "nl_57_output" },
   { .size_bytes = 11760, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 240}}, .scale = {1, (const float[1]){0.020274244248867035}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "eltwise_58_output" },
   { .size_bytes = 1960, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 40}}, .scale = {1, (const float[1]){0.037382952868938446}}, .zeropoint = {1, (const int16_t[1]){1}}, .name = "conv2d_59_output" },
   { .size_bytes = 3136, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 64}}, .scale = {1, (const float[1]){0.0235294122248888}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_60_output" },
   { .size_bytes = 64, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 1, 64}}, .scale = {1, (const float[1]){0.015245246700942516}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "pool_61_output" },
   { .size_bytes = 10, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {2, (const int32_t[2]){1, 10}}, .scale = {1, (const float[1]){0.10983701795339584}}, .zeropoint = {1, (const int16_t[1]){-25}}, .name = "gemm_62_output" },
   { .size_bytes = 10, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {2, (const int32_t[2]){1, 10}}, .scale = {1, (const float[1]){0.00390625}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "nl_63_output" }
  },
  .nodes = (const stai_node_details[47]){
    {.id = 0, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){0}}, .output_tensors = {1, (const int32_t[1]){1}} }, /* conv2d_0 */
    {.id = 1, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){1}}, .output_tensors = {1, (const int32_t[1]){2}} }, /* conv2d_1_pad_before */
    {.id = 1, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){2}}, .output_tensors = {1, (const int32_t[1]){3}} }, /* conv2d_1 */
    {.id = 2, .type = AI_LAYER_POOL_TYPE, .input_tensors = {1, (const int32_t[1]){3}}, .output_tensors = {1, (const int32_t[1]){4}} }, /* pool_2 */
    {.id = 7, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){4}}, .output_tensors = {1, (const int32_t[1]){5}} }, /* conv2d_7 */
    {.id = 8, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){5}}, .output_tensors = {1, (const int32_t[1]){6}} }, /* conv2d_8 */
    {.id = 9, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){6}}, .output_tensors = {1, (const int32_t[1]){7}} }, /* nl_9 */
    {.id = 10, .type = AI_LAYER_ELTWISE_INTEGER_TYPE, .input_tensors = {2, (const int32_t[2]){3, 7}}, .output_tensors = {1, (const int32_t[1]){8}} }, /* eltwise_10 */
    {.id = 11, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){8}}, .output_tensors = {1, (const int32_t[1]){9}} }, /* conv2d_11 */
    {.id = 12, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){9}}, .output_tensors = {1, (const int32_t[1]){10}} }, /* conv2d_12 */
    {.id = 13, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){10}}, .output_tensors = {1, (const int32_t[1]){11}} }, /* conv2d_13_pad_before */
    {.id = 13, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){11}}, .output_tensors = {1, (const int32_t[1]){12}} }, /* conv2d_13 */
    {.id = 14, .type = AI_LAYER_POOL_TYPE, .input_tensors = {1, (const int32_t[1]){12}}, .output_tensors = {1, (const int32_t[1]){13}} }, /* pool_14 */
    {.id = 19, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){13}}, .output_tensors = {1, (const int32_t[1]){14}} }, /* conv2d_19 */
    {.id = 20, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){14}}, .output_tensors = {1, (const int32_t[1]){15}} }, /* conv2d_20 */
    {.id = 21, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){15}}, .output_tensors = {1, (const int32_t[1]){16}} }, /* nl_21 */
    {.id = 22, .type = AI_LAYER_ELTWISE_INTEGER_TYPE, .input_tensors = {2, (const int32_t[2]){12, 16}}, .output_tensors = {1, (const int32_t[1]){17}} }, /* eltwise_22 */
    {.id = 23, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){17}}, .output_tensors = {1, (const int32_t[1]){18}} }, /* conv2d_23 */
    {.id = 24, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){18}}, .output_tensors = {1, (const int32_t[1]){19}} }, /* conv2d_24 */
    {.id = 25, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){19}}, .output_tensors = {1, (const int32_t[1]){20}} }, /* conv2d_25_pad_before */
    {.id = 25, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){20}}, .output_tensors = {1, (const int32_t[1]){21}} }, /* conv2d_25 */
    {.id = 26, .type = AI_LAYER_POOL_TYPE, .input_tensors = {1, (const int32_t[1]){21}}, .output_tensors = {1, (const int32_t[1]){22}} }, /* pool_26 */
    {.id = 31, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){22}}, .output_tensors = {1, (const int32_t[1]){23}} }, /* conv2d_31 */
    {.id = 32, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){23}}, .output_tensors = {1, (const int32_t[1]){24}} }, /* conv2d_32 */
    {.id = 33, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){24}}, .output_tensors = {1, (const int32_t[1]){25}} }, /* nl_33 */
    {.id = 34, .type = AI_LAYER_ELTWISE_INTEGER_TYPE, .input_tensors = {2, (const int32_t[2]){21, 25}}, .output_tensors = {1, (const int32_t[1]){26}} }, /* eltwise_34 */
    {.id = 35, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){26}}, .output_tensors = {1, (const int32_t[1]){27}} }, /* conv2d_35 */
    {.id = 36, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){27}}, .output_tensors = {1, (const int32_t[1]){28}} }, /* conv2d_36 */
    {.id = 37, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){28}}, .output_tensors = {1, (const int32_t[1]){29}} }, /* conv2d_37 */
    {.id = 38, .type = AI_LAYER_POOL_TYPE, .input_tensors = {1, (const int32_t[1]){29}}, .output_tensors = {1, (const int32_t[1]){30}} }, /* pool_38 */
    {.id = 43, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){30}}, .output_tensors = {1, (const int32_t[1]){31}} }, /* conv2d_43 */
    {.id = 44, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){31}}, .output_tensors = {1, (const int32_t[1]){32}} }, /* conv2d_44 */
    {.id = 45, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){32}}, .output_tensors = {1, (const int32_t[1]){33}} }, /* nl_45 */
    {.id = 46, .type = AI_LAYER_ELTWISE_INTEGER_TYPE, .input_tensors = {2, (const int32_t[2]){29, 33}}, .output_tensors = {1, (const int32_t[1]){34}} }, /* eltwise_46 */
    {.id = 47, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){34}}, .output_tensors = {1, (const int32_t[1]){35}} }, /* conv2d_47 */
    {.id = 48, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){35}}, .output_tensors = {1, (const int32_t[1]){36}} }, /* conv2d_48 */
    {.id = 49, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){36}}, .output_tensors = {1, (const int32_t[1]){37}} }, /* conv2d_49 */
    {.id = 50, .type = AI_LAYER_POOL_TYPE, .input_tensors = {1, (const int32_t[1]){37}}, .output_tensors = {1, (const int32_t[1]){38}} }, /* pool_50 */
    {.id = 55, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){38}}, .output_tensors = {1, (const int32_t[1]){39}} }, /* conv2d_55 */
    {.id = 56, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){39}}, .output_tensors = {1, (const int32_t[1]){40}} }, /* conv2d_56 */
    {.id = 57, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){40}}, .output_tensors = {1, (const int32_t[1]){41}} }, /* nl_57 */
    {.id = 58, .type = AI_LAYER_ELTWISE_INTEGER_TYPE, .input_tensors = {2, (const int32_t[2]){37, 41}}, .output_tensors = {1, (const int32_t[1]){42}} }, /* eltwise_58 */
    {.id = 59, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){42}}, .output_tensors = {1, (const int32_t[1]){43}} }, /* conv2d_59 */
    {.id = 60, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){43}}, .output_tensors = {1, (const int32_t[1]){44}} }, /* conv2d_60 */
    {.id = 61, .type = AI_LAYER_POOL_TYPE, .input_tensors = {1, (const int32_t[1]){44}}, .output_tensors = {1, (const int32_t[1]){45}} }, /* pool_61 */
    {.id = 62, .type = AI_LAYER_DENSE_TYPE, .input_tensors = {1, (const int32_t[1]){45}}, .output_tensors = {1, (const int32_t[1]){46}} }, /* gemm_62 */
    {.id = 63, .type = AI_LAYER_SM_TYPE, .input_tensors = {1, (const int32_t[1]){46}}, .output_tensors = {1, (const int32_t[1]){47}} } /* nl_63 */
  },
  .n_nodes = 47
};
#endif

