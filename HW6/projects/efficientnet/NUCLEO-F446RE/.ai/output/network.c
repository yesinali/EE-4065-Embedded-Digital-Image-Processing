/**
  ******************************************************************************
  * @file    network.c
  * @author  AST Embedded Analytics Research Platform
  * @date    2026-01-15T18:23:15+0000
  * @brief   AI Tool Automatic Code Generator for Embedded NN computing
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

#include "ai_lite_inspect.h"
#include "ai_platform_interface.h"
#include "layers.h"
#include "core_convert.h"
#include "network.h"
#include "network_details.h"
#include "network_data.h"
#include "stai_events.h"

#include "lite_operators.h"

#include "ai_lite_inspect.h"
/*****************************************************************************/
#define STAI_INTERNAL_API_MAJOR               (1)
#define STAI_INTERNAL_API_MINOR               (0)
#define STAI_INTERNAL_API_MICRO               (0)

#define STAI_MAGIC                            (0xB1C00100)

/*****************************************************************************/
#define _STAI_CONCAT_ARG(a, b)     a ## b
#define STAI_CONCAT(a, b)         _STAI_CONCAT_ARG(a, b)

/*!  STAI_CAST SECTION                       *********************************/
#define STAI_CAST(type, expr) \
  ((type)(expr))


/*****************************************************************************/
#define STAI_SIZE(_size) \
  ((stai_size)(_size))

/*****************************************************************************/
#define STAI_INIT_BUFFER(_flags, _size, _address) \
  { \
    .size = (_size), \
    .address = (uintptr_t)(_address), \
    .flags = (_flags), \
  }

#define STAI_INIT_TENSOR(_name, _flags, _fmt, _size_bytes, _shape, _scale, _zeropoint) \
  { \
    .size_bytes = (_size_bytes), \
    .flags = (_flags), \
    .format = (stai_format)(_fmt), \
    .shape = STAI_PACK(_shape), \
    .scale = STAI_PACK(_scale), \
    .zeropoint = STAI_PACK(_zeropoint), \
    .name = (_name) \
  }

#define STAI_INIT_ARRAY(_size, _ptr) \
  { .size = STAI_SIZE(_size), .data = STAI_PACK(_ptr) }


#define STAI_CAST_ARRAY(_type, _size, _ptr) \
  { .size = STAI_SIZE(_size), .data = (_type)STAI_PACK(_ptr) }


#define STAI_DECLARE_ARRAY(_type, _size, ...) \
  { .size = STAI_SIZE(_size), .data = (_type[_size]) { STAI_PACK(__VA_ARGS__) } }


#define STAI_EMPTY_ARRAY() \
  { .size = 0, .data = NULL }


#define STAI_INIT_VERSION(_major, _minor, _micro) \
  { .major = (_major), .minor = (_minor), .micro = (_micro), .reserved = 0x0 }

/*****************************************************************************/
/**  Getters and setters  **/

#define STAI_GET_ARRAY_SIZE(nd_array) \
  (nd_array.size)


#define STAI_GET_ARRAY_ELEM(nd_array, pos) \
  (nd_array.data[(pos)])

#define _STAI_SET_ERROR(net_ctx, cond, value, exit) { \
  if (!(net_ctx)) { return STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE; } \
  if (((uintptr_t)net_ctx) & (_STAI_CONTEXT_ALIGNMENT-1)) { return STAI_ERROR_NETWORK_INVALID_CONTEXT_ALIGNMENT; } \
  if (((value) >= STAI_ERROR_GENERIC) && (cond)) { \
    if ((net_ctx)->_return_code == STAI_SUCCESS) { \
      (net_ctx)->_return_code = (value); \
    } \
    return (exit); \
  } \
}

/*****************************************************************************/
/* TODO REMOVE THESE TWO MACROS */
#define STAI_EVENT_NODE_START_CB
#define STAI_EVENT_NODE_STOP_CB

#ifdef STAI_EVENT_NODE_START_CB
#ifndef _STAI_NETWORK_EVENT_NODE_START_CB
  #define _STAI_NETWORK_EVENT_NODE_START_CB(_node_id, _buffers_size, ...) \
  if (net_ctx->_callback) { \
    const stai_event_node_start_stop _start_event = { \
      .node_id=(_node_id), \
      .buffers={ \
        .size=(_buffers_size), \
        .data=(stai_ptr const*)(const stai_ptr[_buffers_size])STAI_PACK(__VA_ARGS__) \
      } \
    }; \
    net_ctx->_callback(net_ctx->_callback_cookie, STAI_EVENT_NODE_START, (const void*)&_start_event); \
  }
#endif
#else
  #define _STAI_NETWORK_EVENT_NODE_START_CB(_node_id, _buffers_size, ...) \
    do { /* _STAI_NETWORK_EVENT_NODE_START_CB() */ } while(0);
#endif      /* STAI_EVENT_NODE_START_CB */

#ifdef STAI_EVENT_NODE_STOP_CB
#ifndef _STAI_NETWORK_EVENT_NODE_STOP_CB
  #define _STAI_NETWORK_EVENT_NODE_STOP_CB(_node_id, _buffers_size, ...) \
  if (net_ctx->_callback) { \
    const stai_event_node_start_stop _stop_event = { \
      .node_id=(_node_id), \
      .buffers={ \
        .size=(_buffers_size), \
        .data=(stai_ptr const*)(stai_ptr[_buffers_size])STAI_PACK(__VA_ARGS__) \
      } \
    }; \
    net_ctx->_callback(net_ctx->_callback_cookie, STAI_EVENT_NODE_STOP, (const void*)&_stop_event); \
  }
#endif
#else
  #define _STAI_NETWORK_EVENT_NODE_STOP_CB(_node_id, _buffers_size, ...) \
    do { /* _STAI_NETWORK_EVENT_NODE_STOP_CB() */ } while(0);
#endif      /* STAI_EVENT_NODE_STOP_CB */


/*****************************************************************************/
#define _STAI_NETWORK_MODEL_SIGNATURE     "0xa51449928f292b59040a43c82a0ce86a"
#define _STAI_NETWORK_DATETIME            "2026-01-15T18:23:15+0000"
#define _STAI_NETWORK_COMPILE_DATETIME    __DATE__ " " __TIME__

#define _STAI_CONTEXT_ALIGNMENT        STAI_NETWORK_CONTEXT_ALIGNMENT

/*****************************************************************************/
#define g_network_activations_1     (NULL)




#if defined(HAVE_NETWORK_INFO)
/*****************************************************************************/
static const stai_network_info g_network_info = {
  .model_signature = _STAI_NETWORK_MODEL_SIGNATURE,
  .c_compile_datetime = _STAI_NETWORK_COMPILE_DATETIME,
  .c_model_name = STAI_NETWORK_MODEL_NAME,
  .c_model_datetime = _STAI_NETWORK_DATETIME,
  .c_model_signature = 0x0,
  .runtime_version = STAI_INIT_VERSION(11, 0, 0),
  .tool_version = STAI_INIT_VERSION(3, 0, 0),
  .api_version = STAI_INIT_VERSION(1, 0, 0),
  .n_macc = STAI_NETWORK_MACC_NUM,
  .n_nodes = STAI_NETWORK_NODES_NUM,
  .flags = STAI_NETWORK_FLAGS,
  .n_inputs = STAI_NETWORK_IN_NUM,
  .n_outputs = STAI_NETWORK_OUT_NUM,
  .n_activations = STAI_NETWORK_ACTIVATIONS_NUM,
  .n_weights = STAI_NETWORK_WEIGHTS_NUM,
  .n_states = STAI_NETWORK_STATES_NUM,
  .inputs = (stai_tensor[STAI_NETWORK_IN_NUM]) {
    STAI_INIT_TENSOR(
      STAI_NETWORK_IN_1_NAME,
      STAI_NETWORK_IN_1_FLAGS,
      STAI_NETWORK_IN_1_FORMAT,
      STAI_NETWORK_IN_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 4, 1, 28, 28, 1),
      STAI_DECLARE_ARRAY(float, 1, 0.003921568859368563f),
      STAI_DECLARE_ARRAY(int16_t, 1, -128)),
    },
    .outputs = (stai_tensor[STAI_NETWORK_OUT_NUM]) {
    STAI_INIT_TENSOR(
      STAI_NETWORK_OUT_1_NAME,
      STAI_NETWORK_OUT_1_FLAGS,
      STAI_NETWORK_OUT_1_FORMAT,
      STAI_NETWORK_OUT_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 2, 1, 10),
      STAI_DECLARE_ARRAY(float, 1, 0.00390625f),
      STAI_DECLARE_ARRAY(int16_t, 1, -128)),
    },
  .activations = (stai_tensor[STAI_NETWORK_ACTIVATIONS_NUM]) {
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_ACTIVATION_1_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_ACTIVATION_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 91456),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },
  .weights = (stai_tensor[STAI_NETWORK_WEIGHTS_NUM]) {
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_1_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 144),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_2_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_2_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 64),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_3_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_3_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 144),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_4_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_4_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 64),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_5_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_5_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 64),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_6_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_6_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 16),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_7_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_7_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 64),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_8_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_8_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 64),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_9_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_9_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 256),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_10_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_10_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 64),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_11_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_11_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1536),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_12_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_12_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 384),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_13_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_13_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 864),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_14_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_14_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 384),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_15_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_15_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 2304),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_16_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_16_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 96),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_17_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_17_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 2304),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_18_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_18_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 384),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_19_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_19_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 2304),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_20_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_20_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 96),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_21_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_21_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 3456),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_22_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_22_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 576),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_23_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_23_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1296),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_24_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_24_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 576),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_25_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_25_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 5184),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_26_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_26_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 144),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_27_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_27_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 5184),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_28_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_28_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 576),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_29_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_29_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 3456),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_30_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_30_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 96),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_31_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_31_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 3456),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_32_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_32_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 576),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_33_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_33_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 3600),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_34_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_34_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 576),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_35_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_35_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 5184),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_36_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_36_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 144),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_37_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_37_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 5184),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_38_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_38_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 576),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_39_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_39_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 5760),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_40_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_40_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 160),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_41_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_41_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 9600),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_42_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_42_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 960),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_43_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_43_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 6000),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_44_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_44_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 960),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_45_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_45_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 14400),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_46_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_46_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 240),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_47_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_47_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 14400),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_48_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_48_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 960),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_49_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_49_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 9600),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_50_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_50_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 160),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_51_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_51_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 2560),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_52_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_52_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 256),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_53_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_53_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 640),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_54_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_54_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 40),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },

  .states = NULL
};
#endif

#define _STAI_CONTEXT_ACQUIRE(_net_ctx, _net_handle) \
  _stai_network_context* _net_ctx = (_stai_network_context*)(_net_handle); \
  STAI_ASSERT(_net_ctx != NULL) \
  _STAI_SET_ERROR(_net_ctx, _net_ctx->_magic != STAI_MAGIC, \
                  STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE, _net_ctx->_return_code)


/*****************************************************************************/
static
void _stai_network_check(_stai_network_context* net_ctx)
{
  stai_size idx;

// Check activations status
  for (idx=0; idx<STAI_NETWORK_ACTIVATIONS_NUM; idx++) {
    if (net_ctx->_activations[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_ACTIVATIONS_NUM) ? STAI_FLAG_ACTIVATIONS : STAI_FLAG_NONE;
// Check inputs status
  for (idx=0; idx<STAI_NETWORK_IN_NUM; idx++) {
    if (net_ctx->_inputs[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_IN_NUM) ? STAI_FLAG_INPUTS : STAI_FLAG_NONE;

  // Check outputs status
  for (idx=0; idx<STAI_NETWORK_OUT_NUM; idx++) {
    if (net_ctx->_outputs[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_OUT_NUM) ? STAI_FLAG_OUTPUTS : STAI_FLAG_NONE;

// Check weights status
  for (idx=0; idx<STAI_NETWORK_WEIGHTS_NUM; idx++) {
    if (net_ctx->_weights[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_WEIGHTS_NUM) ? STAI_FLAG_WEIGHTS : STAI_FLAG_NONE;
STAI_PRINT("  [_stai_network_check] flags: 0x%08x\n", net_ctx->_flags)
}


/*****************************************************************************/
STAI_API_ENTRY
stai_return_code stai_network_init(
  stai_network* network)
{
  /* Memory where to store internal context is provided by applications as a raw byte buffer */
  _stai_network_context* net_ctx = (_stai_network_context*)(network);
  net_ctx->_return_code = STAI_SUCCESS;
  STAI_PRINT("[Entering Network Init] network(%p) context_size(%d)\n", net_ctx, (int32_t)sizeof(_stai_network_context))

  _STAI_SET_ERROR(net_ctx, STAI_NETWORK_CONTEXT_SIZE != sizeof(_stai_network_context),
                 STAI_ERROR_NETWORK_INVALID_CONTEXT_SIZE, net_ctx->_return_code)

  {
    const _stai_network_context _network_context = {
      ._magic = STAI_MAGIC,
      ._signature = STAI_NETWORK_MODEL_SIGNATURE,
      ._flags = STAI_NETWORK_FLAGS,
      ._return_code = STAI_SUCCESS,
      ._callback = NULL,
      ._callback_cookie = NULL,
      ._activations = {
      (stai_ptr)g_network_activations_1
      },
      ._weights = {
      (stai_ptr)g_network_conv2d_0_weights_array,(stai_ptr)g_network_conv2d_0_bias_array,(stai_ptr)g_network_conv2d_1_weights_array,(stai_ptr)g_network_conv2d_1_bias_array,(stai_ptr)g_network_conv2d_7_weights_array,(stai_ptr)g_network_conv2d_7_bias_array,(stai_ptr)g_network_conv2d_8_weights_array,(stai_ptr)g_network_conv2d_8_bias_array,(stai_ptr)g_network_conv2d_11_weights_array,(stai_ptr)g_network_conv2d_11_bias_array,(stai_ptr)g_network_conv2d_12_weights_array,(stai_ptr)g_network_conv2d_12_bias_array,(stai_ptr)g_network_conv2d_13_weights_array,(stai_ptr)g_network_conv2d_13_bias_array,(stai_ptr)g_network_conv2d_19_weights_array,(stai_ptr)g_network_conv2d_19_bias_array,(stai_ptr)g_network_conv2d_20_weights_array,(stai_ptr)g_network_conv2d_20_bias_array,(stai_ptr)g_network_conv2d_23_weights_array,(stai_ptr)g_network_conv2d_23_bias_array,(stai_ptr)g_network_conv2d_24_weights_array,(stai_ptr)g_network_conv2d_24_bias_array,(stai_ptr)g_network_conv2d_25_weights_array,(stai_ptr)g_network_conv2d_25_bias_array,(stai_ptr)g_network_conv2d_31_weights_array,(stai_ptr)g_network_conv2d_31_bias_array,(stai_ptr)g_network_conv2d_32_weights_array,(stai_ptr)g_network_conv2d_32_bias_array,(stai_ptr)g_network_conv2d_35_weights_array,(stai_ptr)g_network_conv2d_35_bias_array,(stai_ptr)g_network_conv2d_36_weights_array,(stai_ptr)g_network_conv2d_36_bias_array,(stai_ptr)g_network_conv2d_37_weights_array,(stai_ptr)g_network_conv2d_37_bias_array,(stai_ptr)g_network_conv2d_43_weights_array,(stai_ptr)g_network_conv2d_43_bias_array,(stai_ptr)g_network_conv2d_44_weights_array,(stai_ptr)g_network_conv2d_44_bias_array,(stai_ptr)g_network_conv2d_47_weights_array,(stai_ptr)g_network_conv2d_47_bias_array,(stai_ptr)g_network_conv2d_48_weights_array,(stai_ptr)g_network_conv2d_48_bias_array,(stai_ptr)g_network_conv2d_49_weights_array,(stai_ptr)g_network_conv2d_49_bias_array,(stai_ptr)g_network_conv2d_55_weights_array,(stai_ptr)g_network_conv2d_55_bias_array,(stai_ptr)g_network_conv2d_56_weights_array,(stai_ptr)g_network_conv2d_56_bias_array,(stai_ptr)g_network_conv2d_59_weights_array,(stai_ptr)g_network_conv2d_59_bias_array,(stai_ptr)g_network_conv2d_60_weights_array,(stai_ptr)g_network_conv2d_60_bias_array,(stai_ptr)g_network_gemm_62_weights_array,(stai_ptr)g_network_gemm_62_bias_array
      },
      ._inputs = {
    NULL},
      ._outputs = {
    NULL},
    };

    // Deep copy of internal context to opaque buffer provided by app
    *net_ctx = _network_context;

    _stai_network_check(net_ctx);
  }

  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_deinit(
  stai_network* network)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  /*  Reset flags to initial state  */
  net_ctx->_flags = STAI_NETWORK_FLAGS;
  return net_ctx->_return_code;
}

/*****************************************************************************/



/* Int quant #0 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_1_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0235294122248888f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #1 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(pool_2_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.002881338819861412f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #2 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_8_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.004110268782824278f),
    AI_PACK_INTQ_ZP(9)))

/* Int quant #3 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(nl_9_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.00390625f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #4 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_10_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.014061874710023403f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #5 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_13_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0235294122248888f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #6 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(pool_14_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.003947367426007986f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #7 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_20_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.011498714797198772f),
    AI_PACK_INTQ_ZP(0)))

/* Int quant #8 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(nl_21_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.00390625f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #9 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_22_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.017011865973472595f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #10 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_25_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0235294122248888f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #11 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(pool_26_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.003610441694036126f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #12 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_32_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.012024056166410446f),
    AI_PACK_INTQ_ZP(-8)))

/* Int quant #13 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(nl_33_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.00390625f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #14 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_34_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.017825670540332794f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #15 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_37_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0235294122248888f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #16 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(pool_38_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.004430076573044062f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #17 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_44_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0069842878729105f),
    AI_PACK_INTQ_ZP(8)))

/* Int quant #18 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(nl_45_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.00390625f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #19 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_46_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.01602328196167946f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #20 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_49_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0235294122248888f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #21 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(pool_50_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.010451308451592922f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #22 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_56_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.050723131746053696f),
    AI_PACK_INTQ_ZP(30)))

/* Int quant #23 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(nl_57_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.00390625f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #24 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_58_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.020274244248867035f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #25 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_60_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0235294122248888f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #26 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(pool_61_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.015245246700942516f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #27 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_62_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.10983701795339584f),
    AI_PACK_INTQ_ZP(-25)))

/* Int quant #28 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_62_weights_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 10,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.004077230114489794f, 0.003795639844611287f, 0.00416895467787981f, 0.0042335581965744495f, 0.004385911859571934f, 0.004104014951735735f, 0.003892162349075079f, 0.0034633714240044355f, 0.0040231854654848576f, 0.003566432511433959f),
    AI_PACK_INTQ_ZP(0, 0, 0, 0, 0, 0, 0, 0, 0, 0)))



/* Array#0 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_1_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 12544, AI_STATIC)

/* Array#1 */
AI_ARRAY_OBJ_DECLARE(
  pool_2_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 16, AI_STATIC)

/* Array#2 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_8_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 16, AI_STATIC)

/* Array#3 */
AI_ARRAY_OBJ_DECLARE(
  nl_9_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 16, AI_STATIC)

/* Array#4 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_10_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 12544, AI_STATIC)

/* Array#5 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_13_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 18816, AI_STATIC)

/* Array#6 */
AI_ARRAY_OBJ_DECLARE(
  pool_14_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 96, AI_STATIC)

/* Array#7 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_20_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 96, AI_STATIC)

/* Array#8 */
AI_ARRAY_OBJ_DECLARE(
  nl_21_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 96, AI_STATIC)

/* Array#9 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_22_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 18816, AI_STATIC)

/* Array#10 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_25_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 28224, AI_STATIC)

/* Array#11 */
AI_ARRAY_OBJ_DECLARE(
  pool_26_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 144, AI_STATIC)

/* Array#12 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_32_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 144, AI_STATIC)

/* Array#13 */
AI_ARRAY_OBJ_DECLARE(
  nl_33_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 144, AI_STATIC)

/* Array#14 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_34_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 28224, AI_STATIC)

/* Array#15 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_37_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 7056, AI_STATIC)

/* Array#16 */
AI_ARRAY_OBJ_DECLARE(
  pool_38_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 144, AI_STATIC)

/* Array#17 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_44_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 144, AI_STATIC)

/* Array#18 */
AI_ARRAY_OBJ_DECLARE(
  nl_45_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 144, AI_STATIC)

/* Array#19 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_46_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 7056, AI_STATIC)

/* Array#20 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_49_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 11760, AI_STATIC)

/* Array#21 */
AI_ARRAY_OBJ_DECLARE(
  pool_50_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 240, AI_STATIC)

/* Array#22 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_56_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 240, AI_STATIC)

/* Array#23 */
AI_ARRAY_OBJ_DECLARE(
  nl_57_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 240, AI_STATIC)

/* Array#24 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_58_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 11760, AI_STATIC)

/* Array#25 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_60_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 3136, AI_STATIC)

/* Array#26 */
AI_ARRAY_OBJ_DECLARE(
  pool_61_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 64, AI_STATIC)

/* Array#27 */
AI_ARRAY_OBJ_DECLARE(
  gemm_62_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 10, AI_STATIC)

/* Array#28 */
AI_ARRAY_OBJ_DECLARE(
  gemm_62_weights_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 640, AI_STATIC)

/* Array#29 */
AI_ARRAY_OBJ_DECLARE(
  gemm_62_bias_array, AI_ARRAY_FORMAT_S32,
  NULL, NULL, 10, AI_STATIC)

/* Array#30 */
AI_ARRAY_OBJ_DECLARE(
  gemm_62_scratch0_array, AI_ARRAY_FORMAT_S16,
  NULL, NULL, 114, AI_STATIC)



/* Tensor #0 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_1_output, AI_STATIC,
  22, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 28, 28), AI_STRIDE_INIT(4, 1, 1, 16, 448),
  1, &conv2d_1_output_array, &conv2d_1_output_array_intq)

/* Tensor #1 */
AI_TENSOR_OBJ_DECLARE(
  pool_2_output, AI_STATIC,
  125, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 1, 1), AI_STRIDE_INIT(4, 1, 1, 16, 16),
  1, &pool_2_output_array, &pool_2_output_array_intq)

/* Tensor #2 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_8_output, AI_STATIC,
  104, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 1, 1), AI_STRIDE_INIT(4, 1, 1, 16, 16),
  1, &conv2d_8_output_array, &conv2d_8_output_array_intq)

/* Tensor #3 */
AI_TENSOR_OBJ_DECLARE(
  nl_9_output, AI_STATIC,
  122, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 1, 1), AI_STRIDE_INIT(4, 1, 1, 16, 16),
  1, &nl_9_output_array, &nl_9_output_array_intq)

/* Tensor #4 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_10_output, AI_STATIC,
  107, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 28, 28), AI_STRIDE_INIT(4, 1, 1, 16, 448),
  1, &eltwise_10_output_array, &eltwise_10_output_array_intq)

/* Tensor #5 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_13_output, AI_STATIC,
  13, 0x1,
  AI_SHAPE_INIT(4, 1, 96, 14, 14), AI_STRIDE_INIT(4, 1, 1, 96, 1344),
  1, &conv2d_13_output_array, &conv2d_13_output_array_intq)

/* Tensor #6 */
AI_TENSOR_OBJ_DECLARE(
  pool_14_output, AI_STATIC,
  123, 0x1,
  AI_SHAPE_INIT(4, 1, 96, 1, 1), AI_STRIDE_INIT(4, 1, 1, 96, 96),
  1, &pool_14_output_array, &pool_14_output_array_intq)

/* Tensor #7 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_20_output, AI_STATIC,
  27, 0x1,
  AI_SHAPE_INIT(4, 1, 96, 1, 1), AI_STRIDE_INIT(4, 1, 1, 96, 96),
  1, &conv2d_20_output_array, &conv2d_20_output_array_intq)

/* Tensor #8 */
AI_TENSOR_OBJ_DECLARE(
  nl_21_output, AI_STATIC,
  116, 0x1,
  AI_SHAPE_INIT(4, 1, 96, 1, 1), AI_STRIDE_INIT(4, 1, 1, 96, 96),
  1, &nl_21_output_array, &nl_21_output_array_intq)

/* Tensor #9 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_22_output, AI_STATIC,
  108, 0x1,
  AI_SHAPE_INIT(4, 1, 96, 14, 14), AI_STRIDE_INIT(4, 1, 1, 96, 1344),
  1, &eltwise_22_output_array, &eltwise_22_output_array_intq)

/* Tensor #10 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_25_output, AI_STATIC,
  39, 0x1,
  AI_SHAPE_INIT(4, 1, 144, 14, 14), AI_STRIDE_INIT(4, 1, 1, 144, 2016),
  1, &conv2d_25_output_array, &conv2d_25_output_array_intq)

/* Tensor #11 */
AI_TENSOR_OBJ_DECLARE(
  pool_26_output, AI_STATIC,
  124, 0x1,
  AI_SHAPE_INIT(4, 1, 144, 1, 1), AI_STRIDE_INIT(4, 1, 1, 144, 144),
  1, &pool_26_output_array, &pool_26_output_array_intq)

/* Tensor #12 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_32_output, AI_STATIC,
  48, 0x1,
  AI_SHAPE_INIT(4, 1, 144, 1, 1), AI_STRIDE_INIT(4, 1, 1, 144, 144),
  1, &conv2d_32_output_array, &conv2d_32_output_array_intq)

/* Tensor #13 */
AI_TENSOR_OBJ_DECLARE(
  nl_33_output, AI_STATIC,
  117, 0x1,
  AI_SHAPE_INIT(4, 1, 144, 1, 1), AI_STRIDE_INIT(4, 1, 1, 144, 144),
  1, &nl_33_output_array, &nl_33_output_array_intq)

/* Tensor #14 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_34_output, AI_STATIC,
  109, 0x1,
  AI_SHAPE_INIT(4, 1, 144, 14, 14), AI_STRIDE_INIT(4, 1, 1, 144, 2016),
  1, &eltwise_34_output_array, &eltwise_34_output_array_intq)

/* Tensor #15 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_37_output, AI_STATIC,
  60, 0x1,
  AI_SHAPE_INIT(4, 1, 144, 7, 7), AI_STRIDE_INIT(4, 1, 1, 144, 1008),
  1, &conv2d_37_output_array, &conv2d_37_output_array_intq)

/* Tensor #16 */
AI_TENSOR_OBJ_DECLARE(
  pool_38_output, AI_STATIC,
  126, 0x1,
  AI_SHAPE_INIT(4, 1, 144, 1, 1), AI_STRIDE_INIT(4, 1, 1, 144, 144),
  1, &pool_38_output_array, &pool_38_output_array_intq)

/* Tensor #17 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_44_output, AI_STATIC,
  68, 0x1,
  AI_SHAPE_INIT(4, 1, 144, 1, 1), AI_STRIDE_INIT(4, 1, 1, 144, 144),
  1, &conv2d_44_output_array, &conv2d_44_output_array_intq)

/* Tensor #18 */
AI_TENSOR_OBJ_DECLARE(
  nl_45_output, AI_STATIC,
  118, 0x1,
  AI_SHAPE_INIT(4, 1, 144, 1, 1), AI_STRIDE_INIT(4, 1, 1, 144, 144),
  1, &nl_45_output_array, &nl_45_output_array_intq)

/* Tensor #19 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_46_output, AI_STATIC,
  110, 0x1,
  AI_SHAPE_INIT(4, 1, 144, 7, 7), AI_STRIDE_INIT(4, 1, 1, 144, 1008),
  1, &eltwise_46_output_array, &eltwise_46_output_array_intq)

/* Tensor #20 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_49_output, AI_STATIC,
  80, 0x1,
  AI_SHAPE_INIT(4, 1, 240, 7, 7), AI_STRIDE_INIT(4, 1, 1, 240, 1680),
  1, &conv2d_49_output_array, &conv2d_49_output_array_intq)

/* Tensor #21 */
AI_TENSOR_OBJ_DECLARE(
  pool_50_output, AI_STATIC,
  127, 0x1,
  AI_SHAPE_INIT(4, 1, 240, 1, 1), AI_STRIDE_INIT(4, 1, 1, 240, 240),
  1, &pool_50_output_array, &pool_50_output_array_intq)

/* Tensor #22 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_56_output, AI_STATIC,
  88, 0x1,
  AI_SHAPE_INIT(4, 1, 240, 1, 1), AI_STRIDE_INIT(4, 1, 1, 240, 240),
  1, &conv2d_56_output_array, &conv2d_56_output_array_intq)

/* Tensor #23 */
AI_TENSOR_OBJ_DECLARE(
  nl_57_output, AI_STATIC,
  119, 0x1,
  AI_SHAPE_INIT(4, 1, 240, 1, 1), AI_STRIDE_INIT(4, 1, 1, 240, 240),
  1, &nl_57_output_array, &nl_57_output_array_intq)

/* Tensor #24 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_58_output, AI_STATIC,
  111, 0x1,
  AI_SHAPE_INIT(4, 1, 240, 7, 7), AI_STRIDE_INIT(4, 1, 1, 240, 1680),
  1, &eltwise_58_output_array, &eltwise_58_output_array_intq)

/* Tensor #25 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_60_output, AI_STATIC,
  96, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 1, 1, 64, 448),
  1, &conv2d_60_output_array, &conv2d_60_output_array_intq)

/* Tensor #26 */
AI_TENSOR_OBJ_DECLARE(
  pool_61_output, AI_STATIC,
  128, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 1, 1), AI_STRIDE_INIT(4, 1, 1, 64, 64),
  1, &pool_61_output_array, &pool_61_output_array_intq)

/* Tensor #27 */
AI_TENSOR_OBJ_DECLARE(
  gemm_62_bias, AI_STATIC,
  112, 0x0,
  AI_SHAPE_INIT(4, 1, 10, 1, 1), AI_STRIDE_INIT(4, 4, 4, 40, 40),
  1, &gemm_62_bias_array, NULL)

/* Tensor #28 */
AI_TENSOR_OBJ_DECLARE(
  gemm_62_output, AI_STATIC,
  113, 0x1,
  AI_SHAPE_INIT(4, 1, 10, 1, 1), AI_STRIDE_INIT(4, 1, 1, 10, 10),
  1, &gemm_62_output_array, &gemm_62_output_array_intq)

/* Tensor #29 */
AI_TENSOR_OBJ_DECLARE(
  gemm_62_scratch0, AI_STATIC,
  114, 0x0,
  AI_SHAPE_INIT(4, 1, 114, 1, 1), AI_STRIDE_INIT(4, 2, 2, 228, 228),
  1, &gemm_62_scratch0_array, NULL)

/* Tensor #30 */
AI_TENSOR_OBJ_DECLARE(
  gemm_62_weights, AI_STATIC,
  115, 0x1,
  AI_SHAPE_INIT(4, 64, 10, 1, 1), AI_STRIDE_INIT(4, 1, 64, 640, 640),
  1, &gemm_62_weights_array, &gemm_62_weights_array_intq)


AI_TENSOR_CHAIN_OBJ_DECLARE(
  pool_2_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_1_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &pool_2_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  pool_2_layer, 2,
  POOL_TYPE, 0x0, NULL,
  pool, forward_ap_integer_INT8,
  &pool_2_chain,
  NULL, &pool_2_layer, AI_STATIC, 
  .pool_size = AI_SHAPE_2D_INIT(28, 28), 
  .pool_stride = AI_SHAPE_2D_INIT(28, 28), 
  .pool_pad = AI_SHAPE_INIT(4, 0, 0, 0, 0), 
)


AI_STATIC_CONST ai_i8 nl_9_nl_params_data[] = { -35, -35, -35, -34, -34, -34, -34, -33, -33, -33, -33, -32, -32, -32, -32, -31, -31, -31, -31, -30, -30, -30, -30, -29, -29, -29, -29, -28, -28, -28, -28, -27, -27, -27, -27, -26, -26, -26, -26, -25, -25, -25, -25, -24, -24, -24, -24, -23, -23, -23, -23, -22, -22, -22, -22, -21, -21, -21, -21, -20, -20, -20, -20, -19, -19, -19, -19, -18, -18, -18, -18, -17, -17, -17, -16, -16, -16, -16, -15, -15, -15, -15, -14, -14, -14, -14, -13, -13, -13, -13, -12, -12, -12, -12, -11, -11, -11, -10, -10, -10, -10, -9, -9, -9, -9, -8, -8, -8, -8, -7, -7, -7, -7, -6, -6, -6, -6, -5, -5, -5, -4, -4, -4, -4, -3, -3, -3, -3, -2, -2, -2, -2, -1, -1, -1, -1, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23, 23, 24, 24, 24, 24, 25, 25, 25, 25, 26, 26, 26, 26, 27, 27, 27, 27, 28, 28, 28, 28, 29, 29, 29, 29, 30, 30, 30, 30 };
AI_ARRAY_OBJ_DECLARE(
    nl_9_nl_params, AI_ARRAY_FORMAT_S8,
    nl_9_nl_params_data, nl_9_nl_params_data, 256, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  nl_9_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_8_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &nl_9_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  nl_9_layer, 9,
  NL_TYPE, 0x0, NULL,
  nl, forward_nl_integer,
  &nl_9_chain,
  NULL, &nl_9_layer, AI_STATIC, 
  .nl_params = &nl_9_nl_params, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_10_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_1_output, &nl_9_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_10_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  eltwise_10_layer, 10,
  ELTWISE_INTEGER_TYPE, 0x0, NULL,
  eltwise_integer, forward_eltwise_integer_INT8,
  &eltwise_10_chain,
  NULL, &eltwise_10_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_INT8, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  pool_14_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_13_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &pool_14_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  pool_14_layer, 14,
  POOL_TYPE, 0x0, NULL,
  pool, forward_ap_integer_INT8,
  &pool_14_chain,
  NULL, &pool_14_layer, AI_STATIC, 
  .pool_size = AI_SHAPE_2D_INIT(14, 14), 
  .pool_stride = AI_SHAPE_2D_INIT(14, 14), 
  .pool_pad = AI_SHAPE_INIT(4, 0, 0, 0, 0), 
)


AI_STATIC_CONST ai_i8 nl_21_nl_params_data[] = { -80, -80, -79, -79, -78, -78, -77, -77, -77, -76, -76, -75, -75, -74, -74, -73, -73, -72, -72, -71, -71, -70, -70, -69, -69, -68, -67, -67, -66, -66, -65, -65, -64, -64, -63, -63, -62, -61, -61, -60, -60, -59, -59, -58, -57, -57, -56, -56, -55, -54, -54, -53, -53, -52, -51, -51, -50, -50, -49, -48, -48, -47, -46, -46, -45, -44, -44, -43, -42, -42, -41, -41, -40, -39, -39, -38, -37, -36, -36, -35, -34, -34, -33, -32, -32, -31, -30, -30, -29, -28, -28, -27, -26, -25, -25, -24, -23, -23, -22, -21, -20, -20, -19, -18, -18, -17, -16, -15, -15, -14, -13, -12, -12, -11, -10, -10, -9, -8, -7, -7, -6, -5, -4, -4, -3, -2, -1, -1, 0, 1, 1, 2, 3, 4, 4, 5, 6, 7, 7, 8, 9, 10, 10, 11, 12, 12, 13, 14, 15, 15, 16, 17, 18, 18, 19, 20, 20, 21, 22, 23, 23, 24, 25, 25, 26, 27, 28, 28, 29, 30, 30, 31, 32, 32, 33, 34, 34, 35, 36, 36, 37, 38, 39, 39, 40, 41, 41, 42, 42, 43, 44, 44, 45, 46, 46, 47, 48, 48, 49, 50, 50, 51, 51, 52, 53, 53, 54, 54, 55, 56, 56, 57, 57, 58, 59, 59, 60, 60, 61, 61, 62, 63, 63, 64, 64, 65, 65, 66, 66, 67, 67, 68, 69, 69, 70, 70, 71, 71, 72, 72, 73, 73, 74, 74, 75, 75, 76, 76, 77, 77, 77, 78, 78, 79, 79, 80 };
AI_ARRAY_OBJ_DECLARE(
    nl_21_nl_params, AI_ARRAY_FORMAT_S8,
    nl_21_nl_params_data, nl_21_nl_params_data, 256, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  nl_21_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_20_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &nl_21_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  nl_21_layer, 21,
  NL_TYPE, 0x0, NULL,
  nl, forward_nl_integer,
  &nl_21_chain,
  NULL, &nl_21_layer, AI_STATIC, 
  .nl_params = &nl_21_nl_params, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_22_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_13_output, &nl_21_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_22_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  eltwise_22_layer, 22,
  ELTWISE_INTEGER_TYPE, 0x0, NULL,
  eltwise_integer, forward_eltwise_integer_INT8,
  &eltwise_22_chain,
  NULL, &eltwise_22_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_INT8, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  pool_26_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_25_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &pool_26_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  pool_26_layer, 26,
  POOL_TYPE, 0x0, NULL,
  pool, forward_ap_integer_INT8,
  &pool_26_chain,
  NULL, &pool_26_layer, AI_STATIC, 
  .pool_size = AI_SHAPE_2D_INIT(14, 14), 
  .pool_stride = AI_SHAPE_2D_INIT(14, 14), 
  .pool_pad = AI_SHAPE_INIT(4, 0, 0, 0, 0), 
)


AI_STATIC_CONST ai_i8 nl_33_nl_params_data[] = { -79, -79, -78, -78, -77, -77, -76, -76, -75, -75, -74, -74, -73, -73, -72, -72, -71, -70, -70, -69, -69, -68, -68, -67, -67, -66, -66, -65, -64, -64, -63, -63, -62, -61, -61, -60, -60, -59, -58, -58, -57, -57, -56, -55, -55, -54, -53, -53, -52, -52, -51, -50, -50, -49, -48, -48, -47, -46, -46, -45, -44, -44, -43, -42, -42, -41, -40, -39, -39, -38, -37, -37, -36, -35, -35, -34, -33, -32, -32, -31, -30, -29, -29, -28, -27, -27, -26, -25, -24, -24, -23, -22, -21, -21, -20, -19, -18, -18, -17, -16, -15, -15, -14, -13, -12, -12, -11, -10, -9, -8, -8, -7, -6, -5, -5, -4, -3, -2, -2, -1, 0, 1, 2, 2, 3, 4, 5, 5, 6, 7, 8, 8, 9, 10, 11, 12, 12, 13, 14, 15, 15, 16, 17, 18, 18, 19, 20, 21, 21, 22, 23, 24, 24, 25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36, 37, 37, 38, 39, 39, 40, 41, 42, 42, 43, 44, 44, 45, 46, 46, 47, 48, 48, 49, 50, 50, 51, 52, 52, 53, 53, 54, 55, 55, 56, 57, 57, 58, 58, 59, 60, 60, 61, 61, 62, 63, 63, 64, 64, 65, 66, 66, 67, 67, 68, 68, 69, 69, 70, 70, 71, 72, 72, 73, 73, 74, 74, 75, 75, 76, 76, 77, 77, 78, 78, 79, 79, 80, 80, 80, 81, 81, 82, 82, 83, 83, 84, 84, 85, 85, 85, 86 };
AI_ARRAY_OBJ_DECLARE(
    nl_33_nl_params, AI_ARRAY_FORMAT_S8,
    nl_33_nl_params_data, nl_33_nl_params_data, 256, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  nl_33_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_32_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &nl_33_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  nl_33_layer, 33,
  NL_TYPE, 0x0, NULL,
  nl, forward_nl_integer,
  &nl_33_chain,
  NULL, &nl_33_layer, AI_STATIC, 
  .nl_params = &nl_33_nl_params, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_34_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_25_output, &nl_33_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_34_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  eltwise_34_layer, 34,
  ELTWISE_INTEGER_TYPE, 0x0, NULL,
  eltwise_integer, forward_eltwise_integer_INT8,
  &eltwise_34_chain,
  NULL, &eltwise_34_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_INT8, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  pool_38_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_37_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &pool_38_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  pool_38_layer, 38,
  POOL_TYPE, 0x0, NULL,
  pool, forward_ap_integer_INT8,
  &pool_38_chain,
  NULL, &pool_38_layer, AI_STATIC, 
  .pool_size = AI_SHAPE_2D_INIT(7, 7), 
  .pool_stride = AI_SHAPE_2D_INIT(7, 7), 
  .pool_pad = AI_SHAPE_INIT(4, 0, 0, 0, 0), 
)


AI_STATIC_CONST ai_i8 nl_45_nl_params_data[] = { -57, -56, -56, -56, -55, -55, -54, -54, -54, -53, -53, -53, -52, -52, -51, -51, -51, -50, -50, -50, -49, -49, -48, -48, -48, -47, -47, -46, -46, -46, -45, -45, -45, -44, -44, -43, -43, -43, -42, -42, -41, -41, -41, -40, -40, -39, -39, -39, -38, -38, -37, -37, -37, -36, -36, -35, -35, -34, -34, -34, -33, -33, -32, -32, -32, -31, -31, -30, -30, -29, -29, -29, -28, -28, -27, -27, -26, -26, -26, -25, -25, -24, -24, -23, -23, -23, -22, -22, -21, -21, -20, -20, -20, -19, -19, -18, -18, -17, -17, -16, -16, -16, -15, -15, -14, -14, -13, -13, -12, -12, -12, -11, -11, -10, -10, -9, -9, -8, -8, -8, -7, -7, -6, -6, -5, -5, -4, -4, -4, -3, -3, -2, -2, -1, -1, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 20, 21, 21, 22, 22, 23, 23, 23, 24, 24, 25, 25, 26, 26, 26, 27, 27, 28, 28, 29, 29, 29, 30, 30, 31, 31, 32, 32, 32, 33, 33, 34, 34, 34, 35, 35, 36, 36, 37, 37, 37, 38, 38, 39, 39, 39, 40, 40, 41, 41, 41, 42, 42, 43, 43, 43, 44, 44, 45, 45, 45, 46, 46, 46, 47, 47, 48, 48, 48, 49, 49, 50, 50, 50 };
AI_ARRAY_OBJ_DECLARE(
    nl_45_nl_params, AI_ARRAY_FORMAT_S8,
    nl_45_nl_params_data, nl_45_nl_params_data, 256, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  nl_45_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_44_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &nl_45_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  nl_45_layer, 45,
  NL_TYPE, 0x0, NULL,
  nl, forward_nl_integer,
  &nl_45_chain,
  NULL, &nl_45_layer, AI_STATIC, 
  .nl_params = &nl_45_nl_params, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_46_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_37_output, &nl_45_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_46_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  eltwise_46_layer, 46,
  ELTWISE_INTEGER_TYPE, 0x0, NULL,
  eltwise_integer, forward_eltwise_integer_INT8,
  &eltwise_46_chain,
  NULL, &eltwise_46_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_INT8, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  pool_50_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_49_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &pool_50_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  pool_50_layer, 50,
  POOL_TYPE, 0x0, NULL,
  pool, forward_ap_integer_INT8,
  &pool_50_chain,
  NULL, &pool_50_layer, AI_STATIC, 
  .pool_size = AI_SHAPE_2D_INIT(7, 7), 
  .pool_stride = AI_SHAPE_2D_INIT(7, 7), 
  .pool_pad = AI_SHAPE_INIT(4, 0, 0, 0, 0), 
)


AI_STATIC_CONST ai_i8 nl_57_nl_params_data[] = { -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -125, -125, -125, -125, -125, -125, -125, -124, -124, -124, -124, -124, -123, -123, -123, -123, -122, -122, -122, -122, -121, -121, -120, -120, -120, -119, -119, -118, -118, -117, -117, -116, -116, -115, -115, -114, -113, -112, -112, -111, -110, -109, -108, -107, -106, -105, -104, -103, -102, -101, -100, -98, -97, -95, -94, -92, -91, -89, -88, -86, -84, -82, -80, -78, -76, -74, -72, -70, -67, -65, -62, -60, -57, -55, -52, -49, -46, -44, -41, -38, -35, -32, -29, -26, -22, -19, -16, -13, -10, -6, -3, 0, 3, 6, 10, 13, 16, 19, 22, 26, 29, 32, 35, 38, 41, 44, 46, 49, 52, 55, 57, 60, 62, 65, 67, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 89, 91, 92, 94, 95, 97, 98, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 112, 113, 114, 115, 115, 116, 116, 117, 117, 118, 118, 119, 119, 120, 120, 120, 121, 121, 122, 122, 122, 122, 123, 123, 123, 123, 124, 124, 124, 124, 124, 125, 125, 125, 125, 125, 125, 125, 126, 126, 126, 126, 126, 126 };
AI_ARRAY_OBJ_DECLARE(
    nl_57_nl_params, AI_ARRAY_FORMAT_S8,
    nl_57_nl_params_data, nl_57_nl_params_data, 256, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  nl_57_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_56_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &nl_57_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  nl_57_layer, 57,
  NL_TYPE, 0x0, NULL,
  nl, forward_nl_integer,
  &nl_57_chain,
  NULL, &nl_57_layer, AI_STATIC, 
  .nl_params = &nl_57_nl_params, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_58_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_49_output, &nl_57_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_58_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  eltwise_58_layer, 58,
  ELTWISE_INTEGER_TYPE, 0x0, NULL,
  eltwise_integer, forward_eltwise_integer_INT8,
  &eltwise_58_chain,
  NULL, &eltwise_58_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_INT8, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  pool_61_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_60_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &pool_61_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  pool_61_layer, 61,
  POOL_TYPE, 0x0, NULL,
  pool, forward_ap_integer_INT8,
  &pool_61_chain,
  NULL, &pool_61_layer, AI_STATIC, 
  .pool_size = AI_SHAPE_2D_INIT(7, 7), 
  .pool_stride = AI_SHAPE_2D_INIT(7, 7), 
  .pool_pad = AI_SHAPE_INIT(4, 0, 0, 0, 0), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  gemm_62_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &pool_61_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gemm_62_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &gemm_62_weights, &gemm_62_bias),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gemm_62_scratch0)
)

AI_LAYER_OBJ_DECLARE(
  gemm_62_layer, 62,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense_integer_SSSA_ch,
  &gemm_62_chain,
  NULL, &gemm_62_layer, AI_STATIC, 
)
/**  Hybrid layers declarations section  *************************************/
void forward_lite_pool_2(_stai_network_context* net_ctx)
{
  conv2d_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 65344);
  conv2d_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 65344);
  pool_2_output_array.data = AI_PTR(net_ctx->_activations[0] + 77888);
  pool_2_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 77888);
  _STAI_NETWORK_EVENT_NODE_START_CB(2, 1, { conv2d_1_output.data->data});
  forward_ap_integer_INT8(&pool_2_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(2, 1, { pool_2_output.data->data});
}
void forward_lite_nl_9(_stai_network_context* net_ctx)
{
  conv2d_8_output_array.data = AI_PTR(net_ctx->_activations[0] + 80768);
  conv2d_8_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 80768);
  nl_9_output_array.data = AI_PTR(net_ctx->_activations[0] + 80784);
  nl_9_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 80784);
  _STAI_NETWORK_EVENT_NODE_START_CB(9, 1, { conv2d_8_output.data->data});
  forward_nl_integer(&nl_9_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(9, 1, { nl_9_output.data->data});
}
void forward_lite_eltwise_10(_stai_network_context* net_ctx)
{
  conv2d_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 65344);
  conv2d_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 65344);
  nl_9_output_array.data = AI_PTR(net_ctx->_activations[0] + 80784);
  nl_9_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 80784);
  eltwise_10_output_array.data = AI_PTR(net_ctx->_activations[0] + 65344);
  eltwise_10_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 65344);
  _STAI_NETWORK_EVENT_NODE_START_CB(10, 2, { conv2d_1_output.data->data,nl_9_output.data->data});
  forward_eltwise_integer_INT8(&eltwise_10_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(10, 1, { eltwise_10_output.data->data});
}
void forward_lite_pool_14(_stai_network_context* net_ctx)
{
  conv2d_13_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  conv2d_13_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  pool_14_output_array.data = AI_PTR(net_ctx->_activations[0] + 18816);
  pool_14_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 18816);
  _STAI_NETWORK_EVENT_NODE_START_CB(14, 1, { conv2d_13_output.data->data});
  forward_ap_integer_INT8(&pool_14_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(14, 1, { pool_14_output.data->data});
}
void forward_lite_nl_21(_stai_network_context* net_ctx)
{
  conv2d_20_output_array.data = AI_PTR(net_ctx->_activations[0] + 18816);
  conv2d_20_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 18816);
  nl_21_output_array.data = AI_PTR(net_ctx->_activations[0] + 18912);
  nl_21_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 18912);
  _STAI_NETWORK_EVENT_NODE_START_CB(21, 1, { conv2d_20_output.data->data});
  forward_nl_integer(&nl_21_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(21, 1, { nl_21_output.data->data});
}
void forward_lite_eltwise_22(_stai_network_context* net_ctx)
{
  conv2d_13_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  conv2d_13_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  nl_21_output_array.data = AI_PTR(net_ctx->_activations[0] + 18912);
  nl_21_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 18912);
  eltwise_22_output_array.data = AI_PTR(net_ctx->_activations[0] + 19008);
  eltwise_22_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 19008);
  _STAI_NETWORK_EVENT_NODE_START_CB(22, 2, { conv2d_13_output.data->data,nl_21_output.data->data});
  forward_eltwise_integer_INT8(&eltwise_22_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(22, 1, { eltwise_22_output.data->data});
}
void forward_lite_pool_26(_stai_network_context* net_ctx)
{
  conv2d_25_output_array.data = AI_PTR(net_ctx->_activations[0] + 5332);
  conv2d_25_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 5332);
  pool_26_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  pool_26_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(26, 1, { conv2d_25_output.data->data});
  forward_ap_integer_INT8(&pool_26_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(26, 1, { pool_26_output.data->data});
}
void forward_lite_nl_33(_stai_network_context* net_ctx)
{
  conv2d_32_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  conv2d_32_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  nl_33_output_array.data = AI_PTR(net_ctx->_activations[0] + 144);
  nl_33_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 144);
  _STAI_NETWORK_EVENT_NODE_START_CB(33, 1, { conv2d_32_output.data->data});
  forward_nl_integer(&nl_33_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(33, 1, { nl_33_output.data->data});
}
void forward_lite_eltwise_34(_stai_network_context* net_ctx)
{
  conv2d_25_output_array.data = AI_PTR(net_ctx->_activations[0] + 5332);
  conv2d_25_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 5332);
  nl_33_output_array.data = AI_PTR(net_ctx->_activations[0] + 144);
  nl_33_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 144);
  eltwise_34_output_array.data = AI_PTR(net_ctx->_activations[0] + 33556);
  eltwise_34_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 33556);
  _STAI_NETWORK_EVENT_NODE_START_CB(34, 2, { conv2d_25_output.data->data,nl_33_output.data->data});
  forward_eltwise_integer_INT8(&eltwise_34_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(34, 1, { eltwise_34_output.data->data});
}
void forward_lite_pool_38(_stai_network_context* net_ctx)
{
  conv2d_37_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  conv2d_37_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  pool_38_output_array.data = AI_PTR(net_ctx->_activations[0] + 7056);
  pool_38_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 7056);
  _STAI_NETWORK_EVENT_NODE_START_CB(38, 1, { conv2d_37_output.data->data});
  forward_ap_integer_INT8(&pool_38_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(38, 1, { pool_38_output.data->data});
}
void forward_lite_nl_45(_stai_network_context* net_ctx)
{
  conv2d_44_output_array.data = AI_PTR(net_ctx->_activations[0] + 7056);
  conv2d_44_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 7056);
  nl_45_output_array.data = AI_PTR(net_ctx->_activations[0] + 7200);
  nl_45_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 7200);
  _STAI_NETWORK_EVENT_NODE_START_CB(45, 1, { conv2d_44_output.data->data});
  forward_nl_integer(&nl_45_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(45, 1, { nl_45_output.data->data});
}
void forward_lite_eltwise_46(_stai_network_context* net_ctx)
{
  conv2d_37_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  conv2d_37_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  nl_45_output_array.data = AI_PTR(net_ctx->_activations[0] + 7200);
  nl_45_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 7200);
  eltwise_46_output_array.data = AI_PTR(net_ctx->_activations[0] + 7344);
  eltwise_46_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 7344);
  _STAI_NETWORK_EVENT_NODE_START_CB(46, 2, { conv2d_37_output.data->data,nl_45_output.data->data});
  forward_eltwise_integer_INT8(&eltwise_46_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(46, 1, { eltwise_46_output.data->data});
}
void forward_lite_pool_50(_stai_network_context* net_ctx)
{
  conv2d_49_output_array.data = AI_PTR(net_ctx->_activations[0] + 37660);
  conv2d_49_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 37660);
  pool_50_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  pool_50_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(50, 1, { conv2d_49_output.data->data});
  forward_ap_integer_INT8(&pool_50_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(50, 1, { pool_50_output.data->data});
}
void forward_lite_nl_57(_stai_network_context* net_ctx)
{
  conv2d_56_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  conv2d_56_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  nl_57_output_array.data = AI_PTR(net_ctx->_activations[0] + 240);
  nl_57_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 240);
  _STAI_NETWORK_EVENT_NODE_START_CB(57, 1, { conv2d_56_output.data->data});
  forward_nl_integer(&nl_57_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(57, 1, { nl_57_output.data->data});
}
void forward_lite_eltwise_58(_stai_network_context* net_ctx)
{
  conv2d_49_output_array.data = AI_PTR(net_ctx->_activations[0] + 37660);
  conv2d_49_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 37660);
  nl_57_output_array.data = AI_PTR(net_ctx->_activations[0] + 240);
  nl_57_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 240);
  eltwise_58_output_array.data = AI_PTR(net_ctx->_activations[0] + 480);
  eltwise_58_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 480);
  _STAI_NETWORK_EVENT_NODE_START_CB(58, 2, { conv2d_49_output.data->data,nl_57_output.data->data});
  forward_eltwise_integer_INT8(&eltwise_58_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(58, 1, { eltwise_58_output.data->data});
}
void forward_lite_pool_61(_stai_network_context* net_ctx)
{
  conv2d_60_output_array.data = AI_PTR(net_ctx->_activations[0] + 800);
  conv2d_60_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 800);
  pool_61_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  pool_61_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(61, 1, { conv2d_60_output.data->data});
  forward_ap_integer_INT8(&pool_61_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(61, 1, { pool_61_output.data->data});
}
void forward_lite_gemm_62(_stai_network_context* net_ctx)
{
  pool_61_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  pool_61_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  gemm_62_weights_array.data = AI_PTR(net_ctx->_weights[52] + 0);
  gemm_62_weights_array.data_start = AI_PTR(net_ctx->_weights[52] + 0);
  gemm_62_bias_array.data = AI_PTR(net_ctx->_weights[53] + 0);
  gemm_62_bias_array.data_start = AI_PTR(net_ctx->_weights[53] + 0);
  gemm_62_scratch0_array.data = AI_PTR(net_ctx->_activations[0] + 64);
  gemm_62_scratch0_array.data_start = AI_PTR(net_ctx->_activations[0] + 64);
  gemm_62_output_array.data = AI_PTR(net_ctx->_activations[0] + 292);
  gemm_62_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 292);
  _STAI_NETWORK_EVENT_NODE_START_CB(62, 1, { pool_61_output.data->data});
  forward_dense_integer_SSSA_ch(&gemm_62_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(62, 1, { gemm_62_output.data->data});
}

/*****************************************************************************/


static const ai_u16 conv2d_0_t_in_0_shape_w_const_u16 = 28;
static const ai_u16 conv2d_0_t_in_0_shape_h_const_u16 = 28;
static const ai_u16 conv2d_0_t_in_0_shape_ch_const_u16 = 1;
static const ai_u16 conv2d_0_t_out_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_0_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_0_t_weight_0_shape_h_const_u16 = 3;
static const ai_u16 conv2d_0_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_0_l_stride_0_const_u16 = 1;
static const ai_i32 conv2d_0_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_0_l_pad_H_0_const_s32 = 1;
static const ai_i8 conv2d_0_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_0_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_0_t_in_0_fmt_scale_const_f32 = 0.003921568859368563f;
static const ai_float conv2d_0_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_0_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.016165917739272118f, 0.013012140057981014f, 0.020047534257173538f, 0.012101773172616959f, 0.009379937313497066f, 0.01860274001955986f, 0.007923663593828678f, 0.011563667096197605f, 0.017760751768946648f, 0.013779576867818832f, 0.017264213413000107f, 0.012449931353330612f, 0.006908439565449953f, 0.018861286342144012f, 0.012674076482653618f, 0.02130036987364292f);
static const ai_layer_format_type conv2d_0_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_0_t_out_0_shape_w_const_u16 = 28;
static const ai_u16 conv2d_0_t_out_0_shape_h_const_u16 = 28;

static const ai_i8 conv2d_1_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_1_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_1_pad_before_t_in_0_shape_h_const_u32 = 28;

static const ai_u16 conv2d_1_t_in_0_shape_w_const_u16 = 30;
static const ai_u16 conv2d_1_t_in_0_shape_h_const_u16 = 30;
static const ai_u16 conv2d_1_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_1_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_1_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_1_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_1_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_1_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_1_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_1_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.014340481720864773f, 0.004711237270385027f, 0.017332598567008972f, 0.012302144430577755f, 0.026740742847323418f, 0.008435329422354698f, 0.013753595761954784f, 0.008893351070582867f, 0.003591791493818164f, 0.006808540318161249f, 0.009225944988429546f, 0.01744774729013443f, 0.016596080735325813f, 0.009321599267423153f, 0.00550667243078351f, 0.007544306572526693f);
static const ai_u16 conv2d_1_t_out_0_shape_w_const_u16 = 28;
static const ai_u16 conv2d_1_t_out_0_shape_h_const_u16 = 28;


static const ai_u16 conv2d_7_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_7_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_7_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_7_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_7_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_7_t_out_0_shape_ch_const_u16 = 4;
static const ai_i8 conv2d_7_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_7_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_7_t_in_0_fmt_scale_const_f32 = 0.002881338819861412f;
static const ai_float conv2d_7_t_out_0_fmt_scale_const_f32 = 0.0024960017763078213f;
static const ai_float conv2d_7_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.004137103445827961f, 0.0039538489654660225f, 0.004415956325829029f, 0.004268535878509283f);
static const ai_layer_format_type conv2d_7_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_8_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_8_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_8_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_8_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_8_t_in_0_shape_ch_const_u16 = 4;
static const ai_u16 conv2d_8_t_out_0_shape_ch_const_u16 = 16;
static const ai_i8 conv2d_8_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_8_t_out_0_fmt_zero_const_s8 = 9;
static const ai_float conv2d_8_t_in_0_fmt_scale_const_f32 = 0.0024960017763078213f;
static const ai_float conv2d_8_t_out_0_fmt_scale_const_f32 = 0.004110268782824278f;
static const ai_float conv2d_8_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.004460854921489954f, 0.003765507834032178f, 0.0025543984957039356f, 0.004021105356514454f, 0.0044100587256252766f, 0.004544314928352833f, 0.0035027186386287212f, 0.00239015300758183f, 0.002634357661008835f, 0.0059043606743216515f, 0.003020441858097911f, 0.0032573856879025698f, 0.003097671316936612f, 0.00351132033392787f, 0.0042478046379983425f, 0.005545585881918669f);
static const ai_layer_format_type conv2d_8_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;



static const ai_u16 conv2d_11_t_in_0_shape_w_const_u16 = 28;
static const ai_u16 conv2d_11_t_in_0_shape_h_const_u16 = 28;
static const ai_u16 conv2d_11_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_11_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_11_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_11_t_out_0_shape_ch_const_u16 = 16;
static const ai_i8 conv2d_11_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_11_t_out_0_fmt_zero_const_s8 = 0;
static const ai_float conv2d_11_t_in_0_fmt_scale_const_f32 = 0.014061874710023403f;
static const ai_float conv2d_11_t_out_0_fmt_scale_const_f32 = 0.06521018594503403f;
static const ai_float conv2d_11_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.010960276238620281f, 0.010737097822129726f, 0.012043115682899952f, 0.009427644312381744f, 0.010128868743777275f, 0.011489719152450562f, 0.009491566568613052f, 0.01418798603117466f, 0.015937717631459236f, 0.008997318334877491f, 0.010471615940332413f, 0.006153814494609833f, 0.012133333832025528f, 0.009138176217675209f, 0.009293884970247746f, 0.011756020598113537f);
static const ai_layer_format_type conv2d_11_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_12_t_in_0_shape_w_const_u16 = 28;
static const ai_u16 conv2d_12_t_in_0_shape_h_const_u16 = 28;
static const ai_u16 conv2d_12_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_12_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_12_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_12_t_out_0_shape_ch_const_u16 = 96;
static const ai_i8 conv2d_12_t_in_0_fmt_zero_const_s8 = 0;
static const ai_i8 conv2d_12_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_12_t_in_0_fmt_scale_const_f32 = 0.06521018594503403f;
static const ai_float conv2d_12_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_12_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0034094355069100857f, 0.00329149910248816f, 0.0029303841292858124f, 0.0025085103698074818f, 0.002887110924348235f, 0.002584856003522873f, 0.0026852076407521963f, 0.0030174965504556894f, 0.002996693830937147f, 0.005347410216927528f, 0.004547202028334141f, 0.004400857724249363f, 0.003182163927704096f, 0.0038265676703304052f, 0.003402169095352292f, 0.0021653673611581326f, 0.0036844827700406313f, 0.0029525472782552242f, 0.004221857525408268f, 0.0024730951990932226f, 0.003053902182728052f, 0.0032367934472858906f, 0.002134795067831874f, 0.0021639035549014807f, 0.0030919229611754417f, 0.0034844083711504936f, 0.003379796864464879f, 0.0025712403003126383f, 0.003548393025994301f, 0.003086241427809f, 0.005267501343041658f, 0.0033046870958060026f, 0.003542426275089383f, 0.0035900443326681852f, 0.0029772373382002115f, 0.0024466202594339848f, 0.003858237760141492f, 0.004018102306872606f, 0.003569984110072255f, 0.004024319816380739f, 0.002727675950154662f, 0.005613906309008598f, 0.0026685234624892473f, 0.003250173758715391f, 0.003286784514784813f, 0.003952528350055218f, 0.0032397035975009203f, 0.0034645642153918743f, 0.0036845800932496786f, 0.0031531411223113537f, 0.003209426300600171f, 0.005233590956777334f, 0.0032481581438332796f, 0.003186031710356474f, 0.0031032939441502094f, 0.0035833928268402815f, 0.0026669781655073166f, 0.002816068707033992f, 0.004030626732856035f, 0.0035640292335301638f, 0.0036970707587897778f, 0.0031162372324615717f, 0.0022714841179549694f, 0.0034764185547828674f, 0.0020550822373479605f, 0.003035672241821885f, 0.004088967107236385f, 0.0040401313453912735f, 0.003956452943384647f, 0.0038969297893345356f, 0.0030849198810756207f, 0.002750842133536935f, 0.0037769607733935118f, 0.0024840461555868387f, 0.002816902007907629f, 0.0029859140049666166f, 0.0030546747148036957f, 0.0038482279051095247f, 0.0028622779063880444f, 0.002985683036968112f, 0.004759000614285469f, 0.0032987857703119516f, 0.0030962228775024414f, 0.002678970107808709f, 0.0024131007958203554f, 0.003208398586139083f, 0.0029076861683279276f, 0.003794235410168767f, 0.003614185843616724f, 0.0026469524018466473f, 0.00356096844188869f, 0.003232652321457863f, 0.004091614857316017f, 0.005025843158364296f, 0.0023007141426205635f, 0.003588945372030139f);
static const ai_layer_format_type conv2d_12_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_13_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_13_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_13_pad_before_t_in_0_shape_h_const_u32 = 28;

static const ai_u16 conv2d_13_t_in_0_shape_w_const_u16 = 30;
static const ai_u16 conv2d_13_t_in_0_shape_h_const_u16 = 30;
static const ai_u16 conv2d_13_t_in_0_shape_ch_const_u16 = 96;
static const ai_u16 conv2d_13_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_13_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_13_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_13_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_13_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_13_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_13_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.009308693930506706f, 0.006429887376725674f, 0.004593629855662584f, 0.0087739871814847f, 0.007660144940018654f, 0.002896771766245365f, 0.006336180958896875f, 0.007815809920430183f, 0.004662069026380777f, 0.005789710208773613f, 0.006077741738408804f, 0.00994136556982994f, 0.007688205223530531f, 0.003307529492303729f, 0.005553812719881535f, 0.00497591495513916f, 0.01067938283085823f, 0.006222686264663935f, 0.009127778001129627f, 0.012980821542441845f, 0.010660147294402122f, 0.00722659844905138f, 0.005468222312629223f, 0.008129443973302841f, 0.0067429435439407825f, 0.006500020157545805f, 0.0038135454524308443f, 0.006615090183913708f, 0.004926681984215975f, 0.0073822131380438805f, 0.005500379018485546f, 0.00528255570679903f, 0.0062899538315832615f, 0.007603238336741924f, 0.0063424198888242245f, 0.004647842608392239f, 0.0077582369558513165f, 0.006997279357165098f, 0.0026075798086822033f, 0.004755709320306778f, 0.006665796972811222f, 0.004690034314990044f, 0.006616143975406885f, 0.003607403254136443f, 0.007765231188386679f, 0.005656672641634941f, 0.004778085742145777f, 0.008225270546972752f, 0.006095751188695431f, 0.010487188585102558f, 0.005676862318068743f, 0.010303651914000511f, 0.006715994793921709f, 0.014795093797147274f, 0.01014065183699131f, 0.009494183585047722f, 0.0036522108130156994f, 0.003809451824054122f, 0.0034580938518047333f, 0.010676473379135132f, 0.012304005213081837f, 0.004318206571042538f, 0.004790032748132944f, 0.013464832678437233f, 0.004873187746852636f, 0.010249348357319832f, 0.007829916663467884f, 0.003487883834168315f, 0.0050603291019797325f, 0.005442079156637192f, 0.005080068949609995f, 0.005600316449999809f, 0.005975143052637577f, 0.004241227172315121f, 0.008110915310680866f, 0.008382495492696762f, 0.005431623198091984f, 0.008202390745282173f, 0.005207543261349201f, 0.008047164417803288f, 0.0029419336933642626f, 0.005931855645030737f, 0.005994806531816721f, 0.006979247089475393f, 0.0029048514552414417f, 0.00838254764676094f, 0.007838092744350433f, 0.004296577535569668f, 0.00686457846313715f, 0.005224738735705614f, 0.004588767886161804f, 0.004953221417963505f, 0.004961853381246328f, 0.004667580593377352f, 0.010388460010290146f, 0.006610885728150606f);
static const ai_u16 conv2d_13_t_out_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_13_t_out_0_shape_h_const_u16 = 14;


static const ai_u16 conv2d_19_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_19_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_19_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_19_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_19_t_in_0_shape_ch_const_u16 = 96;
static const ai_u16 conv2d_19_t_out_0_shape_ch_const_u16 = 24;
static const ai_i8 conv2d_19_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_19_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_19_t_in_0_fmt_scale_const_f32 = 0.003947367426007986f;
static const ai_float conv2d_19_t_out_0_fmt_scale_const_f32 = 0.009633461944758892f;
static const ai_float conv2d_19_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0017514625797048211f, 0.002363099018111825f, 0.0026791850104928017f, 0.0018513979157432914f, 0.001741657848469913f, 0.0017396964831277728f, 0.002711427630856633f, 0.001746981404721737f, 0.002000362379476428f, 0.001957595581188798f, 0.0022864476777613163f, 0.00221705948933959f, 0.0017160713905468583f, 0.0020483816042542458f, 0.0017806616378948092f, 0.002083205385133624f, 0.0018997081788256764f, 0.0029329739045351744f, 0.001751361764036119f, 0.0017885674024000764f, 0.0023049304727464914f, 0.0023678888101130724f, 0.0017288308590650558f, 0.0019948456902056932f);
static const ai_layer_format_type conv2d_19_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_20_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_20_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_20_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_20_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_20_t_in_0_shape_ch_const_u16 = 24;
static const ai_u16 conv2d_20_t_out_0_shape_ch_const_u16 = 96;
static const ai_i8 conv2d_20_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_20_t_out_0_fmt_zero_const_s8 = 0;
static const ai_float conv2d_20_t_in_0_fmt_scale_const_f32 = 0.009633461944758892f;
static const ai_float conv2d_20_t_out_0_fmt_scale_const_f32 = 0.011498714797198772f;
static const ai_float conv2d_20_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0018057021079584956f, 0.0024096998386085033f, 0.002143289428204298f, 0.0020509848836809397f, 0.0022309492342174053f, 0.001795834512449801f, 0.001810995046980679f, 0.002469903090968728f, 0.0017555839149281383f, 0.0025256837252527475f, 0.0019303212175145745f, 0.0018299418734386563f, 0.0018862748984247446f, 0.002701032441109419f, 0.0022448331583291292f, 0.002130116568878293f, 0.0017804414965212345f, 0.0018409558106213808f, 0.0016285459278151393f, 0.0015439887065440416f, 0.0018125526839867234f, 0.0023098632227629423f, 0.0022209042217582464f, 0.0017349193803966045f, 0.0019104625098407269f, 0.001955232350155711f, 0.0026423598174005747f, 0.0017836003098636866f, 0.0020639346912503242f, 0.0023834083694964647f, 0.0022574071772396564f, 0.001580171985551715f, 0.0017004698747768998f, 0.0021177649032324553f, 0.002113315276801586f, 0.0024558864533901215f, 0.0015983331250026822f, 0.0018937267595902085f, 0.0021819511894136667f, 0.0020211918745189905f, 0.0018004763405770063f, 0.0018676836043596268f, 0.0019481205381453037f, 0.002496969187632203f, 0.0017905575223267078f, 0.0017564739100635052f, 0.0016612338367849588f, 0.002549811266362667f, 0.0018120214808732271f, 0.0022008034866303205f, 0.0024071598891168833f, 0.00160537613555789f, 0.0020151492208242416f, 0.001747639151290059f, 0.0024590431712567806f, 0.001652552979066968f, 0.001982350368052721f, 0.0017714874120429158f, 0.001607994781807065f, 0.002287378069013357f, 0.0016875488217920065f, 0.003406030358746648f, 0.0025739187840372324f, 0.0023233178071677685f, 0.0017444770783185959f, 0.0016689582262188196f, 0.0016244393773376942f, 0.0014915771316736937f, 0.0018090427620336413f, 0.001925898715853691f, 0.0017054713098332286f, 0.0018251518486067653f, 0.0017404811223968863f, 0.0024727110285311937f, 0.0017066283617168665f, 0.0019572663586586714f, 0.0018035308457911015f, 0.0016061104834079742f, 0.0018421646673232317f, 0.002027227310463786f, 0.0018961929017677903f, 0.0017113020876422524f, 0.001972784986719489f, 0.002068686531856656f, 0.002709756838157773f, 0.001947922632098198f, 0.002243567258119583f, 0.0021549842786043882f, 0.0018395386869087815f, 0.0020637912675738335f, 0.0021260534413158894f, 0.0016498210607096553f, 0.0021074817050248384f, 0.001739052007906139f, 0.0016437415033578873f, 0.002265489660203457f);
static const ai_layer_format_type conv2d_20_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;



static const ai_u16 conv2d_23_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_23_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_23_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_23_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_23_t_in_0_shape_ch_const_u16 = 96;
static const ai_u16 conv2d_23_t_out_0_shape_ch_const_u16 = 24;
static const ai_i8 conv2d_23_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_23_t_out_0_fmt_zero_const_s8 = -1;
static const ai_float conv2d_23_t_in_0_fmt_scale_const_f32 = 0.017011865973472595f;
static const ai_float conv2d_23_t_out_0_fmt_scale_const_f32 = 0.0646810457110405f;
static const ai_float conv2d_23_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.004776870831847191f, 0.004568558651953936f, 0.004510619677603245f, 0.003816199256107211f, 0.004806050099432468f, 0.003626713529229164f, 0.0036107050254940987f, 0.004998703952878714f, 0.005210149567574263f, 0.0051205032505095005f, 0.0052165621891617775f, 0.004120251163840294f, 0.00396369444206357f, 0.005281648598611355f, 0.004319642670452595f, 0.0035413880832493305f, 0.0037647190038114786f, 0.005187229719012976f, 0.004529171623289585f, 0.0043654371984303f, 0.00522843049839139f, 0.0038298836443573236f, 0.0046012867242097855f, 0.0029857628978788853f);
static const ai_layer_format_type conv2d_23_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_24_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_24_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_24_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_24_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_24_t_in_0_shape_ch_const_u16 = 24;
static const ai_u16 conv2d_24_t_out_0_shape_ch_const_u16 = 144;
static const ai_i8 conv2d_24_t_in_0_fmt_zero_const_s8 = -1;
static const ai_i8 conv2d_24_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_24_t_in_0_fmt_scale_const_f32 = 0.0646810457110405f;
static const ai_float conv2d_24_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_24_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0029299601446837187f, 0.00282250065356493f, 0.0024532508105039597f, 0.002001232234761119f, 0.002897517988458276f, 0.0032256750855594873f, 0.003093009814620018f, 0.003080714028328657f, 0.002881428459659219f, 0.003545102197676897f, 0.002937417710199952f, 0.0027230919804424047f, 0.002949099289253354f, 0.003250734880566597f, 0.003306014696136117f, 0.002404827857390046f, 0.002327120862901211f, 0.0024329647421836853f, 0.0038246826734393835f, 0.003081758739426732f, 0.0022261596750468016f, 0.0028435722924768925f, 0.0024726074188947678f, 0.003606010228395462f, 0.0025882963091135025f, 0.003039366565644741f, 0.0031134255696088076f, 0.002877919701859355f, 0.0037051134277135134f, 0.0024188372772186995f, 0.004245180636644363f, 0.003031299915164709f, 0.0024243120569735765f, 0.004630030132830143f, 0.002299530664458871f, 0.0019579329527914524f, 0.0024857318494468927f, 0.0024668967816978693f, 0.00347224622964859f, 0.002103502629324794f, 0.0032999080140143633f, 0.0028068579267710447f, 0.0026260954327881336f, 0.003030285704880953f, 0.001992132281884551f, 0.003128133015707135f, 0.0029025620315223932f, 0.0026201957371085882f, 0.002420582342892885f, 0.004262540023773909f, 0.0024893763475120068f, 0.0031063458882272243f, 0.0029476750642061234f, 0.003338069189339876f, 0.0018759150989353657f, 0.002144196070730686f, 0.0031643591355532408f, 0.0027381198015064f, 0.002502067480236292f, 0.002813321305438876f, 0.0025861170142889023f, 0.0028233123011887074f, 0.004082788713276386f, 0.003219765843823552f, 0.003040970303118229f, 0.004443132784217596f, 0.002875109203159809f, 0.004393735900521278f, 0.0017900406382977962f, 0.0022015392314642668f, 0.002379236277192831f, 0.0036587996874004602f, 0.0026065553538501263f, 0.00279337540268898f, 0.0028014180716127157f, 0.0026273380499333143f, 0.003668994177132845f, 0.003100884146988392f, 0.0025453190319240093f, 0.002630524802953005f, 0.0027808104641735554f, 0.0031816461123526096f, 0.002142076613381505f, 0.003427710384130478f, 0.0027960683219134808f, 0.002274896251037717f, 0.002179231960326433f, 0.0025918290484696627f, 0.0030221983324736357f, 0.002388958353549242f, 0.0025158850476145744f, 0.0025333650410175323f, 0.002793825464323163f, 0.0036465292796492577f, 0.0030536844860762358f, 0.003214178141206503f, 0.0025045708753168583f, 0.0022809323854744434f, 0.0022508155088871717f, 0.002420067088678479f, 0.002572873840108514f, 0.0021164766512811184f, 0.0019531487487256527f, 0.0025724517181515694f, 0.002303399844095111f, 0.002934579737484455f, 0.002485130447894335f, 0.002184205688536167f, 0.0022794604301452637f, 0.0037152476143091917f, 0.002159045310690999f, 0.0021384006831794977f, 0.0018834939692169428f, 0.0015228617703542113f, 0.0027048320043832064f, 0.0026279333978891373f, 0.0028717995155602694f, 0.0036185309290885925f, 0.0020893707405775785f, 0.0025383522734045982f, 0.003057517111301422f, 0.002401139819994569f, 0.0027673665899783373f, 0.0024900573771446943f, 0.0029071588069200516f, 0.0023052936885505915f, 0.003158674808219075f, 0.003768311347812414f, 0.0038541071116924286f, 0.0028235563077032566f, 0.002386298030614853f, 0.003151850076392293f, 0.0029535298235714436f, 0.003173498436808586f, 0.0021298418287187815f, 0.0027974972035735846f, 0.002842171350494027f, 0.0029973862692713737f, 0.0036321303341537714f, 0.0031663666013628244f, 0.0029005350079387426f, 0.0025912122800946236f, 0.0021452561486512423f, 0.0018524673068895936f);
static const ai_layer_format_type conv2d_24_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_25_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_25_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_25_pad_before_t_in_0_shape_h_const_u32 = 14;

static const ai_u16 conv2d_25_t_in_0_shape_w_const_u16 = 16;
static const ai_u16 conv2d_25_t_in_0_shape_h_const_u16 = 16;
static const ai_u16 conv2d_25_t_in_0_shape_ch_const_u16 = 144;
static const ai_u16 conv2d_25_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_25_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_25_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_25_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_25_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_25_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_25_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.012954001314938068f, 0.008627223782241344f, 0.0054194252006709576f, 0.007967677898705006f, 0.006592866498976946f, 0.009914958849549294f, 0.005180669948458672f, 0.009077883325517178f, 0.00679347850382328f, 0.006358698941767216f, 0.00804994348436594f, 0.01076546497642994f, 0.005537695717066526f, 0.010340875945985317f, 0.01062013953924179f, 0.003519864287227392f, 0.004123757593333721f, 0.005616380833089352f, 0.005098089575767517f, 0.013688500970602036f, 0.010094367899000645f, 0.005235819611698389f, 0.009031252935528755f, 0.0065325177274644375f, 0.006426663603633642f, 0.006263820454478264f, 0.01252205390483141f, 0.010715113021433353f, 0.005226020235568285f, 0.008001954294741154f, 0.006390565074980259f, 0.009287383407354355f, 0.004341614432632923f, 0.004886492155492306f, 0.009031704626977444f, 0.005543222185224295f, 0.010912269353866577f, 0.004250711761415005f, 0.006891455035656691f, 0.0064614312723279f, 0.004634999204427004f, 0.010537214577198029f, 0.007880561985075474f, 0.00517881428822875f, 0.0068488153629004955f, 0.00525315385311842f, 0.0035987079609185457f, 0.009778848849236965f, 0.008856834843754768f, 0.0027482237201184034f, 0.006255561485886574f, 0.005849634762853384f, 0.006287096533924341f, 0.005207412410527468f, 0.007909787818789482f, 0.01231649611145258f, 0.006063902284950018f, 0.012432487681508064f, 0.007094783242791891f, 0.006905853748321533f, 0.004473995883017778f, 0.005039893090724945f, 0.007350700441747904f, 0.0039842636324465275f, 0.0046502407640218735f, 0.005377704743295908f, 0.006300353445112705f, 0.005861107259988785f, 0.007754220627248287f, 0.009219236671924591f, 0.011238993145525455f, 0.007110534701496363f, 0.004607124254107475f, 0.0035940625239163637f, 0.011029725894331932f, 0.011949571780860424f, 0.00609275046736002f, 0.00719982385635376f, 0.008213295601308346f, 0.006267426535487175f, 0.004967946093529463f, 0.00806448981165886f, 0.008366743102669716f, 0.007011049427092075f, 0.006357131060212851f, 0.00862933974713087f, 0.004285111092031002f, 0.006951449438929558f, 0.010064140893518925f, 0.007238311227411032f, 0.006556810811161995f, 0.008217561058700085f, 0.007169951219111681f, 0.008825655095279217f, 0.006472833454608917f, 0.006181837059557438f, 0.007870125584304333f, 0.008395710960030556f, 0.0071828728541731834f, 0.009147360920906067f, 0.009842070750892162f, 0.005241838749498129f, 0.013299484737217426f, 0.008866442367434502f, 0.006446061190217733f, 0.010723222978413105f, 0.005288264714181423f, 0.00629988731816411f, 0.005634060595184565f, 0.004004054702818394f, 0.005827451124787331f, 0.011113647371530533f, 0.007535124197602272f, 0.013767478987574577f, 0.007187951821833849f, 0.007343207020312548f, 0.006261039059609175f, 0.0067534903064370155f, 0.010438788682222366f, 0.007023775018751621f, 0.00362038635648787f, 0.006688317283987999f, 0.007666083984076977f, 0.004900687839835882f, 0.004160006996244192f, 0.006586194038391113f, 0.009500528685748577f, 0.0034187501296401024f, 0.006277176085859537f, 0.006134644150733948f, 0.006254245061427355f, 0.011803900822997093f, 0.008627758361399174f, 0.007561453618109226f, 0.010149330832064152f, 0.007574339397251606f, 0.006408751010894775f, 0.007131808437407017f, 0.0060109724290668964f, 0.0065185618586838245f, 0.006568894721567631f, 0.0067208600230515f, 0.0045561594888567924f, 0.005727939773350954f);
static const ai_u16 conv2d_25_t_out_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_25_t_out_0_shape_h_const_u16 = 14;


static const ai_u16 conv2d_31_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_31_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_31_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_31_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_31_t_in_0_shape_ch_const_u16 = 144;
static const ai_u16 conv2d_31_t_out_0_shape_ch_const_u16 = 36;
static const ai_i8 conv2d_31_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_31_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_31_t_in_0_fmt_scale_const_f32 = 0.003610441694036126f;
static const ai_float conv2d_31_t_out_0_fmt_scale_const_f32 = 0.009359263814985752f;
static const ai_float conv2d_31_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0022353623062372208f, 0.0014833526220172644f, 0.0019538048654794693f, 0.001450942363590002f, 0.001433582860045135f, 0.0021839672699570656f, 0.0015311544993892312f, 0.001751248142682016f, 0.0021693881135433912f, 0.0015755320200696588f, 0.0015572471311315894f, 0.001563815283589065f, 0.0015399770345538855f, 0.0018440510611981153f, 0.0018375380896031857f, 0.00160555902402848f, 0.001430060714483261f, 0.001571351196616888f, 0.0016790067311376333f, 0.0015041782753542066f, 0.001409485237672925f, 0.0016874219290912151f, 0.002786286175251007f, 0.0019987758714705706f, 0.0028673249762505293f, 0.00204468029551208f, 0.001954995561391115f, 0.002283998066559434f, 0.001494398689828813f, 0.0014559385599568486f, 0.0014145976165309548f, 0.0019222212722525f, 0.0016623158007860184f, 0.0014311072882264853f, 0.00165152782574296f, 0.001622318523004651f);
static const ai_layer_format_type conv2d_31_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_32_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_32_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_32_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_32_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_32_t_in_0_shape_ch_const_u16 = 36;
static const ai_u16 conv2d_32_t_out_0_shape_ch_const_u16 = 144;
static const ai_i8 conv2d_32_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_32_t_out_0_fmt_zero_const_s8 = -8;
static const ai_float conv2d_32_t_in_0_fmt_scale_const_f32 = 0.009359263814985752f;
static const ai_float conv2d_32_t_out_0_fmt_scale_const_f32 = 0.012024056166410446f;
static const ai_float conv2d_32_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0015031998045742512f, 0.0017670493107289076f, 0.0020658913999795914f, 0.0013426918303593993f, 0.0017090697074308991f, 0.001608225516974926f, 0.0014719365863129497f, 0.0017665677005425096f, 0.0018070110818371177f, 0.0019002135377377272f, 0.0014998120022937655f, 0.001437879865989089f, 0.0015896287513896823f, 0.0018199353944510221f, 0.0015791591722518206f, 0.001736468868330121f, 0.0016294267261400819f, 0.001406002091243863f, 0.0017589781200513244f, 0.00200886488892138f, 0.0015288550639525056f, 0.0013775512343272567f, 0.0016711397329345345f, 0.0016001809854060411f, 0.0014664925402030349f, 0.0016914212610572577f, 0.0016224350547417998f, 0.0016847125953063369f, 0.001896283938549459f, 0.0017336393939331174f, 0.0013898392207920551f, 0.001763270003721118f, 0.001812947099097073f, 0.0017536808736622334f, 0.0020260352175682783f, 0.001971097895875573f, 0.001827480155043304f, 0.001524684252217412f, 0.0014774109004065394f, 0.0021072719246149063f, 0.0014936568913981318f, 0.002019002102315426f, 0.002148357219994068f, 0.0017995875095948577f, 0.0014290068065747619f, 0.0018667373806238174f, 0.0017518698005005717f, 0.0014239337760955095f, 0.001576057868078351f, 0.0018628081306815147f, 0.0017910973401740193f, 0.001522425445728004f, 0.0018290465231984854f, 0.0013341510202735662f, 0.0017703264020383358f, 0.0017689960077404976f, 0.001630360377021134f, 0.0022301424760371447f, 0.001600374118424952f, 0.0019135740585625172f, 0.0015986659564077854f, 0.0018188998801633716f, 0.001989231910556555f, 0.0019727449398487806f, 0.0018413214711472392f, 0.0017390279099345207f, 0.001771099166944623f, 0.001721855835057795f, 0.0016429969109594822f, 0.002134554320946336f, 0.002224702388048172f, 0.0014549464685842395f, 0.002025376074016094f, 0.0016133238095790148f, 0.0016061872011050582f, 0.0014643906615674496f, 0.0013735770480707288f, 0.002141855889931321f, 0.0016274189110845327f, 0.0013480631168931723f, 0.0018918116111308336f, 0.0017979781841859221f, 0.0017572197830304503f, 0.0019496070453897119f, 0.0016997909406200051f, 0.0017398353666067123f, 0.0013909977860748768f, 0.002076999982818961f, 0.0015262202359735966f, 0.0023776625748723745f, 0.0017657666467130184f, 0.0014363416703417897f, 0.001647875178605318f, 0.0018491466762498021f, 0.0016922445502132177f, 0.0016835578717291355f, 0.00183491175994277f, 0.001664640847593546f, 0.0017244704067707062f, 0.001338220201432705f, 0.00163711654022336f, 0.0018110056407749653f, 0.0022297201212495565f, 0.001541219768114388f, 0.0016714184312149882f, 0.001481133047491312f, 0.001816365635022521f, 0.0014454936608672142f, 0.0013910806737840176f, 0.0020106597803533077f, 0.0014948394382372499f, 0.001447998802177608f, 0.0015164243523031473f, 0.0016473655123263597f, 0.0016720533603802323f, 0.0015312370378524065f, 0.0014839753275737166f, 0.0015567904338240623f, 0.0019251698395237327f, 0.0013884804211556911f, 0.0014845302794128656f, 0.001954302890226245f, 0.0018826400628313422f, 0.0014769568806514144f, 0.0018691912991926074f, 0.0025811921805143356f, 0.00206992425955832f, 0.0013923700898885727f, 0.002278747735545039f, 0.0019277053652331233f, 0.0015386063605546951f, 0.0016178282676264644f, 0.001706554088741541f, 0.001771657494828105f, 0.0017965686274692416f, 0.001543268677778542f, 0.0015534423291683197f, 0.001986010931432247f, 0.001613360713236034f, 0.001436740392819047f, 0.001525016617961228f, 0.0014712627744302154f, 0.001653772429563105f, 0.0016900256741791964f);
static const ai_layer_format_type conv2d_32_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;



static const ai_u16 conv2d_35_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_35_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_35_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_35_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_35_t_in_0_shape_ch_const_u16 = 144;
static const ai_u16 conv2d_35_t_out_0_shape_ch_const_u16 = 24;
static const ai_i8 conv2d_35_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_35_t_out_0_fmt_zero_const_s8 = 14;
static const ai_float conv2d_35_t_in_0_fmt_scale_const_f32 = 0.017825670540332794f;
static const ai_float conv2d_35_t_out_0_fmt_scale_const_f32 = 0.06572987139225006f;
static const ai_float conv2d_35_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.002708825282752514f, 0.0032492629252374172f, 0.003519749501720071f, 0.003047977341338992f, 0.004759922623634338f, 0.003454845165833831f, 0.003579942276701331f, 0.003847065381705761f, 0.005022427532821894f, 0.00410127267241478f, 0.003480450715869665f, 0.003449565265327692f, 0.003589412197470665f, 0.004103192128241062f, 0.00299953855574131f, 0.00390230817720294f, 0.003363050054758787f, 0.004624471068382263f, 0.00344927329570055f, 0.0036637939047068357f, 0.0034807382617145777f, 0.0028780174907296896f, 0.0033545021433383226f, 0.003646192839369178f);
static const ai_layer_format_type conv2d_35_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_36_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_36_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_36_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_36_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_36_t_in_0_shape_ch_const_u16 = 24;
static const ai_u16 conv2d_36_t_out_0_shape_ch_const_u16 = 144;
static const ai_i8 conv2d_36_t_in_0_fmt_zero_const_s8 = 14;
static const ai_i8 conv2d_36_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_36_t_in_0_fmt_scale_const_f32 = 0.06572987139225006f;
static const ai_float conv2d_36_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_36_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.002697686431929469f, 0.0040232413448393345f, 0.002786928089335561f, 0.0024595351424068213f, 0.002457893220707774f, 0.0026580209378153086f, 0.0026063276454806328f, 0.0028026641812175512f, 0.0029362980276346207f, 0.0025721958372741938f, 0.002706271829083562f, 0.0020804021041840315f, 0.002202399307861924f, 0.0021727359853684902f, 0.0031330971978604794f, 0.0022484625224024057f, 0.003134841797873378f, 0.0027404010761529207f, 0.0028434607665985823f, 0.0024137080181390047f, 0.002274748869240284f, 0.003644451266154647f, 0.0035296189598739147f, 0.002255472121760249f, 0.002767979633063078f, 0.0027148635126650333f, 0.0023282645270228386f, 0.0026527727022767067f, 0.0036348560824990273f, 0.0024629405234009027f, 0.0030123854521661997f, 0.0032171285711228848f, 0.002689060987904668f, 0.003914441913366318f, 0.0026596460957080126f, 0.0030338559299707413f, 0.002708103973418474f, 0.002575364662334323f, 0.0028086123056709766f, 0.0035203692968934774f, 0.0037767612375319004f, 0.002168170176446438f, 0.003473715391010046f, 0.0031630960293114185f, 0.002560105174779892f, 0.0024050285574048758f, 0.003784199245274067f, 0.0036331512965261936f, 0.0030409174505621195f, 0.0037203074898570776f, 0.00318146008066833f, 0.002712962217628956f, 0.0029802052304148674f, 0.0026757740415632725f, 0.002474773209542036f, 0.0027600759640336037f, 0.0028733238577842712f, 0.0021428836043924093f, 0.0033719115890562534f, 0.002272410551086068f, 0.0033868730533868074f, 0.003582193749025464f, 0.0032814722508192062f, 0.0025682973209768534f, 0.002831521211192012f, 0.0026407232508063316f, 0.0032976469956338406f, 0.0031575006432831287f, 0.003195561235770583f, 0.0023614424280822277f, 0.002640238031744957f, 0.0027627937961369753f, 0.0020515811629593372f, 0.004461036995053291f, 0.003955002408474684f, 0.0035093198530375957f, 0.004052140284329653f, 0.0028910061810165644f, 0.0022093947045505047f, 0.0028617398347705603f, 0.0025800711009651423f, 0.002840039087459445f, 0.0018759892554953694f, 0.002375258132815361f, 0.003981372807174921f, 0.0025985485408455133f, 0.003224159125238657f, 0.003004214959219098f, 0.002485247328877449f, 0.0019785647746175528f, 0.0029430892318487167f, 0.0022707378957420588f, 0.0026164597366005182f, 0.0028341205324977636f, 0.0029439288191497326f, 0.0019128216663375497f, 0.0033074531238526106f, 0.003896206384524703f, 0.0028064451180398464f, 0.004643708001822233f, 0.0027625260408967733f, 0.0026461503002792597f, 0.0022245990112423897f, 0.0028732498176395893f, 0.0026280968450009823f, 0.002699701813980937f, 0.0023876659106463194f, 0.004027985967695713f, 0.002618874190375209f, 0.0030044992454349995f, 0.002649323083460331f, 0.0021239330526441336f, 0.0023885646369308233f, 0.002961684251204133f, 0.0037012016400694847f, 0.0027384422719478607f, 0.0024032972287386656f, 0.0023378676269203424f, 0.002977353520691395f, 0.0032473134342581034f, 0.002479378366842866f, 0.002927084220573306f, 0.003700834698975086f, 0.0028251593466848135f, 0.0027236430905759335f, 0.0031017072033137083f, 0.0037121984641999006f, 0.002967097796499729f, 0.0023729547392576933f, 0.003123245667666197f, 0.004095803946256638f, 0.003157752100378275f, 0.002715424634516239f, 0.003119313158094883f, 0.002701732562854886f, 0.0030123088508844376f, 0.0028927435632795095f, 0.0037763677537441254f, 0.0031913656275719404f, 0.002264811657369137f, 0.002748135244473815f, 0.0026690529193729162f, 0.002825814066454768f, 0.003747971961274743f);
static const ai_layer_format_type conv2d_36_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_37_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_37_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_37_t_in_0_shape_ch_const_u16 = 144;
static const ai_u16 conv2d_37_t_weight_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_37_t_weight_0_shape_h_const_u16 = 5;
static const ai_i32 conv2d_37_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_37_l_pad_H_0_const_s32 = 1;
static const ai_u16 conv2d_37_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_37_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_37_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_37_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_37_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_37_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_37_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.004220448900014162f, 0.0037564686499536037f, 0.003047521226108074f, 0.0033893845975399017f, 0.003087033051997423f, 0.0035073875915259123f, 0.004058406222611666f, 0.003812237177044153f, 0.004262953530997038f, 0.004354759585112333f, 0.004819420166313648f, 0.0024028399493545294f, 0.005314352922141552f, 0.008123925887048244f, 0.0019430035026744008f, 0.003835822455585003f, 0.003471381962299347f, 0.005155238788574934f, 0.00380940199829638f, 0.0029937359504401684f, 0.004367303568869829f, 0.0025340034626424313f, 0.002688070759177208f, 0.004374832380563021f, 0.0048680477775633335f, 0.0033358768559992313f, 0.0024720800574868917f, 0.0037972209975123405f, 0.003681937465444207f, 0.0027344883419573307f, 0.004692662041634321f, 0.004411677364259958f, 0.005657909903675318f, 0.0047135986387729645f, 0.0031493292190134525f, 0.004672099836170673f, 0.002676971023902297f, 0.0031445741187781096f, 0.004235111642628908f, 0.0022215547505766153f, 0.0033606814686208963f, 0.0028967673424631357f, 0.003622230375185609f, 0.0030439295805990696f, 0.00458896066993475f, 0.003588642692193389f, 0.003914372995495796f, 0.0024083268363028765f, 0.0030519827269017696f, 0.006035999394953251f, 0.005793601740151644f, 0.003916161600500345f, 0.005008263513445854f, 0.004009447060525417f, 0.003093442879617214f, 0.004250721540302038f, 0.0024031049106270075f, 0.006808956619352102f, 0.006013740319758654f, 0.0023624238092452288f, 0.004057405516505241f, 0.0030906314495950937f, 0.0022115353494882584f, 0.002594372956082225f, 0.0033068875782191753f, 0.0029408202972263098f, 0.0022993299644440413f, 0.004644749686121941f, 0.0031550200656056404f, 0.0031415417324751616f, 0.0033671504352241755f, 0.00424041086807847f, 0.005130396224558353f, 0.004194826819002628f, 0.0028398323338478804f, 0.002629812341183424f, 0.002973517868667841f, 0.0029444005340337753f, 0.003872576402500272f, 0.0032789420802146196f, 0.005944001022726297f, 0.003228479065001011f, 0.003162103472277522f, 0.002837719861418009f, 0.00235551199875772f, 0.0025041860062628984f, 0.003582228673622012f, 0.003514941083267331f, 0.0035159671679139137f, 0.0051581538282334805f, 0.006080801133066416f, 0.0047875358723104f, 0.003667606506496668f, 0.0056776138953864574f, 0.003932151477783918f, 0.004410224966704845f, 0.004149339161813259f, 0.003028164617717266f, 0.004407003987580538f, 0.003915361128747463f, 0.004106246400624514f, 0.003962794318795204f, 0.00400882912799716f, 0.0056178453378379345f, 0.002707124687731266f, 0.0027609707321971655f, 0.004356618970632553f, 0.0029334037099033594f, 0.003808956127613783f, 0.004156346898525953f, 0.005383300594985485f, 0.00416930764913559f, 0.0041368380188941956f, 0.00431574834510684f, 0.002960384590551257f, 0.003380140755325556f, 0.0048889052122831345f, 0.002070480491966009f, 0.004787614103406668f, 0.0029661115258932114f, 0.007198616396635771f, 0.002896316349506378f, 0.004569553770124912f, 0.004583245608955622f, 0.0034657360520213842f, 0.003039959352463484f, 0.0033377502113580704f, 0.004513522144407034f, 0.004708878695964813f, 0.0026621653232723475f, 0.0035642499569803476f, 0.004141605459153652f, 0.004149958491325378f, 0.004369198344647884f, 0.005487866699695587f, 0.003380924230441451f, 0.004049041774123907f, 0.003762570908293128f, 0.003458352293819189f, 0.003805755637586117f, 0.0048608663491904736f, 0.0028944911900907755f, 0.004467337392270565f, 0.002949267392978072f);
static const ai_u16 conv2d_37_t_out_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_37_t_out_0_shape_h_const_u16 = 7;


static const ai_u16 conv2d_43_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_43_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_43_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_43_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_43_t_in_0_shape_ch_const_u16 = 144;
static const ai_u16 conv2d_43_t_out_0_shape_ch_const_u16 = 36;
static const ai_i8 conv2d_43_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_43_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_43_t_in_0_fmt_scale_const_f32 = 0.004430076573044062f;
static const ai_float conv2d_43_t_out_0_fmt_scale_const_f32 = 0.005913991015404463f;
static const ai_float conv2d_43_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0021370535250753164f, 0.002114568604156375f, 0.0014218242140486836f, 0.0015451330691576004f, 0.001920452923513949f, 0.0015760601963847876f, 0.0017748696263879538f, 0.0021379387471824884f, 0.0020513117779046297f, 0.0015612700954079628f, 0.0021599638275802135f, 0.0018228314584121108f, 0.001704688649624586f, 0.0014761228812858462f, 0.0020472307223826647f, 0.0017956328811123967f, 0.0017902770778164268f, 0.0018980379682034254f, 0.002021288964897394f, 0.00148404308129102f, 0.001545845647342503f, 0.0015130951069295406f, 0.001426101429387927f, 0.0019700536504387856f, 0.0015210332348942757f, 0.0024512270465493202f, 0.00225840974599123f, 0.0016784027684479952f, 0.001866079750470817f, 0.0014686002396047115f, 0.0014869384467601776f, 0.0021426507737487555f, 0.0014362696092575788f, 0.0018451177747920156f, 0.0017386946128681302f, 0.00200700294226408f);
static const ai_layer_format_type conv2d_43_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_44_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_44_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_44_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_44_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_44_t_in_0_shape_ch_const_u16 = 36;
static const ai_u16 conv2d_44_t_out_0_shape_ch_const_u16 = 144;
static const ai_i8 conv2d_44_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_44_t_out_0_fmt_zero_const_s8 = 8;
static const ai_float conv2d_44_t_in_0_fmt_scale_const_f32 = 0.005913991015404463f;
static const ai_float conv2d_44_t_out_0_fmt_scale_const_f32 = 0.0069842878729105f;
static const ai_float conv2d_44_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0011750656412914395f, 0.0018897412810474634f, 0.0022603587713092566f, 0.001956130610778928f, 0.0020253965631127357f, 0.002528312848880887f, 0.0017505076248198748f, 0.0014841868542134762f, 0.0017233792459592223f, 0.0014862489188089967f, 0.001497034914791584f, 0.0022964042145758867f, 0.0017372546717524529f, 0.001503799925558269f, 0.0017864980036392808f, 0.0014666961506009102f, 0.001784105901606381f, 0.0018300070660188794f, 0.0014914677012711763f, 0.0013191216858103871f, 0.0014031673781573772f, 0.001938401721417904f, 0.0016529387794435024f, 0.00208908854983747f, 0.001758911064825952f, 0.0019006887450814247f, 0.0019302351865917444f, 0.0020196496043354273f, 0.001845607184804976f, 0.0017652831738814712f, 0.0021292702294886112f, 0.001706264796666801f, 0.001631239429116249f, 0.002172939945012331f, 0.0016585229896008968f, 0.002260101493448019f, 0.0014991177013143897f, 0.0016079170163720846f, 0.0012638691114261746f, 0.0015720605151727796f, 0.001450935727916658f, 0.0019470888655632734f, 0.0017906393622979522f, 0.0014884296106174588f, 0.0018705519614741206f, 0.0015441387658938766f, 0.0020726961083710194f, 0.0014076452935114503f, 0.0014047471340745687f, 0.002525908872485161f, 0.0015515059931203723f, 0.001799563760869205f, 0.0016668342286720872f, 0.001573086017742753f, 0.0014190406072884798f, 0.0013562648091465235f, 0.0015378061216324568f, 0.0016962309600785375f, 0.001830260967835784f, 0.0017199283465743065f, 0.001991748809814453f, 0.0018245296087116003f, 0.002241607755422592f, 0.0022135975304991007f, 0.00186557462438941f, 0.0014247008366510272f, 0.0016431915573775768f, 0.002086642198264599f, 0.0018628935795277357f, 0.0015939152799546719f, 0.0015596435405313969f, 0.001626405748538673f, 0.0015585836954414845f, 0.0018380145775154233f, 0.0017490031896159053f, 0.0017712819389998913f, 0.001685347524471581f, 0.002050259616225958f, 0.0014996962854638696f, 0.0020066339056938887f, 0.0018381028203293681f, 0.001720696920529008f, 0.0014469189336523414f, 0.0015206066891551018f, 0.0015164914075285196f, 0.0017209077486768365f, 0.0014774083392694592f, 0.0016179493395611644f, 0.001762205851264298f, 0.001523821149021387f, 0.0017923414707183838f, 0.0017942589474841952f, 0.001631351187825203f, 0.0018262878293171525f, 0.0019138669595122337f, 0.0015374753857031465f, 0.0014496439835056663f, 0.0016758261481299996f, 0.0016847519436851144f, 0.0017270512180402875f, 0.00178579764906317f, 0.001992837991565466f, 0.0018410313641652465f, 0.0017223736504092813f, 0.0018673506565392017f, 0.0018261687364429235f, 0.0016990916337817907f, 0.0018476914847269654f, 0.0017476758221164346f, 0.0015599149046465755f, 0.0021317480131983757f, 0.001950391218997538f, 0.0013689214829355478f, 0.0015027266927063465f, 0.002161675598472357f, 0.0014955364167690277f, 0.0019861869513988495f, 0.0016236008377745748f, 0.0014395271427929401f, 0.0015970930689945817f, 0.00195658253505826f, 0.0020122916903346777f, 0.0014471460599452257f, 0.0018178652971982956f, 0.0021067108027637005f, 0.0021323086693882942f, 0.002050402108579874f, 0.0019366309279575944f, 0.0016433604760095477f, 0.00231103110127151f, 0.0019772397354245186f, 0.001479191007092595f, 0.002238524379208684f, 0.0015320796519517899f, 0.0016865066718310118f, 0.001455936231650412f, 0.0015241862274706364f, 0.001335896784439683f, 0.001874788198620081f, 0.0019725020974874496f, 0.0018096404382959008f, 0.0016769969370216131f, 0.002031017327681184f, 0.0018699540523812175f);
static const ai_layer_format_type conv2d_44_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;



static const ai_u16 conv2d_47_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_47_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_47_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_47_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_47_t_in_0_shape_ch_const_u16 = 144;
static const ai_u16 conv2d_47_t_out_0_shape_ch_const_u16 = 40;
static const ai_i8 conv2d_47_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_47_t_out_0_fmt_zero_const_s8 = -2;
static const ai_float conv2d_47_t_in_0_fmt_scale_const_f32 = 0.01602328196167946f;
static const ai_float conv2d_47_t_out_0_fmt_scale_const_f32 = 0.049935005605220795f;
static const ai_float conv2d_47_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.004655950702726841f, 0.005132641177624464f, 0.005189509131014347f, 0.005242987535893917f, 0.004754518624395132f, 0.004822084680199623f, 0.004067977424710989f, 0.00444948673248291f, 0.003901737043634057f, 0.006594392005354166f, 0.004959317855536938f, 0.003660361748188734f, 0.004299954976886511f, 0.00390983559191227f, 0.004408472683280706f, 0.005117066670209169f, 0.0050265914760529995f, 0.005990590434521437f, 0.006392425391823053f, 0.00426908116787672f, 0.003762026084586978f, 0.004738560412079096f, 0.00448769424110651f, 0.004495998844504356f, 0.004790021572262049f, 0.004332647658884525f, 0.004285166505724192f, 0.006034530233591795f, 0.005071275867521763f, 0.005070891696959734f, 0.0034705614671111107f, 0.0036748743150383234f, 0.004869077820330858f, 0.004715315997600555f, 0.004110528156161308f, 0.004072394222021103f, 0.0050071184523403645f, 0.004873720463365316f, 0.003338430542498827f, 0.003879126626998186f);
static const ai_layer_format_type conv2d_47_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_48_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_48_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_48_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_48_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_48_t_in_0_shape_ch_const_u16 = 40;
static const ai_u16 conv2d_48_t_out_0_shape_ch_const_u16 = 240;
static const ai_i8 conv2d_48_t_in_0_fmt_zero_const_s8 = -2;
static const ai_i8 conv2d_48_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_48_t_in_0_fmt_scale_const_f32 = 0.049935005605220795f;
static const ai_float conv2d_48_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_48_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.002050351118668914f, 0.0026303790509700775f, 0.0022522155195474625f, 0.0019777428824454546f, 0.002227171789854765f, 0.0025263014249503613f, 0.0023476462811231613f, 0.002681564772501588f, 0.001997648738324642f, 0.0021345375571399927f, 0.0014514921931549907f, 0.002389786299318075f, 0.002481616334989667f, 0.002649156143888831f, 0.0016979769570752978f, 0.0020539546385407448f, 0.0025944686494767666f, 0.0026143272407352924f, 0.002230265876278281f, 0.0022885778453201056f, 0.0019506635144352913f, 0.0018917163833975792f, 0.0024455497041344643f, 0.00205800449475646f, 0.0021761180832982063f, 0.002298671519383788f, 0.001896983478218317f, 0.002422601217404008f, 0.0026846386026591063f, 0.002245514653623104f, 0.0020559560507535934f, 0.00216551311314106f, 0.002225932665169239f, 0.0025198550429195166f, 0.0024751266464591026f, 0.0030192674603313208f, 0.0025152633897960186f, 0.0020245362538844347f, 0.002630441915243864f, 0.0034558172337710857f, 0.0023916519712656736f, 0.002051578601822257f, 0.0026128324680030346f, 0.0024629991967231035f, 0.002421617042273283f, 0.002280070446431637f, 0.001714405370876193f, 0.0019148390274494886f, 0.0022732780780643225f, 0.002271646400913596f, 0.0022331986110657454f, 0.002350945258513093f, 0.002114863134920597f, 0.0019855941645801067f, 0.0021944427862763405f, 0.002195232780650258f, 0.0024582671467214823f, 0.00250407587736845f, 0.0022927424870431423f, 0.002652511466294527f, 0.0015291001182049513f, 0.00286108092404902f, 0.0022109965793788433f, 0.0018123302143067122f, 0.0028223467525094748f, 0.002512837527319789f, 0.002142147393897176f, 0.0018858358962461352f, 0.0017101333942264318f, 0.001819254131987691f, 0.0024447753094136715f, 0.002011118922382593f, 0.0028142111841589212f, 0.0024549777153879404f, 0.002980954246595502f, 0.0018390658078715205f, 0.0021767225116491318f, 0.0025547961704432964f, 0.0025334747042506933f, 0.0020003572572022676f, 0.0016020856564864516f, 0.002115685259923339f, 0.002015208825469017f, 0.001875869813375175f, 0.0026280751917511225f, 0.0025381140876561403f, 0.0021479655988514423f, 0.002254982478916645f, 0.001977229956537485f, 0.0018648771801963449f, 0.003443054622039199f, 0.002311017829924822f, 0.0024968828074634075f, 0.0030411810148507357f, 0.002211078768596053f, 0.002616970334202051f, 0.002403693273663521f, 0.002269853837788105f, 0.0021830773912370205f, 0.002651357790455222f, 0.0021539803128689528f, 0.002873578341677785f, 0.0019713672809302807f, 0.0029400161001831293f, 0.0021414756774902344f, 0.002081162529066205f, 0.002298785839229822f, 0.002491801744326949f, 0.0021570869721472263f, 0.0017033221665769815f, 0.0028053231071680784f, 0.002277637366205454f, 0.0020199159625917673f, 0.0023986606393009424f, 0.001965821720659733f, 0.0020758991595357656f, 0.002234088722616434f, 0.002378493081778288f, 0.001877008588053286f, 0.0024009132757782936f, 0.0019725547172129154f, 0.0022874001879245043f, 0.0019180834060534835f, 0.001867439947091043f, 0.002473541535437107f, 0.002092743990942836f, 0.002262960420921445f, 0.002087443368509412f, 0.0018122229957953095f, 0.0021481355652213097f, 0.002421874785795808f, 0.002002933993935585f, 0.0018724140245467424f, 0.0020209692884236574f, 0.00227863690815866f, 0.0018318656366318464f, 0.0025599643122404814f, 0.002459288341924548f, 0.0025687282904982567f, 0.002467501675710082f, 0.002164241624996066f, 0.002094601048156619f, 0.0017780725611373782f, 0.0027073919773101807f, 0.0022777155973017216f, 0.0036032660864293575f, 0.0019006169168278575f, 0.0021699497010558844f, 0.0022959315683692694f, 0.0023144118022173643f, 0.0017795877065509558f, 0.0022796618286520243f, 0.0025121017824858427f, 0.0025292427744716406f, 0.0016491255955770612f, 0.0024965512566268444f, 0.0024300056975334883f, 0.002018419560045004f, 0.002001255052164197f, 0.00233727996237576f, 0.0026159603148698807f, 0.001924147829413414f, 0.0022470245603471994f, 0.0025538052432239056f, 0.0019908687099814415f, 0.0018900713184848428f, 0.0019478733884170651f, 0.0027355162892490625f, 0.002508628647774458f, 0.0017503732815384865f, 0.0022862828336656094f, 0.0017525256844237447f, 0.0027730208821594715f, 0.0022379381116479635f, 0.0017770284321159124f, 0.001439937623217702f, 0.0027815212961286306f, 0.003021665383130312f, 0.0023584922309964895f, 0.002299630781635642f, 0.0025782932061702013f, 0.0017637101700529456f, 0.002029143739491701f, 0.0021289605647325516f, 0.0021220827475190163f, 0.002137382747605443f, 0.0021544897463172674f, 0.0018312751781195402f, 0.0019424501806497574f, 0.0023152842186391354f, 0.0023657314013689756f, 0.0031057733576744795f, 0.0022881529293954372f, 0.0023055272176861763f, 0.0017488007433712482f, 0.0018848105100914836f, 0.0023806130047887564f, 0.002843559952452779f, 0.0031086646486073732f, 0.0023613139055669308f, 0.0020599376875907183f, 0.002109840279445052f, 0.002541846362873912f, 0.0024103629402816296f, 0.002047046087682247f, 0.002024470828473568f, 0.002374732866883278f, 0.0017658506985753775f, 0.0025933252181857824f, 0.003126764902845025f, 0.0015816785162314773f, 0.002283649053424597f, 0.0022455211728811264f, 0.00236025033518672f, 0.002232116414234042f, 0.0025773323141038418f, 0.0020180882420390844f, 0.0019402189645916224f, 0.001731538912281394f, 0.0020700511522591114f, 0.0024657580070197582f, 0.002894103992730379f, 0.001945827272720635f, 0.0026231182273477316f, 0.0022318281698971987f, 0.002803744515404105f, 0.0021171674598008394f, 0.002446093363687396f, 0.0015715773915871978f, 0.0026905525010079145f, 0.0020458505023270845f, 0.0024657524190843105f, 0.0025680058170109987f, 0.0021389382891356945f, 0.0019860544707626104f, 0.0018647286342456937f, 0.0025831435341387987f, 0.0020305216312408447f, 0.0025737835094332695f, 0.0026458827778697014f);
static const ai_layer_format_type conv2d_48_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_49_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_49_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_49_t_in_0_shape_ch_const_u16 = 240;
static const ai_u16 conv2d_49_t_weight_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_49_t_weight_0_shape_h_const_u16 = 5;
static const ai_i32 conv2d_49_l_pad_W_0_const_s32 = 2;
static const ai_i32 conv2d_49_l_pad_H_0_const_s32 = 2;
static const ai_u16 conv2d_49_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_49_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_49_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_49_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_49_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_49_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_49_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0025551547296345234f, 0.004395489115267992f, 0.0033256863243877888f, 0.004131973255425692f, 0.003942759707570076f, 0.004812665283679962f, 0.0047141192480921745f, 0.00305562443099916f, 0.006890174932777882f, 0.005238065961748362f, 0.007934766821563244f, 0.006958051584661007f, 0.003974711988121271f, 0.003682795912027359f, 0.0035449527204036713f, 0.003960452042520046f, 0.004658223129808903f, 0.007293234579265118f, 0.004655699711292982f, 0.005196556448936462f, 0.007059589959681034f, 0.004271505866199732f, 0.003793720854446292f, 0.003581190714612603f, 0.003021701006218791f, 0.006836284417659044f, 0.003983024507761002f, 0.00545781385153532f, 0.0026288621593266726f, 0.004369633737951517f, 0.0037574402522295713f, 0.005826103501021862f, 0.0053787678480148315f, 0.0027008014731109142f, 0.004659639205783606f, 0.005235063377767801f, 0.0031873814295977354f, 0.004285646136850119f, 0.004720556084066629f, 0.005794028285890818f, 0.0037413875106722116f, 0.004830057267099619f, 0.005195403005927801f, 0.0028823695611208677f, 0.0042418865486979485f, 0.004688501823693514f, 0.004087137058377266f, 0.004031590651720762f, 0.0036631650291383266f, 0.005222558509558439f, 0.005781994666904211f, 0.00401694281026721f, 0.006540101021528244f, 0.003524190280586481f, 0.002532163867726922f, 0.002487452933564782f, 0.0034274698700755835f, 0.0041973949410021305f, 0.005327479913830757f, 0.004112858325242996f, 0.0033083350863307714f, 0.005872895009815693f, 0.005469156429171562f, 0.008061707951128483f, 0.003605943638831377f, 0.003095800755545497f, 0.006291122175753117f, 0.00384082761593163f, 0.0040908511728048325f, 0.00406190799549222f, 0.0033190022222697735f, 0.0030149430967867374f, 0.005414713639765978f, 0.003548146691173315f, 0.002799088368192315f, 0.004511955659836531f, 0.0037864302285015583f, 0.006037266459316015f, 0.00762526597827673f, 0.003706853138282895f, 0.004877573810517788f, 0.00383155420422554f, 0.006535395979881287f, 0.00489464309066534f, 0.008135170675814152f, 0.005162225104868412f, 0.0052682687528431416f, 0.005380579736083746f, 0.003903972217813134f, 0.0033854972571134567f, 0.005510079674422741f, 0.004980460740625858f, 0.00404997356235981f, 0.004572627134621143f, 0.003856990486383438f, 0.004313786048442125f, 0.003913079388439655f, 0.003841821802780032f, 0.0037529871333390474f, 0.003574793692678213f, 0.0045431568287312984f, 0.00415393291041255f, 0.007491608615964651f, 0.00412342045456171f, 0.0034245820716023445f, 0.005272387061268091f, 0.003407754236832261f, 0.002481981413438916f, 0.0077539896592497826f, 0.00485250586643815f, 0.004995489958673716f, 0.005596076603978872f, 0.00361675419844687f, 0.004159368574619293f, 0.006170262582600117f, 0.005169954616576433f, 0.0029853242449462414f, 0.003524280618876219f, 0.0025794156827032566f, 0.004195257090032101f, 0.006252519320696592f, 0.002969710621982813f, 0.0052478923462331295f, 0.0043980577029287815f, 0.0068008736707270145f, 0.00398609833791852f, 0.003525694366544485f, 0.004027215763926506f, 0.003818974830210209f, 0.0034470618702471256f, 0.0038989929016679525f, 0.004095874726772308f, 0.004668175242841244f, 0.00439296942204237f, 0.004926246590912342f, 0.004601654130965471f, 0.00356012349948287f, 0.0044418033212423325f, 0.00483532901853323f, 0.00706580001860857f, 0.0034862039610743523f, 0.0036408326122909784f, 0.0035154421348124743f, 0.004804359748959541f, 0.003599129617214203f, 0.0037496129516512156f, 0.00465856958180666f, 0.004036919213831425f, 0.004074346274137497f, 0.005134803708642721f, 0.0059987702406942844f, 0.00583178224042058f, 0.0038097044453024864f, 0.0027522635646164417f, 0.005796902347356081f, 0.0042714220471680164f, 0.004433406516909599f, 0.004677403252571821f, 0.003594363573938608f, 0.003967289812862873f, 0.005654855631291866f, 0.0037169151473790407f, 0.0026083809789270163f, 0.0033152613323181868f, 0.004278962966054678f, 0.004961458966135979f, 0.005879102274775505f, 0.007929829880595207f, 0.004175580572336912f, 0.006982610560953617f, 0.003869475331157446f, 0.003636176697909832f, 0.0031587316188961267f, 0.0027737519703805447f, 0.005067748948931694f, 0.0042804512195289135f, 0.004300498869270086f, 0.004086477216333151f, 0.0032086388673633337f, 0.0048166182823479176f, 0.0038833003491163254f, 0.00433508912101388f, 0.0048370626755058765f, 0.003878912655636668f, 0.0056132045574486256f, 0.00485914945602417f, 0.003759603714570403f, 0.0033315157052129507f, 0.003706678282469511f, 0.004802849609404802f, 0.003992587793618441f, 0.003875279799103737f, 0.004444539081305265f, 0.006305566523224115f, 0.004479788243770599f, 0.004097030032426119f, 0.002984528196975589f, 0.002412166679278016f, 0.00494020851328969f, 0.005472820717841387f, 0.006299834698438644f, 0.004521844908595085f, 0.0039786663837730885f, 0.0036736924666911364f, 0.00727255642414093f, 0.00401262054219842f, 0.002913675969466567f, 0.004884961526840925f, 0.004213915206491947f, 0.004525559488683939f, 0.005055638030171394f, 0.0037362175062298775f, 0.003721961285918951f, 0.0037237401120364666f, 0.004810027778148651f, 0.004843590315431356f, 0.005400527734309435f, 0.0027076888363808393f, 0.004883593879640102f, 0.0026191226206719875f, 0.004398259799927473f, 0.003907183185219765f, 0.003981935791671276f, 0.0041333530098199844f, 0.005704792216420174f, 0.004071538802236319f, 0.007446323521435261f, 0.004140089266002178f, 0.0068429517559707165f, 0.006162832025438547f, 0.0036273361183702946f, 0.0037009255029261112f, 0.0028894320130348206f, 0.004526311531662941f, 0.003175373189151287f, 0.00821651704609394f, 0.005308504216372967f, 0.002692697336897254f, 0.005257757846266031f, 0.004724404308944941f);
static const ai_u16 conv2d_49_t_out_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_49_t_out_0_shape_h_const_u16 = 7;


static const ai_u16 conv2d_55_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_55_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_55_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_55_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_55_t_in_0_shape_ch_const_u16 = 240;
static const ai_u16 conv2d_55_t_out_0_shape_ch_const_u16 = 60;
static const ai_i8 conv2d_55_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_55_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_55_t_in_0_fmt_scale_const_f32 = 0.010451308451592922f;
static const ai_float conv2d_55_t_out_0_fmt_scale_const_f32 = 0.019685499370098114f;
static const ai_float conv2d_55_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.002474722219631076f, 0.002643326763063669f, 0.0017824477981776f, 0.00276712654158473f, 0.0012693569296970963f, 0.0011625108309090137f, 0.0022082841023802757f, 0.0023059530649334192f, 0.002307940972968936f, 0.001202377607114613f, 0.002985751023516059f, 0.002487575402483344f, 0.0025837954599410295f, 0.00237606861628592f, 0.0025040805339813232f, 0.002732893219217658f, 0.0011766451643779874f, 0.0021814515348523855f, 0.002239568391814828f, 0.002028572838753462f, 0.0024848980829119682f, 0.0026442157104611397f, 0.0021801732946187258f, 0.002474546665325761f, 0.002021644962951541f, 0.0023981034755706787f, 0.0025402833707630634f, 0.0024990311358124018f, 0.0025382875464856625f, 0.0021844469010829926f, 0.0011848123976960778f, 0.0024845076259225607f, 0.0011540179839357734f, 0.0011732015991583467f, 0.00265553523786366f, 0.0020405780524015427f, 0.0028030406683683395f, 0.002208869205787778f, 0.0012072876561433077f, 0.0011974489316344261f, 0.0022754203528165817f, 0.00241504586301744f, 0.0011683956254273653f, 0.0022939483169466257f, 0.0011894552735611796f, 0.0019841480534523726f, 0.0012292872415855527f, 0.001229307148605585f, 0.0020890580490231514f, 0.0019309410126879811f, 0.0023004324175417423f, 0.002393412170931697f, 0.002982908161357045f, 0.0020733943674713373f, 0.0011577601544559002f, 0.002330851973965764f, 0.0020112963393330574f, 0.0012221105862408876f, 0.0020853178575634956f, 0.0025254206266254187f);
static const ai_layer_format_type conv2d_55_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_56_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_56_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_56_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_56_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_56_t_in_0_shape_ch_const_u16 = 60;
static const ai_u16 conv2d_56_t_out_0_shape_ch_const_u16 = 240;
static const ai_i8 conv2d_56_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_56_t_out_0_fmt_zero_const_s8 = 30;
static const ai_float conv2d_56_t_in_0_fmt_scale_const_f32 = 0.019685499370098114f;
static const ai_float conv2d_56_t_out_0_fmt_scale_const_f32 = 0.050723131746053696f;
static const ai_float conv2d_56_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.001656211563386023f, 0.0020727927330881357f, 0.002179404255002737f, 0.002130280714482069f, 0.002212569350376725f, 0.001776943332515657f, 0.0024975771084427834f, 0.002096315613016486f, 0.0019317228579893708f, 0.0017145921010524035f, 0.00225452589802444f, 0.0017293182900175452f, 0.002448362996801734f, 0.003750166157260537f, 0.0018082866445183754f, 0.0023253061808645725f, 0.0025968856643885374f, 0.0018370801117271185f, 0.002271870383992791f, 0.002086910419166088f, 0.0023087800946086645f, 0.002200187649577856f, 0.0023830831050872803f, 0.0028479371685534716f, 0.0030442853458225727f, 0.002089575631543994f, 0.0018325404962524772f, 0.0021702919621020555f, 0.0019453808199614286f, 0.0019291987409815192f, 0.0021951415110379457f, 0.001577231683768332f, 0.0026071693282574415f, 0.0020783201325684786f, 0.0028784109745174646f, 0.00237292074598372f, 0.0018658324843272567f, 0.0014051195466890931f, 0.0015878601698204875f, 0.0026186590548604727f, 0.0018799244426190853f, 0.001947959535755217f, 0.0017884669359773397f, 0.0021267859265208244f, 0.002906042616814375f, 0.0015486071351915598f, 0.0020995079539716244f, 0.0016491462010890245f, 0.0011306116357445717f, 0.003610566956922412f, 0.002356552518904209f, 0.002796425251290202f, 0.0021200308110564947f, 0.002411952242255211f, 0.002100034151226282f, 0.0018167642410844564f, 0.0022927899844944477f, 0.0021464736200869083f, 0.002083459170535207f, 0.002253135899081826f, 0.0019230649340897799f, 0.0024193020071834326f, 0.0026261452585458755f, 0.0019123776583001018f, 0.002153827575966716f, 0.0018450923962518573f, 0.001718192477710545f, 0.002149271545931697f, 0.0014270071405917406f, 0.0022074771113693714f, 0.0018446325557306409f, 0.0021644532680511475f, 0.002267028670758009f, 0.001999157713726163f, 0.002310882555320859f, 0.001810797257348895f, 0.001676145358942449f, 0.0023918026126921177f, 0.0031737126410007477f, 0.0022732215002179146f, 0.002063342137262225f, 0.0020172044169157743f, 0.0017022035317495465f, 0.0020249858498573303f, 0.002218484180048108f, 0.001903533237054944f, 0.003828601446002722f, 0.0015372573398053646f, 0.002164494013413787f, 0.0015881775179877877f, 0.0022779935970902443f, 0.0032832312863320112f, 0.00183751224540174f, 0.00238539045676589f, 0.0015991046093404293f, 0.0018913947278633714f, 0.0016545671969652176f, 0.002558755222707987f, 0.002033676952123642f, 0.002641150029376149f, 0.0019110796274617314f, 0.0019369206856936216f, 0.001845399965532124f, 0.002143777906894684f, 0.002494651358574629f, 0.0021053326781839132f, 0.0023700986057519913f, 0.0019391535315662622f, 0.002352145966142416f, 0.0017027142457664013f, 0.0022504101507365704f, 0.0018061723094433546f, 0.0017425828846171498f, 0.0025853440165519714f, 0.002644821535795927f, 0.0023391572758555412f, 0.0020345773082226515f, 0.0020040024537593126f, 0.0018936386331915855f, 0.0021978470031172037f, 0.0020794193260371685f, 0.0032135609071701765f, 0.0018862613942474127f, 0.002212396590039134f, 0.001535459770821035f, 0.0022749565541744232f, 0.0024338283110409975f, 0.0016371349338442087f, 0.0020647565834224224f, 0.001879826420918107f, 0.0023446318227797747f, 0.002018155762925744f, 0.0014160365099087358f, 0.0019066495588049293f, 0.0018426468595862389f, 0.0019990517757833004f, 0.0017159510171040893f, 0.002044517546892166f, 0.0014890383463352919f, 0.002024866407737136f, 0.002228378551080823f, 0.002075514756143093f, 0.0014578107511624694f, 0.0027006343007087708f, 0.0023294209968298674f, 0.00206923158839345f, 0.0024164370261132717f, 0.0032747574150562286f, 0.0018074176041409373f, 0.0019483756041154265f, 0.0020894655026495457f, 0.0019603220280259848f, 0.0016413398552685976f, 0.0019517202163115144f, 0.0021166217047721148f, 0.0026789181865751743f, 0.0020979689434170723f, 0.002183435019105673f, 0.0021944597829133272f, 0.001486029359512031f, 0.0023965099826455116f, 0.0020873246248811483f, 0.001952029881067574f, 0.0023043451365083456f, 0.002345199463889003f, 0.0027111771050840616f, 0.0019853494595736265f, 0.0018007255857810378f, 0.0023105680011212826f, 0.002302552806213498f, 0.0022735027596354485f, 0.0016200679820030928f, 0.001640096539631486f, 0.0031548941042274237f, 0.002674963790923357f, 0.002581496024504304f, 0.002128196880221367f, 0.001878424547612667f, 0.001693234546110034f, 0.0026457118801772594f, 0.0020420015789568424f, 0.001516480464488268f, 0.0021992987021803856f, 0.0016612500185146928f, 0.0020437827333807945f, 0.0022076910827308893f, 0.0015932210953906178f, 0.0017547293100506067f, 0.002177056623622775f, 0.0018651178106665611f, 0.002118472009897232f, 0.0023354056756943464f, 0.0019875832367688417f, 0.0024953060783445835f, 0.002199797425419092f, 0.0020652434322983027f, 0.0017535608494654298f, 0.0016083933878690004f, 0.0015186105156317353f, 0.0026033101603388786f, 0.00219844002276659f, 0.0019523324444890022f, 0.001983011607080698f, 0.0023320650216192007f, 0.002543499693274498f, 0.0021643461659550667f, 0.0019139382056891918f, 0.0028608120046555996f, 0.0019307073671370745f, 0.0017555796075612307f, 0.0021111872047185898f, 0.0015863135922700167f, 0.002478651935234666f, 0.0025059818290174007f, 0.001842469908297062f, 0.002185575431212783f, 0.0020660182926803827f, 0.001962117152288556f, 0.0013562794774770737f, 0.0016970245633274317f, 0.0018695417093113065f, 0.0022454620338976383f, 0.001960342051461339f, 0.0018971872050315142f, 0.0016359917353838682f, 0.0031205464620143175f, 0.0019977169577032328f, 0.0019202812109142542f, 0.0019375062547624111f, 0.0015707446727901697f, 0.0020378187764436007f, 0.0016915512969717383f, 0.0020867870189249516f, 0.0023737067822366953f, 0.0018745820270851254f, 0.0021927489433437586f, 0.0019022711785510182f, 0.001995760016143322f, 0.0018045547185465693f, 0.0025602702517062426f);
static const ai_layer_format_type conv2d_56_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;



static const ai_u16 conv2d_59_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_59_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_59_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_59_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_59_t_in_0_shape_ch_const_u16 = 240;
static const ai_u16 conv2d_59_t_out_0_shape_ch_const_u16 = 40;
static const ai_i8 conv2d_59_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_59_t_out_0_fmt_zero_const_s8 = 1;
static const ai_float conv2d_59_t_in_0_fmt_scale_const_f32 = 0.020274244248867035f;
static const ai_float conv2d_59_t_out_0_fmt_scale_const_f32 = 0.037382952868938446f;
static const ai_float conv2d_59_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.003181791864335537f, 0.0035843460354954004f, 0.0036848667077720165f, 0.0027073787059634924f, 0.0029078363440930843f, 0.0035880987998098135f, 0.003723755944520235f, 0.0028298080433160067f, 0.003348048310726881f, 0.0034413423854857683f, 0.0034148991107940674f, 0.003730577416718006f, 0.0034787103068083525f, 0.003616193076595664f, 0.0034448576625436544f, 0.0035675412509590387f, 0.003558002645149827f, 0.0030549250077456236f, 0.003387193428352475f, 0.0031916152220219374f, 0.003628612495958805f, 0.0033843605779111385f, 0.003292173845693469f, 0.003663991577923298f, 0.0034740299452096224f, 0.002568610478192568f, 0.003921912517398596f, 0.003528101136907935f, 0.0029245775658637285f, 0.004350103437900543f, 0.0036207796074450016f, 0.0034974291920661926f, 0.004869521129876375f, 0.003624255070462823f, 0.003025829792022705f, 0.0033700685016810894f, 0.0041172984056174755f, 0.0029825407546013594f, 0.0033536453265696764f, 0.0029330498073250055f);
static const ai_layer_format_type conv2d_59_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_60_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_60_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_60_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_60_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_60_t_in_0_shape_ch_const_u16 = 40;
static const ai_u16 conv2d_60_t_out_0_shape_ch_const_u16 = 64;
static const ai_i8 conv2d_60_t_in_0_fmt_zero_const_s8 = 1;
static const ai_i8 conv2d_60_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_60_t_in_0_fmt_scale_const_f32 = 0.037382952868938446f;
static const ai_float conv2d_60_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_60_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0022271848283708096f, 0.003275471506640315f, 0.0020160889253020287f, 0.0026217454578727484f, 0.002791859908029437f, 0.002686417894437909f, 0.002355405827984214f, 0.002896380377933383f, 0.0029992612544447184f, 0.002648993395268917f, 0.0024458523839712143f, 0.0022106857504695654f, 0.0019738900009542704f, 0.002851740922778845f, 0.002430947497487068f, 0.0021767960861325264f, 0.0028817711863666773f, 0.0022201170213520527f, 0.0026878872886300087f, 0.002082402817904949f, 0.0025741413701325655f, 0.0021551158279180527f, 0.0022460573818534613f, 0.002513925079256296f, 0.002596563659608364f, 0.0020635451655834913f, 0.0025711138732731342f, 0.0028231756296008825f, 0.002743175020441413f, 0.002849781420081854f, 0.003055806504562497f, 0.0029418186750262976f, 0.0028295305091887712f, 0.002053520642220974f, 0.0028291780035942793f, 0.0026636123657226562f, 0.0019359546713531017f, 0.003296191804111004f, 0.0021069475915282965f, 0.002050454029813409f, 0.0026361122727394104f, 0.0022694310173392296f, 0.0024152740370482206f, 0.002245528856292367f, 0.0031023758929222822f, 0.0025013932026922703f, 0.002174634952098131f, 0.002437812276184559f, 0.0032203260343521833f, 0.002267057541757822f, 0.0036288786213845015f, 0.002628748305141926f, 0.003337561385706067f, 0.002469191327691078f, 0.0020481483079493046f, 0.0028287458699196577f, 0.0020698048174381256f, 0.0023595031816512346f, 0.003179766470566392f, 0.002147429622709751f, 0.0019661530386656523f, 0.0026035388000309467f, 0.0018672208534553647f, 0.0024818943347781897f);
static const ai_layer_format_type conv2d_60_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;



static const ai_u32 nl_63_t_in_0_shape_ch_prod_const_u32 = 10;
STAI_API_ENTRY
stai_return_code stai_network_run(
  stai_network* network,
  const stai_run_mode mode)
{
   STAI_UNUSED(mode)
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_ACTIVATIONS) != STAI_FLAG_ACTIVATIONS,
        STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_PTR, net_ctx->_return_code)

  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_INPUTS) != STAI_FLAG_INPUTS,
                  STAI_ERROR_NETWORK_INVALID_IN_PTR, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_OUTPUTS) != STAI_FLAG_OUTPUTS,
                  STAI_ERROR_NETWORK_INVALID_OUT_PTR, net_ctx->_return_code)

  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_WEIGHTS) != STAI_FLAG_WEIGHTS,
                  STAI_ERROR_NETWORK_INVALID_WEIGHTS_PTR, net_ctx->_return_code)


  /* LITE_KERNEL_SECTION BEGIN conv2d_0 */
  {
      const ai_i8* conv2d_0_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_inputs[0] + 0);
    const ai_i8* conv2d_0_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 0);
    const ai_i32* conv2d_0_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[1] + 0);
    ai_i8* conv2d_0_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 67648);
    ai_i16* conv2d_0_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 67100);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(0, 1, {(stai_ptr) conv2d_0_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_sssa8_ch(conv2d_0_t_in_0_ptr_const_s8, conv2d_0_t_in_0_shape_w_const_u16, conv2d_0_t_in_0_shape_h_const_u16, conv2d_0_t_in_0_shape_ch_const_u16, conv2d_0_t_weight_0_ptr_const_s8, conv2d_0_t_out_0_shape_ch_const_u16, conv2d_0_t_weight_0_shape_w_const_u16, conv2d_0_t_weight_0_shape_h_const_u16, conv2d_0_l_stride_1_const_u16, conv2d_0_l_stride_0_const_u16, conv2d_0_l_pad_W_0_const_s32, conv2d_0_l_pad_H_0_const_s32, conv2d_0_t_weight_1_ptr_const_s32, conv2d_0_t_in_0_fmt_zero_const_s8, conv2d_0_t_out_0_fmt_zero_const_s8, conv2d_0_t_in_0_fmt_scale_const_f32, conv2d_0_t_out_0_fmt_scale_const_f32, conv2d_0_t_weight_0_fmt_scale_const_f32, conv2d_0_l_out_ch_format_const_layer_format_type, conv2d_0_t_out_0_ptr_s8, conv2d_0_t_out_0_shape_w_const_u16, conv2d_0_t_out_0_shape_h_const_u16, 1, 548, conv2d_0_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(0, 1, {(stai_ptr) conv2d_0_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_0 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_1_pad_before */
  {
      const ai_ptr conv2d_1_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 67648);
    ai_ptr conv2d_1_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 65792);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(1, 1, {(stai_ptr) conv2d_1_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_1_pad_before_t_in_0_ptr_const_ptr, conv2d_1_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_1_pad_before_v_pad_constant_value_const_s8), conv2d_1_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_1_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(448), (ai_i32)(480), (ai_i32)(480), (ai_i32)(16), (ai_i32)(16));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(1, 1, {(stai_ptr) conv2d_1_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_1_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_1 */
  {
      const ai_i8* conv2d_1_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 65792);
    const ai_i8* conv2d_1_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[2] + 0);
    const ai_i32* conv2d_1_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[3] + 0);
    ai_i8* conv2d_1_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 65344);
    ai_i16* conv2d_1_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 80192);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(1, 1, {(stai_ptr) conv2d_1_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_1_t_in_0_ptr_const_s8, conv2d_1_t_in_0_shape_w_const_u16, conv2d_1_t_in_0_shape_h_const_u16, conv2d_1_t_in_0_shape_ch_const_u16, conv2d_1_t_weight_0_ptr_const_s8, conv2d_1_l_stride_1_const_u16, conv2d_1_l_stride_0_const_u16, conv2d_1_t_weight_1_ptr_const_s32, conv2d_1_t_in_0_fmt_zero_const_s8, conv2d_1_t_out_0_fmt_zero_const_s8, conv2d_1_t_in_0_fmt_scale_const_f32, conv2d_1_t_out_0_fmt_scale_const_f32, conv2d_1_t_weight_0_fmt_scale_const_f32, conv2d_1_t_out_0_ptr_s8, conv2d_1_t_out_0_shape_w_const_u16, conv2d_1_t_out_0_shape_h_const_u16, 0, 593, conv2d_1_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(1, 1, {(stai_ptr) conv2d_1_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_1 */
  /* LITE_KERNEL_SECTION BEGIN pool_2 */
  {
    
  forward_lite_pool_2(net_ctx);
  }
  /* LITE_KERNEL_SECTION END pool_2 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_7 */
  {
      const ai_i8* conv2d_7_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 77888);
    const ai_i8* conv2d_7_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[4] + 0);
    const ai_i32* conv2d_7_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[5] + 0);
    ai_i8* conv2d_7_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 80784);
    ai_i16* conv2d_7_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 80680);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(7, 1, {(stai_ptr) conv2d_7_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_7_t_in_0_ptr_const_s8, conv2d_7_t_in_0_shape_w_const_u16, conv2d_7_t_in_0_shape_h_const_u16, conv2d_7_l_stride_1_const_u16, conv2d_7_l_stride_0_const_u16, conv2d_7_t_in_0_shape_ch_const_u16, conv2d_7_t_weight_0_ptr_const_s8, conv2d_7_t_out_0_shape_ch_const_u16, conv2d_7_t_weight_1_ptr_const_s32, conv2d_7_t_in_0_fmt_zero_const_s8, conv2d_7_t_out_0_fmt_zero_const_s8, conv2d_7_t_in_0_fmt_scale_const_f32, conv2d_7_t_out_0_fmt_scale_const_f32, conv2d_7_t_weight_0_fmt_scale_const_f32, conv2d_7_l_out_ch_format_const_layer_format_type, conv2d_7_t_out_0_ptr_s8, 1, 104, conv2d_7_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(7, 1, {(stai_ptr) conv2d_7_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_7 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_8 */
  {
      const ai_i8* conv2d_8_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 80784);
    const ai_i8* conv2d_8_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[6] + 0);
    const ai_i32* conv2d_8_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[7] + 0);
    ai_i8* conv2d_8_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 80768);
    ai_i16* conv2d_8_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 77888);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(8, 1, {(stai_ptr) conv2d_8_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_8_t_in_0_ptr_const_s8, conv2d_8_t_in_0_shape_w_const_u16, conv2d_8_t_in_0_shape_h_const_u16, conv2d_8_l_stride_1_const_u16, conv2d_8_l_stride_0_const_u16, conv2d_8_t_in_0_shape_ch_const_u16, conv2d_8_t_weight_0_ptr_const_s8, conv2d_8_t_out_0_shape_ch_const_u16, conv2d_8_t_weight_1_ptr_const_s32, conv2d_8_t_in_0_fmt_zero_const_s8, conv2d_8_t_out_0_fmt_zero_const_s8, conv2d_8_t_in_0_fmt_scale_const_f32, conv2d_8_t_out_0_fmt_scale_const_f32, conv2d_8_t_weight_0_fmt_scale_const_f32, conv2d_8_l_out_ch_format_const_layer_format_type, conv2d_8_t_out_0_ptr_s8, 1, 176, conv2d_8_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(8, 1, {(stai_ptr) conv2d_8_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_8 */
  /* LITE_KERNEL_SECTION BEGIN nl_9 */
  {
    
  forward_lite_nl_9(net_ctx);
  }
  /* LITE_KERNEL_SECTION END nl_9 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_10 */
  {
    
  forward_lite_eltwise_10(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_10 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_11 */
  {
      const ai_i8* conv2d_11_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 65344);
    const ai_i8* conv2d_11_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[8] + 0);
    const ai_i32* conv2d_11_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[9] + 0);
    ai_i8* conv2d_11_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 77888);
    ai_i16* conv2d_11_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 65120);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(11, 1, {(stai_ptr) conv2d_11_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_11_t_in_0_ptr_const_s8, conv2d_11_t_in_0_shape_w_const_u16, conv2d_11_t_in_0_shape_h_const_u16, conv2d_11_l_stride_1_const_u16, conv2d_11_l_stride_0_const_u16, conv2d_11_t_in_0_shape_ch_const_u16, conv2d_11_t_weight_0_ptr_const_s8, conv2d_11_t_out_0_shape_ch_const_u16, conv2d_11_t_weight_1_ptr_const_s32, conv2d_11_t_in_0_fmt_zero_const_s8, conv2d_11_t_out_0_fmt_zero_const_s8, conv2d_11_t_in_0_fmt_scale_const_f32, conv2d_11_t_out_0_fmt_scale_const_f32, conv2d_11_t_weight_0_fmt_scale_const_f32, conv2d_11_l_out_ch_format_const_layer_format_type, conv2d_11_t_out_0_ptr_s8, 1, 224, conv2d_11_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(11, 1, {(stai_ptr) conv2d_11_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_11 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_12 */
  {
      const ai_i8* conv2d_12_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 77888);
    const ai_i8* conv2d_12_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[10] + 0);
    const ai_i32* conv2d_12_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[11] + 0);
    ai_i8* conv2d_12_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 12480);
    ai_i16* conv2d_12_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 90432);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(12, 1, {(stai_ptr) conv2d_12_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_12_t_in_0_ptr_const_s8, conv2d_12_t_in_0_shape_w_const_u16, conv2d_12_t_in_0_shape_h_const_u16, conv2d_12_l_stride_1_const_u16, conv2d_12_l_stride_0_const_u16, conv2d_12_t_in_0_shape_ch_const_u16, conv2d_12_t_weight_0_ptr_const_s8, conv2d_12_t_out_0_shape_ch_const_u16, conv2d_12_t_weight_1_ptr_const_s32, conv2d_12_t_in_0_fmt_zero_const_s8, conv2d_12_t_out_0_fmt_zero_const_s8, conv2d_12_t_in_0_fmt_scale_const_f32, conv2d_12_t_out_0_fmt_scale_const_f32, conv2d_12_t_weight_0_fmt_scale_const_f32, conv2d_12_l_out_ch_format_const_layer_format_type, conv2d_12_t_out_0_ptr_s8, 1, 1024, conv2d_12_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(12, 1, {(stai_ptr) conv2d_12_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_12 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_13_pad_before */
  {
      const ai_ptr conv2d_13_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 12480);
    ai_ptr conv2d_13_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 1344);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(13, 1, {(stai_ptr) conv2d_13_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_13_pad_before_t_in_0_ptr_const_ptr, conv2d_13_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_13_pad_before_v_pad_constant_value_const_s8), conv2d_13_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_13_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(2688), (ai_i32)(0), (ai_i32)(5760), (ai_i32)(0), (ai_i32)(192));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(13, 1, {(stai_ptr) conv2d_13_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_13_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_13 */
  {
      const ai_i8* conv2d_13_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1344);
    const ai_i8* conv2d_13_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[12] + 0);
    const ai_i32* conv2d_13_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[13] + 0);
    ai_i8* conv2d_13_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_13_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 87900);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(13, 1, {(stai_ptr) conv2d_13_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_13_t_in_0_ptr_const_s8, conv2d_13_t_in_0_shape_w_const_u16, conv2d_13_t_in_0_shape_h_const_u16, conv2d_13_t_in_0_shape_ch_const_u16, conv2d_13_t_weight_0_ptr_const_s8, conv2d_13_l_stride_1_const_u16, conv2d_13_l_stride_0_const_u16, conv2d_13_t_weight_1_ptr_const_s32, conv2d_13_t_in_0_fmt_zero_const_s8, conv2d_13_t_out_0_fmt_zero_const_s8, conv2d_13_t_in_0_fmt_scale_const_f32, conv2d_13_t_out_0_fmt_scale_const_f32, conv2d_13_t_weight_0_fmt_scale_const_f32, conv2d_13_t_out_0_ptr_s8, conv2d_13_t_out_0_shape_w_const_u16, conv2d_13_t_out_0_shape_h_const_u16, 0, 3553, conv2d_13_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(13, 1, {(stai_ptr) conv2d_13_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_13 */
  /* LITE_KERNEL_SECTION BEGIN pool_14 */
  {
    
  forward_lite_pool_14(net_ctx);
  }
  /* LITE_KERNEL_SECTION END pool_14 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_19 */
  {
      const ai_i8* conv2d_19_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 18816);
    const ai_i8* conv2d_19_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[14] + 0);
    const ai_i32* conv2d_19_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[15] + 0);
    ai_i8* conv2d_19_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 19536);
    ai_i16* conv2d_19_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 18912);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(19, 1, {(stai_ptr) conv2d_19_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_19_t_in_0_ptr_const_s8, conv2d_19_t_in_0_shape_w_const_u16, conv2d_19_t_in_0_shape_h_const_u16, conv2d_19_l_stride_1_const_u16, conv2d_19_l_stride_0_const_u16, conv2d_19_t_in_0_shape_ch_const_u16, conv2d_19_t_weight_0_ptr_const_s8, conv2d_19_t_out_0_shape_ch_const_u16, conv2d_19_t_weight_1_ptr_const_s32, conv2d_19_t_in_0_fmt_zero_const_s8, conv2d_19_t_out_0_fmt_zero_const_s8, conv2d_19_t_in_0_fmt_scale_const_f32, conv2d_19_t_out_0_fmt_scale_const_f32, conv2d_19_t_weight_0_fmt_scale_const_f32, conv2d_19_l_out_ch_format_const_layer_format_type, conv2d_19_t_out_0_ptr_s8, 1, 624, conv2d_19_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(19, 1, {(stai_ptr) conv2d_19_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_19 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_20 */
  {
      const ai_i8* conv2d_20_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 19536);
    const ai_i8* conv2d_20_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[16] + 0);
    const ai_i32* conv2d_20_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[17] + 0);
    ai_i8* conv2d_20_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 18816);
    ai_i16* conv2d_20_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 19560);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(20, 1, {(stai_ptr) conv2d_20_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_20_t_in_0_ptr_const_s8, conv2d_20_t_in_0_shape_w_const_u16, conv2d_20_t_in_0_shape_h_const_u16, conv2d_20_l_stride_1_const_u16, conv2d_20_l_stride_0_const_u16, conv2d_20_t_in_0_shape_ch_const_u16, conv2d_20_t_weight_0_ptr_const_s8, conv2d_20_t_out_0_shape_ch_const_u16, conv2d_20_t_weight_1_ptr_const_s32, conv2d_20_t_in_0_fmt_zero_const_s8, conv2d_20_t_out_0_fmt_zero_const_s8, conv2d_20_t_in_0_fmt_scale_const_f32, conv2d_20_t_out_0_fmt_scale_const_f32, conv2d_20_t_weight_0_fmt_scale_const_f32, conv2d_20_l_out_ch_format_const_layer_format_type, conv2d_20_t_out_0_ptr_s8, 1, 1056, conv2d_20_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(20, 1, {(stai_ptr) conv2d_20_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_20 */
  /* LITE_KERNEL_SECTION BEGIN nl_21 */
  {
    
  forward_lite_nl_21(net_ctx);
  }
  /* LITE_KERNEL_SECTION END nl_21 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_22 */
  {
    
  forward_lite_eltwise_22(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_22 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_23 */
  {
      const ai_i8* conv2d_23_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 19008);
    const ai_i8* conv2d_23_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[18] + 0);
    const ai_i32* conv2d_23_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[19] + 0);
    ai_i8* conv2d_23_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 624);
    ai_i16* conv2d_23_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(23, 1, {(stai_ptr) conv2d_23_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_23_t_in_0_ptr_const_s8, conv2d_23_t_in_0_shape_w_const_u16, conv2d_23_t_in_0_shape_h_const_u16, conv2d_23_l_stride_1_const_u16, conv2d_23_l_stride_0_const_u16, conv2d_23_t_in_0_shape_ch_const_u16, conv2d_23_t_weight_0_ptr_const_s8, conv2d_23_t_out_0_shape_ch_const_u16, conv2d_23_t_weight_1_ptr_const_s32, conv2d_23_t_in_0_fmt_zero_const_s8, conv2d_23_t_out_0_fmt_zero_const_s8, conv2d_23_t_in_0_fmt_scale_const_f32, conv2d_23_t_out_0_fmt_scale_const_f32, conv2d_23_t_weight_0_fmt_scale_const_f32, conv2d_23_l_out_ch_format_const_layer_format_type, conv2d_23_t_out_0_ptr_s8, 1, 624, conv2d_23_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(23, 1, {(stai_ptr) conv2d_23_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_23 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_24 */
  {
      const ai_i8* conv2d_24_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 624);
    const ai_i8* conv2d_24_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[20] + 0);
    const ai_i32* conv2d_24_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[21] + 0);
    ai_i8* conv2d_24_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 6864);
    ai_i16* conv2d_24_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 5328);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(24, 1, {(stai_ptr) conv2d_24_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_24_t_in_0_ptr_const_s8, conv2d_24_t_in_0_shape_w_const_u16, conv2d_24_t_in_0_shape_h_const_u16, conv2d_24_l_stride_1_const_u16, conv2d_24_l_stride_0_const_u16, conv2d_24_t_in_0_shape_ch_const_u16, conv2d_24_t_weight_0_ptr_const_s8, conv2d_24_t_out_0_shape_ch_const_u16, conv2d_24_t_weight_1_ptr_const_s32, conv2d_24_t_in_0_fmt_zero_const_s8, conv2d_24_t_out_0_fmt_zero_const_s8, conv2d_24_t_in_0_fmt_scale_const_f32, conv2d_24_t_out_0_fmt_scale_const_f32, conv2d_24_t_weight_0_fmt_scale_const_f32, conv2d_24_l_out_ch_format_const_layer_format_type, conv2d_24_t_out_0_ptr_s8, 1, 1536, conv2d_24_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(24, 1, {(stai_ptr) conv2d_24_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_24 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_25_pad_before */
  {
      const ai_ptr conv2d_25_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 6864);
    ai_ptr conv2d_25_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 35088);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(25, 1, {(stai_ptr) conv2d_25_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_25_pad_before_t_in_0_ptr_const_ptr, conv2d_25_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_25_pad_before_v_pad_constant_value_const_s8), conv2d_25_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_25_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(2016), (ai_i32)(2304), (ai_i32)(2304), (ai_i32)(144), (ai_i32)(144));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(25, 1, {(stai_ptr) conv2d_25_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_25_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_25 */
  {
      const ai_i8* conv2d_25_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 35088);
    const ai_i8* conv2d_25_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[22] + 0);
    const ai_i32* conv2d_25_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[23] + 0);
    ai_i8* conv2d_25_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 5332);
    ai_i16* conv2d_25_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(25, 1, {(stai_ptr) conv2d_25_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_25_t_in_0_ptr_const_s8, conv2d_25_t_in_0_shape_w_const_u16, conv2d_25_t_in_0_shape_h_const_u16, conv2d_25_t_in_0_shape_ch_const_u16, conv2d_25_t_weight_0_ptr_const_s8, conv2d_25_l_stride_1_const_u16, conv2d_25_l_stride_0_const_u16, conv2d_25_t_weight_1_ptr_const_s32, conv2d_25_t_in_0_fmt_zero_const_s8, conv2d_25_t_out_0_fmt_zero_const_s8, conv2d_25_t_in_0_fmt_scale_const_f32, conv2d_25_t_out_0_fmt_scale_const_f32, conv2d_25_t_weight_0_fmt_scale_const_f32, conv2d_25_t_out_0_ptr_s8, conv2d_25_t_out_0_shape_w_const_u16, conv2d_25_t_out_0_shape_h_const_u16, 0, 5329, conv2d_25_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(25, 1, {(stai_ptr) conv2d_25_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_25 */
  /* LITE_KERNEL_SECTION BEGIN pool_26 */
  {
    
  forward_lite_pool_26(net_ctx);
  }
  /* LITE_KERNEL_SECTION END pool_26 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_31 */
  {
      const ai_i8* conv2d_31_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_31_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[24] + 0);
    const ai_i32* conv2d_31_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[25] + 0);
    ai_i8* conv2d_31_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1080);
    ai_i16* conv2d_31_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 144);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(31, 1, {(stai_ptr) conv2d_31_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_31_t_in_0_ptr_const_s8, conv2d_31_t_in_0_shape_w_const_u16, conv2d_31_t_in_0_shape_h_const_u16, conv2d_31_l_stride_1_const_u16, conv2d_31_l_stride_0_const_u16, conv2d_31_t_in_0_shape_ch_const_u16, conv2d_31_t_weight_0_ptr_const_s8, conv2d_31_t_out_0_shape_ch_const_u16, conv2d_31_t_weight_1_ptr_const_s32, conv2d_31_t_in_0_fmt_zero_const_s8, conv2d_31_t_out_0_fmt_zero_const_s8, conv2d_31_t_in_0_fmt_scale_const_f32, conv2d_31_t_out_0_fmt_scale_const_f32, conv2d_31_t_weight_0_fmt_scale_const_f32, conv2d_31_l_out_ch_format_const_layer_format_type, conv2d_31_t_out_0_ptr_s8, 1, 936, conv2d_31_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(31, 1, {(stai_ptr) conv2d_31_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_31 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_32 */
  {
      const ai_i8* conv2d_32_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1080);
    const ai_i8* conv2d_32_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[26] + 0);
    const ai_i32* conv2d_32_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[27] + 0);
    ai_i8* conv2d_32_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_32_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 1116);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(32, 1, {(stai_ptr) conv2d_32_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_32_t_in_0_ptr_const_s8, conv2d_32_t_in_0_shape_w_const_u16, conv2d_32_t_in_0_shape_h_const_u16, conv2d_32_l_stride_1_const_u16, conv2d_32_l_stride_0_const_u16, conv2d_32_t_in_0_shape_ch_const_u16, conv2d_32_t_weight_0_ptr_const_s8, conv2d_32_t_out_0_shape_ch_const_u16, conv2d_32_t_weight_1_ptr_const_s32, conv2d_32_t_in_0_fmt_zero_const_s8, conv2d_32_t_out_0_fmt_zero_const_s8, conv2d_32_t_in_0_fmt_scale_const_f32, conv2d_32_t_out_0_fmt_scale_const_f32, conv2d_32_t_weight_0_fmt_scale_const_f32, conv2d_32_l_out_ch_format_const_layer_format_type, conv2d_32_t_out_0_ptr_s8, 1, 1584, conv2d_32_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(32, 1, {(stai_ptr) conv2d_32_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_32 */
  /* LITE_KERNEL_SECTION BEGIN nl_33 */
  {
    
  forward_lite_nl_33(net_ctx);
  }
  /* LITE_KERNEL_SECTION END nl_33 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_34 */
  {
    
  forward_lite_eltwise_34(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_34 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_35 */
  {
      const ai_i8* conv2d_35_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 33556);
    const ai_i8* conv2d_35_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[28] + 0);
    const ai_i32* conv2d_35_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[29] + 0);
    ai_i8* conv2d_35_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 816);
    ai_i16* conv2d_35_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(35, 1, {(stai_ptr) conv2d_35_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_35_t_in_0_ptr_const_s8, conv2d_35_t_in_0_shape_w_const_u16, conv2d_35_t_in_0_shape_h_const_u16, conv2d_35_l_stride_1_const_u16, conv2d_35_l_stride_0_const_u16, conv2d_35_t_in_0_shape_ch_const_u16, conv2d_35_t_weight_0_ptr_const_s8, conv2d_35_t_out_0_shape_ch_const_u16, conv2d_35_t_weight_1_ptr_const_s32, conv2d_35_t_in_0_fmt_zero_const_s8, conv2d_35_t_out_0_fmt_zero_const_s8, conv2d_35_t_in_0_fmt_scale_const_f32, conv2d_35_t_out_0_fmt_scale_const_f32, conv2d_35_t_weight_0_fmt_scale_const_f32, conv2d_35_l_out_ch_format_const_layer_format_type, conv2d_35_t_out_0_ptr_s8, 1, 816, conv2d_35_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(35, 1, {(stai_ptr) conv2d_35_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_35 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_36 */
  {
      const ai_i8* conv2d_36_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 816);
    const ai_i8* conv2d_36_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[30] + 0);
    const ai_i32* conv2d_36_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[31] + 0);
    ai_i8* conv2d_36_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 7056);
    ai_i16* conv2d_36_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 5520);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(36, 1, {(stai_ptr) conv2d_36_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_36_t_in_0_ptr_const_s8, conv2d_36_t_in_0_shape_w_const_u16, conv2d_36_t_in_0_shape_h_const_u16, conv2d_36_l_stride_1_const_u16, conv2d_36_l_stride_0_const_u16, conv2d_36_t_in_0_shape_ch_const_u16, conv2d_36_t_weight_0_ptr_const_s8, conv2d_36_t_out_0_shape_ch_const_u16, conv2d_36_t_weight_1_ptr_const_s32, conv2d_36_t_in_0_fmt_zero_const_s8, conv2d_36_t_out_0_fmt_zero_const_s8, conv2d_36_t_in_0_fmt_scale_const_f32, conv2d_36_t_out_0_fmt_scale_const_f32, conv2d_36_t_weight_0_fmt_scale_const_f32, conv2d_36_l_out_ch_format_const_layer_format_type, conv2d_36_t_out_0_ptr_s8, 1, 1536, conv2d_36_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(36, 1, {(stai_ptr) conv2d_36_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_36 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_37 */
  {
      const ai_i8* conv2d_37_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 7056);
    const ai_i8* conv2d_37_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[32] + 0);
    const ai_i32* conv2d_37_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[33] + 0);
    ai_i8* conv2d_37_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_37_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 35280);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(37, 1, {(stai_ptr) conv2d_37_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_37_t_in_0_ptr_const_s8, conv2d_37_t_in_0_shape_w_const_u16, conv2d_37_t_in_0_shape_h_const_u16, conv2d_37_t_in_0_shape_ch_const_u16, conv2d_37_t_weight_0_ptr_const_s8, conv2d_37_t_weight_0_shape_w_const_u16, conv2d_37_t_weight_0_shape_h_const_u16, conv2d_37_l_pad_W_0_const_s32, conv2d_37_l_pad_H_0_const_s32, conv2d_37_l_stride_1_const_u16, conv2d_37_l_stride_0_const_u16, conv2d_37_t_weight_1_ptr_const_s32, conv2d_37_t_in_0_fmt_zero_const_s8, conv2d_37_t_out_0_fmt_zero_const_s8, conv2d_37_t_in_0_fmt_scale_const_f32, conv2d_37_t_out_0_fmt_scale_const_f32, conv2d_37_t_weight_0_fmt_scale_const_f32, conv2d_37_t_out_0_ptr_s8, conv2d_37_t_out_0_shape_w_const_u16, conv2d_37_t_out_0_shape_h_const_u16, 0, 12241, conv2d_37_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(37, 1, {(stai_ptr) conv2d_37_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_37 */
  /* LITE_KERNEL_SECTION BEGIN pool_38 */
  {
    
  forward_lite_pool_38(net_ctx);
  }
  /* LITE_KERNEL_SECTION END pool_38 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_43 */
  {
      const ai_i8* conv2d_43_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 7056);
    const ai_i8* conv2d_43_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[34] + 0);
    const ai_i32* conv2d_43_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[35] + 0);
    ai_i8* conv2d_43_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 8136);
    ai_i16* conv2d_43_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 7200);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(43, 1, {(stai_ptr) conv2d_43_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_43_t_in_0_ptr_const_s8, conv2d_43_t_in_0_shape_w_const_u16, conv2d_43_t_in_0_shape_h_const_u16, conv2d_43_l_stride_1_const_u16, conv2d_43_l_stride_0_const_u16, conv2d_43_t_in_0_shape_ch_const_u16, conv2d_43_t_weight_0_ptr_const_s8, conv2d_43_t_out_0_shape_ch_const_u16, conv2d_43_t_weight_1_ptr_const_s32, conv2d_43_t_in_0_fmt_zero_const_s8, conv2d_43_t_out_0_fmt_zero_const_s8, conv2d_43_t_in_0_fmt_scale_const_f32, conv2d_43_t_out_0_fmt_scale_const_f32, conv2d_43_t_weight_0_fmt_scale_const_f32, conv2d_43_l_out_ch_format_const_layer_format_type, conv2d_43_t_out_0_ptr_s8, 1, 936, conv2d_43_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(43, 1, {(stai_ptr) conv2d_43_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_43 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_44 */
  {
      const ai_i8* conv2d_44_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 8136);
    const ai_i8* conv2d_44_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[36] + 0);
    const ai_i32* conv2d_44_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[37] + 0);
    ai_i8* conv2d_44_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 7056);
    ai_i16* conv2d_44_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 8172);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(44, 1, {(stai_ptr) conv2d_44_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_44_t_in_0_ptr_const_s8, conv2d_44_t_in_0_shape_w_const_u16, conv2d_44_t_in_0_shape_h_const_u16, conv2d_44_l_stride_1_const_u16, conv2d_44_l_stride_0_const_u16, conv2d_44_t_in_0_shape_ch_const_u16, conv2d_44_t_weight_0_ptr_const_s8, conv2d_44_t_out_0_shape_ch_const_u16, conv2d_44_t_weight_1_ptr_const_s32, conv2d_44_t_in_0_fmt_zero_const_s8, conv2d_44_t_out_0_fmt_zero_const_s8, conv2d_44_t_in_0_fmt_scale_const_f32, conv2d_44_t_out_0_fmt_scale_const_f32, conv2d_44_t_weight_0_fmt_scale_const_f32, conv2d_44_l_out_ch_format_const_layer_format_type, conv2d_44_t_out_0_ptr_s8, 1, 1584, conv2d_44_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(44, 1, {(stai_ptr) conv2d_44_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_44 */
  /* LITE_KERNEL_SECTION BEGIN nl_45 */
  {
    
  forward_lite_nl_45(net_ctx);
  }
  /* LITE_KERNEL_SECTION END nl_45 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_46 */
  {
    
  forward_lite_eltwise_46(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_46 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_47 */
  {
      const ai_i8* conv2d_47_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 7344);
    const ai_i8* conv2d_47_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[38] + 0);
    const ai_i32* conv2d_47_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[39] + 0);
    ai_i8* conv2d_47_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 976);
    ai_i16* conv2d_47_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(47, 1, {(stai_ptr) conv2d_47_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_47_t_in_0_ptr_const_s8, conv2d_47_t_in_0_shape_w_const_u16, conv2d_47_t_in_0_shape_h_const_u16, conv2d_47_l_stride_1_const_u16, conv2d_47_l_stride_0_const_u16, conv2d_47_t_in_0_shape_ch_const_u16, conv2d_47_t_weight_0_ptr_const_s8, conv2d_47_t_out_0_shape_ch_const_u16, conv2d_47_t_weight_1_ptr_const_s32, conv2d_47_t_in_0_fmt_zero_const_s8, conv2d_47_t_out_0_fmt_zero_const_s8, conv2d_47_t_in_0_fmt_scale_const_f32, conv2d_47_t_out_0_fmt_scale_const_f32, conv2d_47_t_weight_0_fmt_scale_const_f32, conv2d_47_l_out_ch_format_const_layer_format_type, conv2d_47_t_out_0_ptr_s8, 1, 976, conv2d_47_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(47, 1, {(stai_ptr) conv2d_47_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_47 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_48 */
  {
      const ai_i8* conv2d_48_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 976);
    const ai_i8* conv2d_48_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[40] + 0);
    const ai_i32* conv2d_48_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[41] + 0);
    ai_i8* conv2d_48_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 5496);
    ai_i16* conv2d_48_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 2936);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(48, 1, {(stai_ptr) conv2d_48_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_48_t_in_0_ptr_const_s8, conv2d_48_t_in_0_shape_w_const_u16, conv2d_48_t_in_0_shape_h_const_u16, conv2d_48_l_stride_1_const_u16, conv2d_48_l_stride_0_const_u16, conv2d_48_t_in_0_shape_ch_const_u16, conv2d_48_t_weight_0_ptr_const_s8, conv2d_48_t_out_0_shape_ch_const_u16, conv2d_48_t_weight_1_ptr_const_s32, conv2d_48_t_in_0_fmt_zero_const_s8, conv2d_48_t_out_0_fmt_zero_const_s8, conv2d_48_t_in_0_fmt_scale_const_f32, conv2d_48_t_out_0_fmt_scale_const_f32, conv2d_48_t_weight_0_fmt_scale_const_f32, conv2d_48_l_out_ch_format_const_layer_format_type, conv2d_48_t_out_0_ptr_s8, 1, 2560, conv2d_48_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(48, 1, {(stai_ptr) conv2d_48_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_48 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_49 */
  {
      const ai_i8* conv2d_49_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 5496);
    const ai_i8* conv2d_49_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[42] + 0);
    const ai_i32* conv2d_49_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[43] + 0);
    ai_i8* conv2d_49_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 37660);
    ai_i16* conv2d_49_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 17256);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(49, 1, {(stai_ptr) conv2d_49_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_49_t_in_0_ptr_const_s8, conv2d_49_t_in_0_shape_w_const_u16, conv2d_49_t_in_0_shape_h_const_u16, conv2d_49_t_in_0_shape_ch_const_u16, conv2d_49_t_weight_0_ptr_const_s8, conv2d_49_t_weight_0_shape_w_const_u16, conv2d_49_t_weight_0_shape_h_const_u16, conv2d_49_l_pad_W_0_const_s32, conv2d_49_l_pad_H_0_const_s32, conv2d_49_l_stride_1_const_u16, conv2d_49_l_stride_0_const_u16, conv2d_49_t_weight_1_ptr_const_s32, conv2d_49_t_in_0_fmt_zero_const_s8, conv2d_49_t_out_0_fmt_zero_const_s8, conv2d_49_t_in_0_fmt_scale_const_f32, conv2d_49_t_out_0_fmt_scale_const_f32, conv2d_49_t_weight_0_fmt_scale_const_f32, conv2d_49_t_out_0_ptr_s8, conv2d_49_t_out_0_shape_w_const_u16, conv2d_49_t_out_0_shape_h_const_u16, 0, 20401, conv2d_49_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(49, 1, {(stai_ptr) conv2d_49_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_49 */
  /* LITE_KERNEL_SECTION BEGIN pool_50 */
  {
    
  forward_lite_pool_50(net_ctx);
  }
  /* LITE_KERNEL_SECTION END pool_50 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_55 */
  {
      const ai_i8* conv2d_55_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_55_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[44] + 0);
    const ai_i32* conv2d_55_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[45] + 0);
    ai_i8* conv2d_55_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1800);
    ai_i16* conv2d_55_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 240);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(55, 1, {(stai_ptr) conv2d_55_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_55_t_in_0_ptr_const_s8, conv2d_55_t_in_0_shape_w_const_u16, conv2d_55_t_in_0_shape_h_const_u16, conv2d_55_l_stride_1_const_u16, conv2d_55_l_stride_0_const_u16, conv2d_55_t_in_0_shape_ch_const_u16, conv2d_55_t_weight_0_ptr_const_s8, conv2d_55_t_out_0_shape_ch_const_u16, conv2d_55_t_weight_1_ptr_const_s32, conv2d_55_t_in_0_fmt_zero_const_s8, conv2d_55_t_out_0_fmt_zero_const_s8, conv2d_55_t_in_0_fmt_scale_const_f32, conv2d_55_t_out_0_fmt_scale_const_f32, conv2d_55_t_weight_0_fmt_scale_const_f32, conv2d_55_l_out_ch_format_const_layer_format_type, conv2d_55_t_out_0_ptr_s8, 1, 1560, conv2d_55_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(55, 1, {(stai_ptr) conv2d_55_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_55 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_56 */
  {
      const ai_i8* conv2d_56_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1800);
    const ai_i8* conv2d_56_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[46] + 0);
    const ai_i32* conv2d_56_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[47] + 0);
    ai_i8* conv2d_56_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_56_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 1860);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(56, 1, {(stai_ptr) conv2d_56_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_56_t_in_0_ptr_const_s8, conv2d_56_t_in_0_shape_w_const_u16, conv2d_56_t_in_0_shape_h_const_u16, conv2d_56_l_stride_1_const_u16, conv2d_56_l_stride_0_const_u16, conv2d_56_t_in_0_shape_ch_const_u16, conv2d_56_t_weight_0_ptr_const_s8, conv2d_56_t_out_0_shape_ch_const_u16, conv2d_56_t_weight_1_ptr_const_s32, conv2d_56_t_in_0_fmt_zero_const_s8, conv2d_56_t_out_0_fmt_zero_const_s8, conv2d_56_t_in_0_fmt_scale_const_f32, conv2d_56_t_out_0_fmt_scale_const_f32, conv2d_56_t_weight_0_fmt_scale_const_f32, conv2d_56_l_out_ch_format_const_layer_format_type, conv2d_56_t_out_0_ptr_s8, 1, 2640, conv2d_56_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(56, 1, {(stai_ptr) conv2d_56_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_56 */
  /* LITE_KERNEL_SECTION BEGIN nl_57 */
  {
    
  forward_lite_nl_57(net_ctx);
  }
  /* LITE_KERNEL_SECTION END nl_57 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_58 */
  {
    
  forward_lite_eltwise_58(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_58 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_59 */
  {
      const ai_i8* conv2d_59_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 480);
    const ai_i8* conv2d_59_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[48] + 0);
    const ai_i32* conv2d_59_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[49] + 0);
    ai_i8* conv2d_59_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 13600);
    ai_i16* conv2d_59_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 12240);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(59, 1, {(stai_ptr) conv2d_59_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_59_t_in_0_ptr_const_s8, conv2d_59_t_in_0_shape_w_const_u16, conv2d_59_t_in_0_shape_h_const_u16, conv2d_59_l_stride_1_const_u16, conv2d_59_l_stride_0_const_u16, conv2d_59_t_in_0_shape_ch_const_u16, conv2d_59_t_weight_0_ptr_const_s8, conv2d_59_t_out_0_shape_ch_const_u16, conv2d_59_t_weight_1_ptr_const_s32, conv2d_59_t_in_0_fmt_zero_const_s8, conv2d_59_t_out_0_fmt_zero_const_s8, conv2d_59_t_in_0_fmt_scale_const_f32, conv2d_59_t_out_0_fmt_scale_const_f32, conv2d_59_t_weight_0_fmt_scale_const_f32, conv2d_59_l_out_ch_format_const_layer_format_type, conv2d_59_t_out_0_ptr_s8, 1, 1360, conv2d_59_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(59, 1, {(stai_ptr) conv2d_59_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_59 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_60 */
  {
      const ai_i8* conv2d_60_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 13600);
    const ai_i8* conv2d_60_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[50] + 0);
    const ai_i32* conv2d_60_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[51] + 0);
    ai_i8* conv2d_60_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 800);
    ai_i16* conv2d_60_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(60, 1, {(stai_ptr) conv2d_60_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_60_t_in_0_ptr_const_s8, conv2d_60_t_in_0_shape_w_const_u16, conv2d_60_t_in_0_shape_h_const_u16, conv2d_60_l_stride_1_const_u16, conv2d_60_l_stride_0_const_u16, conv2d_60_t_in_0_shape_ch_const_u16, conv2d_60_t_weight_0_ptr_const_s8, conv2d_60_t_out_0_shape_ch_const_u16, conv2d_60_t_weight_1_ptr_const_s32, conv2d_60_t_in_0_fmt_zero_const_s8, conv2d_60_t_out_0_fmt_zero_const_s8, conv2d_60_t_in_0_fmt_scale_const_f32, conv2d_60_t_out_0_fmt_scale_const_f32, conv2d_60_t_weight_0_fmt_scale_const_f32, conv2d_60_l_out_ch_format_const_layer_format_type, conv2d_60_t_out_0_ptr_s8, 1, 800, conv2d_60_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(60, 1, {(stai_ptr) conv2d_60_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_60 */
  /* LITE_KERNEL_SECTION BEGIN pool_61 */
  {
    
  forward_lite_pool_61(net_ctx);
  }
  /* LITE_KERNEL_SECTION END pool_61 */
  /* LITE_KERNEL_SECTION BEGIN gemm_62 */
  {
    
  forward_lite_gemm_62(net_ctx);
  }
  /* LITE_KERNEL_SECTION END gemm_62 */
  /* LITE_KERNEL_SECTION BEGIN nl_63 */
  {
      ai_i8* nl_63_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_outputs[0] + 0);
    const ai_i8* nl_63_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 292);
    ai_i32* nl_63_t_scratch_0_ptr_s32 = (ai_i32*)(net_ctx->_activations[0] + 304);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(63, 1, {(stai_ptr) nl_63_t_in_0_ptr_const_s8});
    
  forward_lite_nl_softmax_is8os8(nl_63_t_out_0_ptr_s8, nl_63_t_in_0_ptr_const_s8, nl_63_t_in_0_shape_ch_prod_const_u32, 1, 10, 1886985600, 23, -248, nl_63_t_scratch_0_ptr_s32);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(63, 1, {(stai_ptr) nl_63_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END nl_63 */
  return net_ctx->_return_code;
}

/*****************************************************************************/
/*  Getters APIs Section  */
STAI_API_ENTRY
stai_size stai_network_get_context_size()
{
  return (stai_size)STAI_NETWORK_CONTEXT_SIZE;
}

#if defined(HAVE_NETWORK_INFO)
STAI_API_ENTRY
stai_return_code stai_network_get_info(
  stai_network* network,
  stai_network_info* info)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, info==NULL, STAI_ERROR_NETWORK_INVALID_INFO, net_ctx->_return_code)

  // Copy of network info struct
  *info = g_network_info;

  return STAI_SUCCESS;
}
#endif


STAI_API_ENTRY
stai_return_code stai_network_get_activations(
  stai_network* network, stai_ptr* activations, stai_size* n_activations)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  _STAI_SET_ERROR(net_ctx, !n_activations, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_activations = STAI_NETWORK_ACTIVATIONS_NUM;
for (stai_size idx=0; activations && (idx<STAI_NETWORK_ACTIVATIONS_NUM); idx++) {
    // get address of the activations buffers
    activations[idx] = net_ctx->_activations[idx];
  }return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_weights(
  stai_network* network, stai_ptr* weights, stai_size* n_weights)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_weights, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_weights = STAI_NETWORK_WEIGHTS_NUM;
for (stai_size idx=0; weights && (idx<STAI_NETWORK_WEIGHTS_NUM); idx++) {
    // get address of the weights buffers
    weights[idx] = net_ctx->_weights[idx];
  }return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_inputs(
  stai_network* network, stai_ptr* inputs, stai_size* n_inputs)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_inputs, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_inputs = STAI_NETWORK_IN_NUM;
  for (stai_size idx=0; inputs && (idx<STAI_NETWORK_IN_NUM); idx++) {
    inputs[idx] = net_ctx->_inputs[idx];
  }
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_outputs(
  stai_network* network, stai_ptr* outputs, stai_size* n_outputs)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_outputs, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_outputs = STAI_NETWORK_OUT_NUM;
  for (stai_size idx=0; outputs && (idx<STAI_NETWORK_OUT_NUM); idx++) {
    outputs[idx] = net_ctx->_outputs[idx];
  }
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_error(
  stai_network* network)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  /* return 1st generated error or STAI_SUCCESS if no errors so far */
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_states(
  stai_network* network, stai_ptr* states, stai_size* n_states)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_states, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  /* get the number of internals states (supporting multi-heap also for internal states) */
  *n_states = STAI_NETWORK_STATES_NUM;

  STAI_UNUSED(states)
return net_ctx->_return_code;
}


/*****************************************************************************/
/*  Setters APIs Section  */

STAI_API_ENTRY
stai_return_code stai_network_set_activations(
  stai_network* network,
  const stai_ptr* activations,
  const stai_size n_activations)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
const uintptr_t _activations_alignment[] = STAI_NETWORK_ACTIVATIONS_ALIGNMENTS;
  STAI_PRINT("  [stai_network_set_activations] network(%p) activations[%d]: %p\n\n", net_ctx, n_activations, activations)
  _STAI_SET_ERROR(net_ctx, !activations,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_activations!=STAI_NETWORK_ACTIVATIONS_NUM,
                  STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_NUM, net_ctx->_return_code)

  for (stai_size idx=0; activations && idx<STAI_NETWORK_ACTIVATIONS_NUM; idx++) {
    STAI_PRINT("  activation[%d]: %p\n", idx, activations[idx])
    _STAI_SET_ERROR(net_ctx, activations[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)activations[idx]) & (_activations_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_activations[idx] = activations[idx];
  }
  net_ctx->_inputs[0] = activations[0] + 66316;

  net_ctx->_outputs[0] = activations[0] + 0;
_stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_weights(
  stai_network* network,
  const stai_ptr* weights,
  const stai_size n_weights)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
const uintptr_t _weights_alignment[] = STAI_NETWORK_WEIGHTS_ALIGNMENTS;
  _STAI_SET_ERROR(net_ctx, !weights,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_weights!=STAI_NETWORK_WEIGHTS_NUM,
                  STAI_ERROR_NETWORK_INVALID_WEIGHTS_NUM, net_ctx->_return_code)
  for (stai_size idx=0; weights && idx<STAI_NETWORK_WEIGHTS_NUM; idx++) {
    STAI_PRINT("  weight[%d]: %p\n", idx, weights[idx])
    _STAI_SET_ERROR(net_ctx, weights[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_WEIGHTS_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)weights[idx]) & (_weights_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_weights[idx] = weights[idx];
  }_stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_inputs(
  stai_network* network,
  const stai_ptr* inputs,
  const stai_size n_inputs)
{
  const uintptr_t _inputs_alignment[] = STAI_NETWORK_IN_ALIGNMENTS;
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !inputs,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_inputs!=STAI_NETWORK_IN_NUM,
                  STAI_ERROR_NETWORK_INVALID_IN_NUM, net_ctx->_return_code)

  for (stai_size idx=0; inputs && idx<STAI_NETWORK_IN_NUM; idx++) {
    STAI_PRINT("  input[%d]: %p\n", idx, inputs[idx])
    _STAI_SET_ERROR(net_ctx, inputs[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_IN_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)inputs[idx]) & (_inputs_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_inputs[idx] = inputs[idx];
  }

  _stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_outputs(
  stai_network* network,
  const stai_ptr* outputs,
  const stai_size n_outputs)
{
  const uintptr_t _outputs_alignment[] = STAI_NETWORK_OUT_ALIGNMENTS;
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !outputs,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_outputs!=STAI_NETWORK_OUT_NUM,
                  STAI_ERROR_NETWORK_INVALID_OUT_NUM, net_ctx->_return_code)

  for (stai_size idx=0; outputs && idx<n_outputs; idx++) {
    STAI_PRINT("  output[%d]: %p\n", idx, outputs[idx])
    _STAI_SET_ERROR(net_ctx, outputs[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_OUT_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)outputs[idx]) & (_outputs_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_outputs[idx] = outputs[idx];
  }

  _stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_states(
  stai_network* network,
  const stai_ptr* states,
  const stai_size n_states)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  STAI_UNUSED(states)
  STAI_UNUSED(n_states)
_stai_network_check(net_ctx);
  return net_ctx->_return_code;
}

STAI_API_ENTRY
stai_return_code stai_network_set_callback(
  stai_network* network, const stai_event_cb cb, void* cb_cookie)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  STAI_PRINT("  set_callback %p cb %p cookie %p\n", net_ctx, cb, cb_cookie)
  // _STAI_SET_ERROR(net_ctx, cb==NULL, STAI_ERROR_NETWORK_INVALID_CALLBACK, net_ctx->_return_code)
  net_ctx->_callback = cb;
  net_ctx->_callback_cookie = cb_cookie;
  return net_ctx->_return_code;
}

#undef _STAI_SET_ERROR
#undef _STAI_CONTEXT_ALIGNMENT
#undef _STAI_CONTEXT_ACQUIRE
#undef _STAI_NETWORK_EVENT_NODE_START_CB
#undef _STAI_NETWORK_EVENT_NODE_STOP_CB
#undef _STAI_NETWORK_MODEL_SIGNATURE
#undef _STAI_NETWORK_DATETIME
#undef _STAI_NETWORK_COMPILE_DATETIME

