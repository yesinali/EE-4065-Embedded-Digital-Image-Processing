/**
  ******************************************************************************
  * @file    network.c
  * @author  AST Embedded Analytics Research Platform
  * @date    2026-01-15T18:50:43+0000
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

#include "ai_lite_inspect.h"

#include "lite_operators.h"
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
#define _STAI_NETWORK_MODEL_SIGNATURE     "0x8f597c6ef596b2933d1aeaa061997c8d"
#define _STAI_NETWORK_DATETIME            "2026-01-15T18:50:43+0000"
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
      STAI_DECLARE_ARRAY(int32_t, 1, 25976),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },
  .weights = (stai_tensor[STAI_NETWORK_WEIGHTS_NUM]) {
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_1_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 216),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_2_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_2_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 96),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_3_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_3_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 288),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_4_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_4_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 48),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_5_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_5_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 108),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_6_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_6_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 48),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_7_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_7_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 288),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_8_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_8_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 96),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_9_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_9_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 576),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_10_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_10_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 48),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_11_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_11_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 108),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_12_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_12_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 48),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_13_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_13_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 576),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_14_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_14_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 192),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_15_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_15_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1152),
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
      STAI_DECLARE_ARRAY(int32_t, 1, 216),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_18_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_18_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 96),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_19_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_19_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1152),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_20_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_20_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 192),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_21_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_21_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 2304),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_22_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_22_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 96),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_23_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_23_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 216),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_24_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_24_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 96),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_25_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_25_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 2304),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_26_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_26_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 384),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_27_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_27_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 12288),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_28_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_28_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_29_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_29_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1280),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_30_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_30_SIZE_BYTES,
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
      (stai_ptr)g_network_conv2d_0_weights_array,(stai_ptr)g_network_conv2d_0_bias_array,(stai_ptr)g_network_conv2d_1_weights_array,(stai_ptr)g_network_conv2d_1_bias_array,(stai_ptr)g_network_conv2d_11_weights_array,(stai_ptr)g_network_conv2d_11_bias_array,(stai_ptr)g_network_conv2d_12_weights_array,(stai_ptr)g_network_conv2d_12_bias_array,(stai_ptr)g_network_conv2d_16_weights_array,(stai_ptr)g_network_conv2d_16_bias_array,(stai_ptr)g_network_conv2d_26_weights_array,(stai_ptr)g_network_conv2d_26_bias_array,(stai_ptr)g_network_conv2d_27_weights_array,(stai_ptr)g_network_conv2d_27_bias_array,(stai_ptr)g_network_conv2d_29_weights_array,(stai_ptr)g_network_conv2d_29_bias_array,(stai_ptr)g_network_conv2d_39_weights_array,(stai_ptr)g_network_conv2d_39_bias_array,(stai_ptr)g_network_conv2d_40_weights_array,(stai_ptr)g_network_conv2d_40_bias_array,(stai_ptr)g_network_conv2d_44_weights_array,(stai_ptr)g_network_conv2d_44_bias_array,(stai_ptr)g_network_conv2d_54_weights_array,(stai_ptr)g_network_conv2d_54_bias_array,(stai_ptr)g_network_conv2d_55_weights_array,(stai_ptr)g_network_conv2d_55_bias_array,(stai_ptr)g_network_conv2d_57_weights_array,(stai_ptr)g_network_conv2d_57_bias_array,(stai_ptr)g_network_gemm_59_weights_array,(stai_ptr)g_network_gemm_59_bias_array
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
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_0_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.05027042329311371f),
    AI_PACK_INTQ_ZP(-1)))

/* Int quant #1 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(nl_0_nl_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.05027042329311371f),
    AI_PACK_INTQ_ZP(-1)))

/* Int quant #2 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(pool_13_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.05027042329311371f),
    AI_PACK_INTQ_ZP(-1)))

/* Int quant #3 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_1_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.02701232023537159f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #4 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(transpose_6_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.02701232023537159f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #5 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_12_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.05027042329311371f),
    AI_PACK_INTQ_ZP(-1)))

/* Int quant #6 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(concat_14_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.05027042329311371f),
    AI_PACK_INTQ_ZP(-1)))

/* Int quant #7 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(nl_15_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.025288520380854607f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #8 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_16_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.025270722806453705f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #9 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(transpose_21_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.025270722806453705f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #10 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_27_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.06529568135738373f),
    AI_PACK_INTQ_ZP(-10)))

/* Int quant #11 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_28_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.045860446989536285f),
    AI_PACK_INTQ_ZP(14)))

/* Int quant #12 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(nl_28_nl_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.045860446989536285f),
    AI_PACK_INTQ_ZP(14)))

/* Int quant #13 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(pool_41_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.045860446989536285f),
    AI_PACK_INTQ_ZP(14)))

/* Int quant #14 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_29_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.02582520991563797f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #15 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(transpose_34_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.02582520991563797f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #16 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_40_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.045860446989536285f),
    AI_PACK_INTQ_ZP(14)))

/* Int quant #17 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(concat_42_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.045860446989536285f),
    AI_PACK_INTQ_ZP(14)))

/* Int quant #18 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(nl_43_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.020261414349079132f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #19 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_44_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.023730019107460976f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #20 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(transpose_49_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.023730019107460976f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #21 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_55_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.06528100371360779f),
    AI_PACK_INTQ_ZP(4)))

/* Int quant #22 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_56_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.03139876574277878f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #23 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_57_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.04918375983834267f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #24 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(pool_58_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.011819638311862946f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #25 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_59_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.15971285104751587f),
    AI_PACK_INTQ_ZP(4)))

/* Int quant #26 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_59_weights_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 10,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.006102012004703283f, 0.00609638961032033f, 0.005447993520647287f, 0.005259854253381491f, 0.005884373094886541f, 0.006743642967194319f, 0.0055161574855446815f, 0.006351787131279707f, 0.006275787018239498f, 0.0057706465013325214f),
    AI_PACK_INTQ_ZP(0, 0, 0, 0, 0, 0, 0, 0, 0, 0)))



/* Array#0 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_0_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 18816, AI_STATIC)

/* Array#1 */
AI_ARRAY_OBJ_DECLARE(
  nl_0_nl_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 18816, AI_STATIC)

/* Array#2 */
AI_ARRAY_OBJ_DECLARE(
  pool_13_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 4704, AI_STATIC)

/* Array#3 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_1_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 9408, AI_STATIC)

/* Array#4 */
AI_ARRAY_OBJ_DECLARE(
  transpose_6_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 9408, AI_STATIC)

/* Array#5 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_12_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 4704, AI_STATIC)

/* Array#6 */
AI_ARRAY_OBJ_DECLARE(
  concat_14_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 9408, AI_STATIC)

/* Array#7 */
AI_ARRAY_OBJ_DECLARE(
  nl_15_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 9408, AI_STATIC)

/* Array#8 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_16_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 2352, AI_STATIC)

/* Array#9 */
AI_ARRAY_OBJ_DECLARE(
  transpose_21_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 2352, AI_STATIC)

/* Array#10 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_27_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 9408, AI_STATIC)

/* Array#11 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_28_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 9408, AI_STATIC)

/* Array#12 */
AI_ARRAY_OBJ_DECLARE(
  nl_28_nl_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 9408, AI_STATIC)

/* Array#13 */
AI_ARRAY_OBJ_DECLARE(
  pool_41_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 2352, AI_STATIC)

/* Array#14 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_29_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 4704, AI_STATIC)

/* Array#15 */
AI_ARRAY_OBJ_DECLARE(
  transpose_34_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 4704, AI_STATIC)

/* Array#16 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_40_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 2352, AI_STATIC)

/* Array#17 */
AI_ARRAY_OBJ_DECLARE(
  concat_42_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 4704, AI_STATIC)

/* Array#18 */
AI_ARRAY_OBJ_DECLARE(
  nl_43_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 4704, AI_STATIC)

/* Array#19 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_44_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 1176, AI_STATIC)

/* Array#20 */
AI_ARRAY_OBJ_DECLARE(
  transpose_49_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 1176, AI_STATIC)

/* Array#21 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_55_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 4704, AI_STATIC)

/* Array#22 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_56_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 4704, AI_STATIC)

/* Array#23 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_57_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 6272, AI_STATIC)

/* Array#24 */
AI_ARRAY_OBJ_DECLARE(
  pool_58_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 128, AI_STATIC)

/* Array#25 */
AI_ARRAY_OBJ_DECLARE(
  gemm_59_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 10, AI_STATIC)

/* Array#26 */
AI_ARRAY_OBJ_DECLARE(
  gemm_59_weights_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 1280, AI_STATIC)

/* Array#27 */
AI_ARRAY_OBJ_DECLARE(
  gemm_59_bias_array, AI_ARRAY_FORMAT_S32,
  NULL, NULL, 10, AI_STATIC)

/* Array#28 */
AI_ARRAY_OBJ_DECLARE(
  gemm_59_scratch0_array, AI_ARRAY_FORMAT_S16,
  NULL, NULL, 178, AI_STATIC)



/* Tensor #0 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_0_output, AI_STATIC,
  3, 0x1,
  AI_SHAPE_INIT(4, 1, 24, 28, 28), AI_STRIDE_INIT(4, 1, 1, 24, 672),
  1, &conv2d_0_output_array, &conv2d_0_output_array_intq)

/* Tensor #1 */
AI_TENSOR_OBJ_DECLARE(
  nl_0_nl_output, AI_STATIC,
  72, 0x1,
  AI_SHAPE_INIT(4, 1, 24, 28, 28), AI_STRIDE_INIT(4, 1, 1, 24, 672),
  1, &nl_0_nl_output_array, &nl_0_nl_output_array_intq)

/* Tensor #2 */
AI_TENSOR_OBJ_DECLARE(
  pool_13_output, AI_STATIC,
  78, 0x1,
  AI_SHAPE_INIT(4, 1, 24, 14, 14), AI_STRIDE_INIT(4, 1, 1, 24, 336),
  1, &pool_13_output_array, &pool_13_output_array_intq)

/* Tensor #3 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_1_output0, AI_STATIC,
  22, 0x1,
  AI_SHAPE_INIT(5, 1, 6, 28, 28, 2), AI_STRIDE_INIT(5, 1, 1, 12, 336, 6),
  1, &conv2d_1_output_array, &conv2d_1_output_array_intq)

/* Tensor #4 */
AI_TENSOR_OBJ_DECLARE(
  transpose_6_output, AI_STATIC,
  88, 0x1,
  AI_SHAPE_INIT(5, 1, 2, 28, 28, 6), AI_STRIDE_INIT(5, 1, 1, 12, 336, 2),
  1, &transpose_6_output_array, &transpose_6_output_array_intq)

/* Tensor #5 */
AI_TENSOR_OBJ_DECLARE(
  concat_14_output, AI_STATIC,
  0, 0x1,
  AI_SHAPE_INIT(4, 1, 48, 14, 14), AI_STRIDE_INIT(4, 1, 1, 48, 672),
  1, &concat_14_output_array, &concat_14_output_array_intq)

/* Tensor #6 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_12_output, AI_STATIC,
  12, 0x1,
  AI_SHAPE_INIT(4, 1, 24, 14, 14), AI_STRIDE_INIT(4, 1, 1, 24, 336),
  1, &conv2d_12_output_array, &conv2d_12_output_array_intq)

/* Tensor #7 */
AI_TENSOR_OBJ_DECLARE(
  nl_15_output, AI_STATIC,
  73, 0x1,
  AI_SHAPE_INIT(4, 1, 48, 14, 14), AI_STRIDE_INIT(4, 1, 1, 48, 672),
  1, &nl_15_output_array, &nl_15_output_array_intq)

/* Tensor #8 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_16_output0, AI_STATIC,
  17, 0x1,
  AI_SHAPE_INIT(5, 1, 6, 14, 14, 2), AI_STRIDE_INIT(5, 1, 1, 12, 168, 6),
  1, &conv2d_16_output_array, &conv2d_16_output_array_intq)

/* Tensor #9 */
AI_TENSOR_OBJ_DECLARE(
  transpose_21_output, AI_STATIC,
  82, 0x1,
  AI_SHAPE_INIT(5, 1, 2, 14, 14, 6), AI_STRIDE_INIT(5, 1, 1, 12, 168, 2),
  1, &transpose_21_output_array, &transpose_21_output_array_intq)

/* Tensor #10 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_27_output, AI_STATIC,
  31, 0x1,
  AI_SHAPE_INIT(4, 1, 48, 14, 14), AI_STRIDE_INIT(4, 1, 1, 48, 672),
  1, &conv2d_27_output_array, &conv2d_27_output_array_intq)

/* Tensor #11 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_28_output, AI_STATIC,
  66, 0x1,
  AI_SHAPE_INIT(4, 1, 48, 14, 14), AI_STRIDE_INIT(4, 1, 1, 48, 672),
  1, &eltwise_28_output_array, &eltwise_28_output_array_intq)

/* Tensor #12 */
AI_TENSOR_OBJ_DECLARE(
  nl_28_nl_output, AI_STATIC,
  74, 0x1,
  AI_SHAPE_INIT(4, 1, 48, 14, 14), AI_STRIDE_INIT(4, 1, 1, 48, 672),
  1, &nl_28_nl_output_array, &nl_28_nl_output_array_intq)

/* Tensor #13 */
AI_TENSOR_OBJ_DECLARE(
  pool_41_output, AI_STATIC,
  79, 0x1,
  AI_SHAPE_INIT(4, 1, 48, 7, 7), AI_STRIDE_INIT(4, 1, 1, 48, 336),
  1, &pool_41_output_array, &pool_41_output_array_intq)

/* Tensor #14 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_29_output0, AI_STATIC,
  36, 0x1,
  AI_SHAPE_INIT(5, 1, 12, 14, 14, 2), AI_STRIDE_INIT(5, 1, 1, 24, 336, 12),
  1, &conv2d_29_output_array, &conv2d_29_output_array_intq)

/* Tensor #15 */
AI_TENSOR_OBJ_DECLARE(
  transpose_34_output, AI_STATIC,
  84, 0x1,
  AI_SHAPE_INIT(5, 1, 2, 14, 14, 12), AI_STRIDE_INIT(5, 1, 1, 24, 336, 2),
  1, &transpose_34_output_array, &transpose_34_output_array_intq)

/* Tensor #16 */
AI_TENSOR_OBJ_DECLARE(
  concat_42_output, AI_STATIC,
  1, 0x1,
  AI_SHAPE_INIT(4, 1, 96, 7, 7), AI_STRIDE_INIT(4, 1, 1, 96, 672),
  1, &concat_42_output_array, &concat_42_output_array_intq)

/* Tensor #17 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_40_output, AI_STATIC,
  45, 0x1,
  AI_SHAPE_INIT(4, 1, 48, 7, 7), AI_STRIDE_INIT(4, 1, 1, 48, 336),
  1, &conv2d_40_output_array, &conv2d_40_output_array_intq)

/* Tensor #18 */
AI_TENSOR_OBJ_DECLARE(
  nl_43_output, AI_STATIC,
  75, 0x1,
  AI_SHAPE_INIT(4, 1, 96, 7, 7), AI_STRIDE_INIT(4, 1, 1, 96, 672),
  1, &nl_43_output_array, &nl_43_output_array_intq)

/* Tensor #19 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_44_output0, AI_STATIC,
  50, 0x1,
  AI_SHAPE_INIT(5, 1, 12, 7, 7, 2), AI_STRIDE_INIT(5, 1, 1, 24, 168, 12),
  1, &conv2d_44_output_array, &conv2d_44_output_array_intq)

/* Tensor #20 */
AI_TENSOR_OBJ_DECLARE(
  transpose_49_output, AI_STATIC,
  86, 0x1,
  AI_SHAPE_INIT(5, 1, 2, 7, 7, 12), AI_STRIDE_INIT(5, 1, 1, 24, 168, 2),
  1, &transpose_49_output_array, &transpose_49_output_array_intq)

/* Tensor #21 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_55_output, AI_STATIC,
  59, 0x1,
  AI_SHAPE_INIT(4, 1, 96, 7, 7), AI_STRIDE_INIT(4, 1, 1, 96, 672),
  1, &conv2d_55_output_array, &conv2d_55_output_array_intq)

/* Tensor #22 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_56_output, AI_STATIC,
  67, 0x1,
  AI_SHAPE_INIT(4, 1, 96, 7, 7), AI_STRIDE_INIT(4, 1, 1, 96, 672),
  1, &eltwise_56_output_array, &eltwise_56_output_array_intq)

/* Tensor #23 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_57_output, AI_STATIC,
  63, 0x1,
  AI_SHAPE_INIT(4, 1, 128, 7, 7), AI_STRIDE_INIT(4, 1, 1, 128, 896),
  1, &conv2d_57_output_array, &conv2d_57_output_array_intq)

/* Tensor #24 */
AI_TENSOR_OBJ_DECLARE(
  pool_58_output, AI_STATIC,
  80, 0x1,
  AI_SHAPE_INIT(4, 1, 128, 1, 1), AI_STRIDE_INIT(4, 1, 1, 128, 128),
  1, &pool_58_output_array, &pool_58_output_array_intq)

/* Tensor #25 */
AI_TENSOR_OBJ_DECLARE(
  gemm_59_bias, AI_STATIC,
  68, 0x0,
  AI_SHAPE_INIT(4, 1, 10, 1, 1), AI_STRIDE_INIT(4, 4, 4, 40, 40),
  1, &gemm_59_bias_array, NULL)

/* Tensor #26 */
AI_TENSOR_OBJ_DECLARE(
  gemm_59_output, AI_STATIC,
  69, 0x1,
  AI_SHAPE_INIT(4, 1, 10, 1, 1), AI_STRIDE_INIT(4, 1, 1, 10, 10),
  1, &gemm_59_output_array, &gemm_59_output_array_intq)

/* Tensor #27 */
AI_TENSOR_OBJ_DECLARE(
  gemm_59_scratch0, AI_STATIC,
  70, 0x0,
  AI_SHAPE_INIT(4, 1, 178, 1, 1), AI_STRIDE_INIT(4, 2, 2, 356, 356),
  1, &gemm_59_scratch0_array, NULL)

/* Tensor #28 */
AI_TENSOR_OBJ_DECLARE(
  gemm_59_weights, AI_STATIC,
  71, 0x1,
  AI_SHAPE_INIT(4, 128, 10, 1, 1), AI_STRIDE_INIT(4, 1, 128, 1280, 1280),
  1, &gemm_59_weights_array, &gemm_59_weights_array_intq)



AI_STATIC_CONST ai_i8 nl_0_nl_nl_params_data[] = { -1 };
AI_ARRAY_OBJ_DECLARE(
    nl_0_nl_nl_params, AI_ARRAY_FORMAT_S8,
    nl_0_nl_nl_params_data, nl_0_nl_nl_params_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  nl_0_nl_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &nl_0_nl_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  nl_0_nl_layer, 0,
  NL_TYPE, 0x0, NULL,
  nl, forward_relu_integer,
  &nl_0_nl_chain,
  NULL, &nl_0_nl_layer, AI_STATIC, 
  .nl_params = &nl_0_nl_nl_params, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  pool_13_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &nl_0_nl_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &pool_13_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  pool_13_layer, 13,
  POOL_TYPE, 0x0, NULL,
  pool, forward_ap_integer_INT8,
  &pool_13_chain,
  NULL, &pool_13_layer, AI_STATIC, 
  .pool_size = AI_SHAPE_2D_INIT(3, 3), 
  .pool_stride = AI_SHAPE_2D_INIT(2, 2), 
  .pool_pad = AI_SHAPE_INIT(4, 0, 0, 2, 2), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  transpose_6_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_1_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &transpose_6_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  transpose_6_layer, 6,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &transpose_6_chain,
  NULL, &transpose_6_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_DEPTH, AI_SHAPE_WIDTH, AI_SHAPE_HEIGHT, AI_SHAPE_CHANNEL, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  concat_14_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &pool_13_output, &conv2d_12_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &concat_14_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  concat_14_layer, 14,
  CONCAT_TYPE, 0x0, NULL,
  concat, forward_concat,
  &concat_14_chain,
  NULL, &concat_14_layer, AI_STATIC, 
  .axis = AI_SHAPE_CHANNEL, 
)


AI_STATIC_CONST ai_i8 nl_15_nl_params_data[] = { -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -126, -124, -122, -120, -118, -116, -114, -112, -110, -108, -106, -104, -102, -100, -98, -96, -94, -92, -90, -88, -86, -84, -82, -80, -78, -76, -74, -72, -70, -68, -66, -64, -62, -60, -58, -56, -54, -52, -50, -48, -46, -45, -43, -41, -39, -37, -35, -33, -31, -29, -27, -25, -23, -21, -19, -17, -15, -13, -11, -9, -7, -5, -3, -1, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93, 95, 97, 99, 101, 103, 105, 107, 109, 111, 113, 115, 117, 118, 120, 122, 124, 126 };
AI_ARRAY_OBJ_DECLARE(
    nl_15_nl_params, AI_ARRAY_FORMAT_S8,
    nl_15_nl_params_data, nl_15_nl_params_data, 256, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  nl_15_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &concat_14_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &nl_15_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  nl_15_layer, 15,
  NL_TYPE, 0x0, NULL,
  nl, forward_nl_integer,
  &nl_15_chain,
  NULL, &nl_15_layer, AI_STATIC, 
  .nl_params = &nl_15_nl_params, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  transpose_21_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_16_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &transpose_21_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  transpose_21_layer, 21,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &transpose_21_chain,
  NULL, &transpose_21_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_DEPTH, AI_SHAPE_WIDTH, AI_SHAPE_HEIGHT, AI_SHAPE_CHANNEL, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_28_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &nl_15_output, &conv2d_27_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_28_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  eltwise_28_layer, 28,
  ELTWISE_INTEGER_TYPE, 0x0, NULL,
  eltwise_integer, forward_eltwise_integer_INT8,
  &eltwise_28_chain,
  NULL, &eltwise_28_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_INT8, 
)


AI_STATIC_CONST ai_i8 nl_28_nl_nl_params_data[] = { 14 };
AI_ARRAY_OBJ_DECLARE(
    nl_28_nl_nl_params, AI_ARRAY_FORMAT_S8,
    nl_28_nl_nl_params_data, nl_28_nl_nl_params_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  nl_28_nl_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_28_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &nl_28_nl_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  nl_28_nl_layer, 28,
  NL_TYPE, 0x0, NULL,
  nl, forward_relu_integer,
  &nl_28_nl_chain,
  NULL, &nl_28_nl_layer, AI_STATIC, 
  .nl_params = &nl_28_nl_nl_params, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  pool_41_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &nl_28_nl_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &pool_41_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  pool_41_layer, 41,
  POOL_TYPE, 0x0, NULL,
  pool, forward_ap_integer_INT8,
  &pool_41_chain,
  NULL, &pool_41_layer, AI_STATIC, 
  .pool_size = AI_SHAPE_2D_INIT(3, 3), 
  .pool_stride = AI_SHAPE_2D_INIT(2, 2), 
  .pool_pad = AI_SHAPE_INIT(4, 0, 0, 2, 2), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  transpose_34_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_29_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &transpose_34_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  transpose_34_layer, 34,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &transpose_34_chain,
  NULL, &transpose_34_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_DEPTH, AI_SHAPE_WIDTH, AI_SHAPE_HEIGHT, AI_SHAPE_CHANNEL, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  concat_42_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &pool_41_output, &conv2d_40_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &concat_42_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  concat_42_layer, 42,
  CONCAT_TYPE, 0x0, NULL,
  concat, forward_concat,
  &concat_42_chain,
  NULL, &concat_42_layer, AI_STATIC, 
  .axis = AI_SHAPE_CHANNEL, 
)


AI_STATIC_CONST ai_i8 nl_43_nl_params_data[] = { -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -126, -123, -121, -119, -117, -114, -112, -110, -108, -105, -103, -101, -99, -96, -94, -92, -90, -87, -85, -83, -80, -78, -76, -74, -71, -69, -67, -65, -62, -60, -58, -56, -53, -51, -49, -47, -44, -42, -40, -37, -35, -33, -31, -28, -26, -24, -22, -19, -17, -15, -13, -10, -8, -6, -4, -1, 1, 3, 6, 8, 10, 12, 15, 17, 19, 21, 24, 26, 28, 30, 33, 35, 37, 39, 42, 44, 46, 49, 51, 53, 55, 58, 60, 62, 64, 67, 69, 71, 73, 76, 78, 80, 82, 85, 87, 89, 92, 94, 96, 98, 101, 103, 105, 107, 110, 112, 114, 116, 119, 121, 123, 126, 127 };
AI_ARRAY_OBJ_DECLARE(
    nl_43_nl_params, AI_ARRAY_FORMAT_S8,
    nl_43_nl_params_data, nl_43_nl_params_data, 256, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  nl_43_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &concat_42_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &nl_43_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  nl_43_layer, 43,
  NL_TYPE, 0x0, NULL,
  nl, forward_nl_integer,
  &nl_43_chain,
  NULL, &nl_43_layer, AI_STATIC, 
  .nl_params = &nl_43_nl_params, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  transpose_49_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_44_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &transpose_49_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  transpose_49_layer, 49,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &transpose_49_chain,
  NULL, &transpose_49_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_DEPTH, AI_SHAPE_WIDTH, AI_SHAPE_HEIGHT, AI_SHAPE_CHANNEL, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_56_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &nl_43_output, &conv2d_55_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_56_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  eltwise_56_layer, 56,
  ELTWISE_INTEGER_TYPE, 0x0, NULL,
  eltwise_integer, forward_eltwise_integer_INT8,
  &eltwise_56_chain,
  NULL, &eltwise_56_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_INT8, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  pool_58_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_57_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &pool_58_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  pool_58_layer, 58,
  POOL_TYPE, 0x0, NULL,
  pool, forward_ap_integer_INT8,
  &pool_58_chain,
  NULL, &pool_58_layer, AI_STATIC, 
  .pool_size = AI_SHAPE_2D_INIT(7, 7), 
  .pool_stride = AI_SHAPE_2D_INIT(7, 7), 
  .pool_pad = AI_SHAPE_INIT(4, 0, 0, 0, 0), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  gemm_59_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &pool_58_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gemm_59_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &gemm_59_weights, &gemm_59_bias),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gemm_59_scratch0)
)

AI_LAYER_OBJ_DECLARE(
  gemm_59_layer, 59,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense_integer_SSSA_ch,
  &gemm_59_chain,
  NULL, &gemm_59_layer, AI_STATIC, 
)
/**  Hybrid layers declarations section  *************************************/
void forward_lite_nl_0_nl(_stai_network_context* net_ctx)
{
  conv2d_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 2240);
  conv2d_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 2240);
  nl_0_nl_output_array.data = AI_PTR(net_ctx->_activations[0] + 2240);
  nl_0_nl_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 2240);
  _STAI_NETWORK_EVENT_NODE_START_CB(0, 1, { conv2d_0_output.data->data});
  forward_relu_integer(&nl_0_nl_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(0, 1, { nl_0_nl_output.data->data});
}
void forward_lite_pool_13(_stai_network_context* net_ctx)
{
  nl_0_nl_output_array.data = AI_PTR(net_ctx->_activations[0] + 2240);
  nl_0_nl_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 2240);
  pool_13_output_array.data = AI_PTR(net_ctx->_activations[0] + 21056);
  pool_13_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 21056);
  _STAI_NETWORK_EVENT_NODE_START_CB(13, 1, { nl_0_nl_output.data->data});
  forward_ap_integer_INT8(&pool_13_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(13, 1, { pool_13_output.data->data});
}
void forward_lite_transpose_6(_stai_network_context* net_ctx)
{
  conv2d_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 1904);
  conv2d_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1904);
  transpose_6_output_array.data = AI_PTR(net_ctx->_activations[0] + 11312);
  transpose_6_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 11312);
  _STAI_NETWORK_EVENT_NODE_START_CB(6, 1, { conv2d_1_output0.data->data});
  forward_transpose(&transpose_6_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(6, 1, { transpose_6_output.data->data});
}
void forward_lite_concat_14(_stai_network_context* net_ctx)
{
  pool_13_output_array.data = AI_PTR(net_ctx->_activations[0] + 21056);
  pool_13_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 21056);
  conv2d_12_output_array.data = AI_PTR(net_ctx->_activations[0] + 4704);
  conv2d_12_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 4704);
  concat_14_output_array.data = AI_PTR(net_ctx->_activations[0] + 9408);
  concat_14_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 9408);
  _STAI_NETWORK_EVENT_NODE_START_CB(14, 2, { pool_13_output.data->data,conv2d_12_output.data->data});
  forward_concat(&concat_14_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(14, 1, { concat_14_output.data->data});
}
void forward_lite_nl_15(_stai_network_context* net_ctx)
{
  concat_14_output_array.data = AI_PTR(net_ctx->_activations[0] + 9408);
  concat_14_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 9408);
  nl_15_output_array.data = AI_PTR(net_ctx->_activations[0] + 9408);
  nl_15_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 9408);
  _STAI_NETWORK_EVENT_NODE_START_CB(15, 1, { concat_14_output.data->data});
  forward_nl_integer(&nl_15_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(15, 1, { nl_15_output.data->data});
}
void forward_lite_transpose_21(_stai_network_context* net_ctx)
{
  conv2d_16_output_array.data = AI_PTR(net_ctx->_activations[0] + 1904);
  conv2d_16_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1904);
  transpose_21_output_array.data = AI_PTR(net_ctx->_activations[0] + 7056);
  transpose_21_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 7056);
  _STAI_NETWORK_EVENT_NODE_START_CB(21, 1, { conv2d_16_output0.data->data});
  forward_transpose(&transpose_21_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(21, 1, { transpose_21_output.data->data});
}
void forward_lite_eltwise_28(_stai_network_context* net_ctx)
{
  nl_15_output_array.data = AI_PTR(net_ctx->_activations[0] + 9408);
  nl_15_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 9408);
  conv2d_27_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  conv2d_27_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  eltwise_28_output_array.data = AI_PTR(net_ctx->_activations[0] + 9408);
  eltwise_28_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 9408);
  _STAI_NETWORK_EVENT_NODE_START_CB(28, 2, { nl_15_output.data->data,conv2d_27_output.data->data});
  forward_eltwise_integer_INT8(&eltwise_28_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(28, 1, { eltwise_28_output.data->data});
}
void forward_lite_nl_28_nl(_stai_network_context* net_ctx)
{
  eltwise_28_output_array.data = AI_PTR(net_ctx->_activations[0] + 9408);
  eltwise_28_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 9408);
  nl_28_nl_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  nl_28_nl_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(28, 1, { eltwise_28_output.data->data});
  forward_relu_integer(&nl_28_nl_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(28, 1, { nl_28_nl_output.data->data});
}
void forward_lite_pool_41(_stai_network_context* net_ctx)
{
  nl_28_nl_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  nl_28_nl_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  pool_41_output_array.data = AI_PTR(net_ctx->_activations[0] + 9408);
  pool_41_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 9408);
  _STAI_NETWORK_EVENT_NODE_START_CB(41, 1, { nl_28_nl_output.data->data});
  forward_ap_integer_INT8(&pool_41_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(41, 1, { pool_41_output.data->data});
}
void forward_lite_transpose_34(_stai_network_context* net_ctx)
{
  conv2d_29_output_array.data = AI_PTR(net_ctx->_activations[0] + 12192);
  conv2d_29_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 12192);
  transpose_34_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  transpose_34_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(34, 1, { conv2d_29_output0.data->data});
  forward_transpose(&transpose_34_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(34, 1, { transpose_34_output.data->data});
}
void forward_lite_concat_42(_stai_network_context* net_ctx)
{
  pool_41_output_array.data = AI_PTR(net_ctx->_activations[0] + 9408);
  pool_41_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 9408);
  conv2d_40_output_array.data = AI_PTR(net_ctx->_activations[0] + 2068);
  conv2d_40_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 2068);
  concat_42_output_array.data = AI_PTR(net_ctx->_activations[0] + 4420);
  concat_42_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 4420);
  _STAI_NETWORK_EVENT_NODE_START_CB(42, 2, { pool_41_output.data->data,conv2d_40_output.data->data});
  forward_concat(&concat_42_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(42, 1, { concat_42_output.data->data});
}
void forward_lite_nl_43(_stai_network_context* net_ctx)
{
  concat_42_output_array.data = AI_PTR(net_ctx->_activations[0] + 4420);
  concat_42_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 4420);
  nl_43_output_array.data = AI_PTR(net_ctx->_activations[0] + 9124);
  nl_43_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 9124);
  _STAI_NETWORK_EVENT_NODE_START_CB(43, 1, { concat_42_output.data->data});
  forward_nl_integer(&nl_43_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(43, 1, { nl_43_output.data->data});
}
void forward_lite_transpose_49(_stai_network_context* net_ctx)
{
  conv2d_44_output_array.data = AI_PTR(net_ctx->_activations[0] + 624);
  conv2d_44_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 624);
  transpose_49_output_array.data = AI_PTR(net_ctx->_activations[0] + 1800);
  transpose_49_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1800);
  _STAI_NETWORK_EVENT_NODE_START_CB(49, 1, { conv2d_44_output0.data->data});
  forward_transpose(&transpose_49_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(49, 1, { transpose_49_output.data->data});
}
void forward_lite_eltwise_56(_stai_network_context* net_ctx)
{
  nl_43_output_array.data = AI_PTR(net_ctx->_activations[0] + 9124);
  nl_43_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 9124);
  conv2d_55_output_array.data = AI_PTR(net_ctx->_activations[0] + 3124);
  conv2d_55_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 3124);
  eltwise_56_output_array.data = AI_PTR(net_ctx->_activations[0] + 13828);
  eltwise_56_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 13828);
  _STAI_NETWORK_EVENT_NODE_START_CB(56, 2, { nl_43_output.data->data,conv2d_55_output.data->data});
  forward_eltwise_integer_INT8(&eltwise_56_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(56, 1, { eltwise_56_output.data->data});
}
void forward_lite_pool_58(_stai_network_context* net_ctx)
{
  conv2d_57_output_array.data = AI_PTR(net_ctx->_activations[0] + 1664);
  conv2d_57_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1664);
  pool_58_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  pool_58_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(58, 1, { conv2d_57_output.data->data});
  forward_ap_integer_INT8(&pool_58_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(58, 1, { pool_58_output.data->data});
}
void forward_lite_gemm_59(_stai_network_context* net_ctx)
{
  pool_58_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  pool_58_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  gemm_59_weights_array.data = AI_PTR(net_ctx->_weights[28] + 0);
  gemm_59_weights_array.data_start = AI_PTR(net_ctx->_weights[28] + 0);
  gemm_59_bias_array.data = AI_PTR(net_ctx->_weights[29] + 0);
  gemm_59_bias_array.data_start = AI_PTR(net_ctx->_weights[29] + 0);
  gemm_59_scratch0_array.data = AI_PTR(net_ctx->_activations[0] + 128);
  gemm_59_scratch0_array.data_start = AI_PTR(net_ctx->_activations[0] + 128);
  gemm_59_output_array.data = AI_PTR(net_ctx->_activations[0] + 484);
  gemm_59_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 484);
  _STAI_NETWORK_EVENT_NODE_START_CB(59, 1, { pool_58_output.data->data});
  forward_dense_integer_SSSA_ch(&gemm_59_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(59, 1, { gemm_59_output.data->data});
}

/*****************************************************************************/


static const ai_u16 conv2d_0_t_in_0_shape_w_const_u16 = 28;
static const ai_u16 conv2d_0_t_in_0_shape_h_const_u16 = 28;
static const ai_u16 conv2d_0_t_in_0_shape_ch_const_u16 = 1;
static const ai_u16 conv2d_0_t_out_0_shape_ch_const_u16 = 24;
static const ai_u16 conv2d_0_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_0_t_weight_0_shape_h_const_u16 = 3;
static const ai_u16 conv2d_0_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_0_l_stride_0_const_u16 = 1;
static const ai_i32 conv2d_0_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_0_l_pad_H_0_const_s32 = 1;
static const ai_i8 conv2d_0_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_0_t_out_0_fmt_zero_const_s8 = -1;
static const ai_float conv2d_0_t_in_0_fmt_scale_const_f32 = 0.003921568859368563f;
static const ai_float conv2d_0_t_out_0_fmt_scale_const_f32 = 0.05027042329311371f;
static const ai_float conv2d_0_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.015671223402023315f, 0.015130935236811638f, 0.011645971797406673f, 0.010978542268276215f, 0.015545526519417763f, 0.010236202739179134f, 0.016970066353678703f, 0.012803426012396812f, 0.012955227866768837f, 0.015902621671557426f, 0.01657022535800934f, 0.01636292040348053f, 0.012973180040717125f, 0.019157398492097855f, 0.01586340181529522f, 0.004823233932256699f, 0.010496046394109726f, 0.015116190537810326f, 0.01730364002287388f, 0.0175803080201149f, 0.018146773800253868f, 0.021045703440904617f, 0.009117367677390575f, 0.016855143010616302f);
static const ai_layer_format_type conv2d_0_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_0_t_out_0_shape_w_const_u16 = 28;
static const ai_u16 conv2d_0_t_out_0_shape_h_const_u16 = 28;



static const ai_u16 conv2d_1_t_in_0_shape_w_const_u16 = 28;
static const ai_u16 conv2d_1_t_in_0_shape_h_const_u16 = 28;
static const ai_u16 conv2d_1_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_1_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_1_t_in_0_shape_ch_const_u16 = 24;
static const ai_u16 conv2d_1_t_out_0_shape_ch_const_u16 = 12;
static const ai_i8 conv2d_1_t_in_0_fmt_zero_const_s8 = -1;
static const ai_i8 conv2d_1_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_1_t_in_0_fmt_scale_const_f32 = 0.05027042329311371f;
static const ai_float conv2d_1_t_out_0_fmt_scale_const_f32 = 0.02701232023537159f;
static const ai_float conv2d_1_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0078027513809502125f, 0.007127768360078335f, 0.0060566384345293045f, 0.005971967242658138f, 0.005144469905644655f, 0.008250904269516468f, 0.004009883385151625f, 0.0047421520575881f, 0.006415290758013725f, 0.003900953335687518f, 0.005234659183770418f, 0.00413147546350956f);
static const ai_layer_format_type conv2d_1_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;


static const ai_i8 conv2d_11_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_11_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_11_pad_before_t_in_0_shape_h_const_u32 = 28;

static const ai_u16 conv2d_11_t_in_0_shape_w_const_u16 = 30;
static const ai_u16 conv2d_11_t_in_0_shape_h_const_u16 = 30;
static const ai_u16 conv2d_11_t_in_0_shape_ch_const_u16 = 12;
static const ai_u16 conv2d_11_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_11_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_11_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_11_t_out_0_fmt_zero_const_s8 = 11;
static const ai_float conv2d_11_t_in_0_fmt_scale_const_f32 = 0.02701232023537159f;
static const ai_float conv2d_11_t_out_0_fmt_scale_const_f32 = 0.057651445269584656f;
static const ai_float conv2d_11_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.007712642662227154f, 0.0050839646719396114f, 0.006979421712458134f, 0.005104715470224619f, 0.005405736155807972f, 0.004168236628174782f, 0.007425182498991489f, 0.01452586054801941f, 0.0037999460473656654f, 0.005118843168020248f, 0.0023624522145837545f, 0.006409503519535065f);
static const ai_u16 conv2d_11_t_out_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_11_t_out_0_shape_h_const_u16 = 14;

static const ai_u16 conv2d_12_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_12_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_12_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_12_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_12_t_in_0_shape_ch_const_u16 = 12;
static const ai_u16 conv2d_12_t_out_0_shape_ch_const_u16 = 24;
static const ai_i8 conv2d_12_t_in_0_fmt_zero_const_s8 = 11;
static const ai_i8 conv2d_12_t_out_0_fmt_zero_const_s8 = -1;
static const ai_float conv2d_12_t_in_0_fmt_scale_const_f32 = 0.057651445269584656f;
static const ai_float conv2d_12_t_out_0_fmt_scale_const_f32 = 0.05027042329311371f;
static const ai_float conv2d_12_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.002479549963027239f, 0.0027816258370876312f, 0.0033565061166882515f, 0.0037466890644282103f, 0.004032304976135492f, 0.003464050590991974f, 0.004091923125088215f, 0.004814847372472286f, 0.00442303204908967f, 0.0034968284890055656f, 0.0037392275407910347f, 0.004075266886502504f, 0.004049613606184721f, 0.002537454478442669f, 0.004907586611807346f, 0.0033844197168946266f, 0.0037169212009757757f, 0.0025823181495070457f, 0.004029750358313322f, 0.0031883076298981905f, 0.003611114574596286f, 0.005784953944385052f, 0.002984082093462348f, 0.0036569014191627502f);
static const ai_layer_format_type conv2d_12_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;



static const ai_u16 conv2d_16_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_16_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_16_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_16_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_16_t_in_0_shape_ch_const_u16 = 48;
static const ai_u16 conv2d_16_t_out_0_shape_ch_const_u16 = 12;
static const ai_i8 conv2d_16_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_16_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_16_t_in_0_fmt_scale_const_f32 = 0.025288520380854607f;
static const ai_float conv2d_16_t_out_0_fmt_scale_const_f32 = 0.025270722806453705f;
static const ai_float conv2d_16_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0031252484768629074f, 0.0030733286403119564f, 0.00443504611030221f, 0.003220298094674945f, 0.008984153158962727f, 0.00399048812687397f, 0.0024454984813928604f, 0.0046828798949718475f, 0.0029775055591017008f, 0.0036976297851651907f, 0.0035339174792170525f, 0.00376253598369658f);
static const ai_layer_format_type conv2d_16_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;


static const ai_i8 conv2d_26_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_26_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_26_pad_before_t_in_0_shape_h_const_u32 = 14;

static const ai_u16 conv2d_26_t_in_0_shape_w_const_u16 = 16;
static const ai_u16 conv2d_26_t_in_0_shape_h_const_u16 = 16;
static const ai_u16 conv2d_26_t_in_0_shape_ch_const_u16 = 12;
static const ai_u16 conv2d_26_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_26_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_26_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_26_t_out_0_fmt_zero_const_s8 = 11;
static const ai_float conv2d_26_t_in_0_fmt_scale_const_f32 = 0.025270722806453705f;
static const ai_float conv2d_26_t_out_0_fmt_scale_const_f32 = 0.04860549420118332f;
static const ai_float conv2d_26_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.005970587022602558f, 0.010788390412926674f, 0.010722246021032333f, 0.006742090918123722f, 0.0044176410883665085f, 0.007414751686155796f, 0.005942260846495628f, 0.0067438469268381596f, 0.004471844062209129f, 0.011485918425023556f, 0.009167320095002651f, 0.00883192103356123f);
static const ai_u16 conv2d_26_t_out_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_26_t_out_0_shape_h_const_u16 = 14;

static const ai_u16 conv2d_27_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_27_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_27_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_27_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_27_t_in_0_shape_ch_const_u16 = 12;
static const ai_u16 conv2d_27_t_out_0_shape_ch_const_u16 = 48;
static const ai_i8 conv2d_27_t_in_0_fmt_zero_const_s8 = 11;
static const ai_i8 conv2d_27_t_out_0_fmt_zero_const_s8 = -10;
static const ai_float conv2d_27_t_in_0_fmt_scale_const_f32 = 0.04860549420118332f;
static const ai_float conv2d_27_t_out_0_fmt_scale_const_f32 = 0.06529568135738373f;
static const ai_float conv2d_27_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.005209020338952541f, 0.0046501075848937035f, 0.004852017387747765f, 0.004675876349210739f, 0.0036609950475394726f, 0.0042776004411280155f, 0.006495890207588673f, 0.006760306190699339f, 0.0049284640699625015f, 0.004142687655985355f, 0.004270962905138731f, 0.0031155217438936234f, 0.005398621316999197f, 0.002824140014126897f, 0.003560611978173256f, 0.004781173542141914f, 0.004040272906422615f, 0.004932320676743984f, 0.004334982018917799f, 0.0023949076421558857f, 0.004104870371520519f, 0.0035272163804620504f, 0.004381135571748018f, 0.004397043492645025f, 0.004965467844158411f, 0.003129673423245549f, 0.0031915053259581327f, 0.004516003653407097f, 0.006176509894430637f, 0.004024054855108261f, 0.004108564928174019f, 0.004724616184830666f, 0.004465850070118904f, 0.004047030583024025f, 0.004905488342046738f, 0.005363560747355223f, 0.0029189083725214005f, 0.00374701083637774f, 0.0040506417863070965f, 0.003888773499056697f, 0.003221832448616624f, 0.004343775101006031f, 0.00525779090821743f, 0.004490274470299482f, 0.005980154033750296f, 0.003394145518541336f, 0.003117920598015189f, 0.003019677707925439f);
static const ai_layer_format_type conv2d_27_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;




static const ai_u16 conv2d_29_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_29_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_29_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_29_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_29_t_in_0_shape_ch_const_u16 = 48;
static const ai_u16 conv2d_29_t_out_0_shape_ch_const_u16 = 24;
static const ai_i8 conv2d_29_t_in_0_fmt_zero_const_s8 = 14;
static const ai_i8 conv2d_29_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_29_t_in_0_fmt_scale_const_f32 = 0.045860446989536285f;
static const ai_float conv2d_29_t_out_0_fmt_scale_const_f32 = 0.02582520991563797f;
static const ai_float conv2d_29_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0017678719013929367f, 0.0030260521452873945f, 0.0036511372309178114f, 0.002159555908292532f, 0.001948230667039752f, 0.002363265259191394f, 0.003037002868950367f, 0.00203146506100893f, 0.002372909337282181f, 0.0022766238544136286f, 0.001739808009006083f, 0.002443819772452116f, 0.0024214654695242643f, 0.002570333192124963f, 0.002901000203564763f, 0.0031948117539286613f, 0.002106191124767065f, 0.0025985324755311012f, 0.002130145439878106f, 0.003092925762757659f, 0.0024692353326827288f, 0.0019130673026666045f, 0.002934239339083433f, 0.0023970373440533876f);
static const ai_layer_format_type conv2d_29_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;


static const ai_i8 conv2d_39_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_39_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_39_pad_before_t_in_0_shape_h_const_u32 = 14;

static const ai_u16 conv2d_39_t_in_0_shape_w_const_u16 = 16;
static const ai_u16 conv2d_39_t_in_0_shape_h_const_u16 = 16;
static const ai_u16 conv2d_39_t_in_0_shape_ch_const_u16 = 24;
static const ai_u16 conv2d_39_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_39_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_39_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_39_t_out_0_fmt_zero_const_s8 = 11;
static const ai_float conv2d_39_t_in_0_fmt_scale_const_f32 = 0.02582520991563797f;
static const ai_float conv2d_39_t_out_0_fmt_scale_const_f32 = 0.05677644908428192f;
static const ai_float conv2d_39_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.007198937702924013f, 0.012514489702880383f, 0.005736705381423235f, 0.009288659319281578f, 0.006704497151076794f, 0.0044247740879654884f, 0.005559231620281935f, 0.007315930910408497f, 0.011860580183565617f, 0.007503144443035126f, 0.005710199940949678f, 0.005970173515379429f, 0.005419822875410318f, 0.006868087220937014f, 0.011118484660983086f, 0.004198806826025248f, 0.00512975687161088f, 0.005417215172201395f, 0.0045848507434129715f, 0.0059927343390882015f, 0.006699476391077042f, 0.006803042721003294f, 0.005372277926653624f, 0.007210044655948877f);
static const ai_u16 conv2d_39_t_out_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_39_t_out_0_shape_h_const_u16 = 7;

static const ai_u16 conv2d_40_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_40_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_40_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_40_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_40_t_in_0_shape_ch_const_u16 = 24;
static const ai_u16 conv2d_40_t_out_0_shape_ch_const_u16 = 48;
static const ai_i8 conv2d_40_t_in_0_fmt_zero_const_s8 = 11;
static const ai_i8 conv2d_40_t_out_0_fmt_zero_const_s8 = 14;
static const ai_float conv2d_40_t_in_0_fmt_scale_const_f32 = 0.05677644908428192f;
static const ai_float conv2d_40_t_out_0_fmt_scale_const_f32 = 0.045860446989536285f;
static const ai_float conv2d_40_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0038326927460730076f, 0.0029692649841308594f, 0.0025747662875801325f, 0.0021854450460523367f, 0.0036975990515202284f, 0.003478600410744548f, 0.003448593197390437f, 0.004157291725277901f, 0.00281010614708066f, 0.00261182663962245f, 0.003357093781232834f, 0.003909614868462086f, 0.003659988520666957f, 0.002252227161079645f, 0.003603970166295767f, 0.0025036921724677086f, 0.002747184131294489f, 0.0028094262816011906f, 0.0030551443342119455f, 0.00291738985106349f, 0.0033754592295736074f, 0.0031601358205080032f, 0.0023338492028415203f, 0.0025725942105054855f, 0.0020744032226502895f, 0.004433991387486458f, 0.002696187701076269f, 0.0021174747962504625f, 0.0028500440530478954f, 0.0029774177819490433f, 0.002374824834987521f, 0.003823045175522566f, 0.0029059131629765034f, 0.0028501548804342747f, 0.0026800520718097687f, 0.0030644144862890244f, 0.0027393721975386143f, 0.0032092563342303038f, 0.0029060353990644217f, 0.003069855272769928f, 0.002344361273571849f, 0.0023740846663713455f, 0.0025214022025465965f, 0.0027193622663617134f, 0.00238347752019763f, 0.0028988171834498644f, 0.0039423368871212006f, 0.002317412756383419f);
static const ai_layer_format_type conv2d_40_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;



static const ai_u16 conv2d_44_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_44_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_44_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_44_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_44_t_in_0_shape_ch_const_u16 = 96;
static const ai_u16 conv2d_44_t_out_0_shape_ch_const_u16 = 24;
static const ai_i8 conv2d_44_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_44_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_44_t_in_0_fmt_scale_const_f32 = 0.020261414349079132f;
static const ai_float conv2d_44_t_out_0_fmt_scale_const_f32 = 0.023730019107460976f;
static const ai_float conv2d_44_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0020975719671696424f, 0.004165072925388813f, 0.003317172871902585f, 0.0034149889834225178f, 0.0021596194710582495f, 0.0037261166144162416f, 0.0030598058365285397f, 0.003355867462232709f, 0.002861330984160304f, 0.0035133271012455225f, 0.0025400244630873203f, 0.00366365653462708f, 0.0031301314011216164f, 0.0037211247254163027f, 0.005070213694125414f, 0.00405812868848443f, 0.0036773672327399254f, 0.0019975020550191402f, 0.004981235135346651f, 0.00200852588750422f, 0.0025959687773138285f, 0.004048974718898535f, 0.00504180695861578f, 0.003371292259544134f);
static const ai_layer_format_type conv2d_44_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;


static const ai_i8 conv2d_54_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_54_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_54_pad_before_t_in_0_shape_h_const_u32 = 7;

static const ai_u16 conv2d_54_t_in_0_shape_w_const_u16 = 9;
static const ai_u16 conv2d_54_t_in_0_shape_h_const_u16 = 9;
static const ai_u16 conv2d_54_t_in_0_shape_ch_const_u16 = 24;
static const ai_u16 conv2d_54_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_54_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_54_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_54_t_out_0_fmt_zero_const_s8 = 0;
static const ai_float conv2d_54_t_in_0_fmt_scale_const_f32 = 0.023730019107460976f;
static const ai_float conv2d_54_t_out_0_fmt_scale_const_f32 = 0.05613403022289276f;
static const ai_float conv2d_54_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.006567941978573799f, 0.008046873845160007f, 0.004717237316071987f, 0.005732097662985325f, 0.004695762414485216f, 0.005165253300219774f, 0.005044328980147839f, 0.004609829746186733f, 0.014577213674783707f, 0.00523788295686245f, 0.005407686810940504f, 0.0090987803414464f, 0.008072257041931152f, 0.004576734267175198f, 0.005808547604829073f, 0.02439873479306698f, 0.008359620347619057f, 0.010799546726047993f, 0.007937368005514145f, 0.005508403293788433f, 0.00779892411082983f, 0.0045017460361123085f, 0.004424715880304575f, 0.005103802774101496f);
static const ai_u16 conv2d_54_t_out_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_54_t_out_0_shape_h_const_u16 = 7;

static const ai_u16 conv2d_55_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_55_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_55_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_55_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_55_t_in_0_shape_ch_const_u16 = 24;
static const ai_u16 conv2d_55_t_out_0_shape_ch_const_u16 = 96;
static const ai_i8 conv2d_55_t_in_0_fmt_zero_const_s8 = 0;
static const ai_i8 conv2d_55_t_out_0_fmt_zero_const_s8 = 4;
static const ai_float conv2d_55_t_in_0_fmt_scale_const_f32 = 0.05613403022289276f;
static const ai_float conv2d_55_t_out_0_fmt_scale_const_f32 = 0.06528100371360779f;
static const ai_float conv2d_55_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.003946176264435053f, 0.0036909389309585094f, 0.005538702942430973f, 0.006346233654767275f, 0.003881418379023671f, 0.004232938401401043f, 0.0034588873386383057f, 0.006423145066946745f, 0.003061461029574275f, 0.00418750150129199f, 0.004988320637494326f, 0.00360542768612504f, 0.005334340967237949f, 0.004381428472697735f, 0.005391278769820929f, 0.0029785207007080317f, 0.00657897861674428f, 0.003576607210561633f, 0.0057021500542759895f, 0.004048361908644438f, 0.0045246840454638f, 0.006409001536667347f, 0.007214375771582127f, 0.0017410848522558808f, 0.004429712891578674f, 0.004705910570919514f, 0.006611560937017202f, 0.005495511461049318f, 0.0034944063518196344f, 0.004098183009773493f, 0.00340076326392591f, 0.003699677065014839f, 0.0019662061240524054f, 0.002990057924762368f, 0.006239817012101412f, 0.002920240629464388f, 0.004073373973369598f, 0.0056893182918429375f, 0.004791701212525368f, 0.004733366426080465f, 0.00544650899246335f, 0.004038532730191946f, 0.005246207118034363f, 0.0050528040155768394f, 0.007144050672650337f, 0.007174579426646233f, 0.004271282814443111f, 0.004413777031004429f, 0.005488003604114056f, 0.005856704898178577f, 0.00704608578234911f, 0.006057867780327797f, 0.005754805635660887f, 0.003202209249138832f, 0.004788864869624376f, 0.007281968370079994f, 0.005286817438900471f, 0.006808614823967218f, 0.0028873025439679623f, 0.0054856869392097f, 0.005425325594842434f, 0.004230669233947992f, 0.0038717114366590977f, 0.006114345043897629f, 0.003348897211253643f, 0.004990086890757084f, 0.0057473015040159225f, 0.004010005854070187f, 0.0035661105066537857f, 0.005050805862993002f, 0.006944796070456505f, 0.004870185162872076f, 0.005134919658303261f, 0.008190941996872425f, 0.007999163120985031f, 0.0046381051652133465f, 0.005026324186474085f, 0.005089803598821163f, 0.007594555150717497f, 0.004346288740634918f, 0.0049515413120388985f, 0.007229686714708805f, 0.006360157392919064f, 0.004472601693123579f, 0.004705281928181648f, 0.006019181571900845f, 0.005661629140377045f, 0.003553204471245408f, 0.006340148393064737f, 0.005161043256521225f, 0.0033821393735706806f, 0.004486458376049995f, 0.004815602675080299f, 0.005561197642236948f, 0.0053978473879396915f, 0.003928055055439472f);
static const ai_layer_format_type conv2d_55_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;


static const ai_u16 conv2d_57_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_57_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_57_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_57_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_57_t_in_0_shape_ch_const_u16 = 96;
static const ai_u16 conv2d_57_t_out_0_shape_ch_const_u16 = 128;
static const ai_i8 conv2d_57_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_57_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_57_t_in_0_fmt_scale_const_f32 = 0.03139876574277878f;
static const ai_float conv2d_57_t_out_0_fmt_scale_const_f32 = 0.04918375983834267f;
static const ai_float conv2d_57_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.004251870326697826f, 0.0032474822364747524f, 0.004121809732168913f, 0.004734874237328768f, 0.004001071210950613f, 0.004591524135321379f, 0.00424042996019125f, 0.004338694736361504f, 0.004335584584623575f, 0.00460081547498703f, 0.00491744140163064f, 0.004157726187258959f, 0.005003040656447411f, 0.003903494216501713f, 0.0028072886634618044f, 0.00339889177121222f, 0.002869108458980918f, 0.0041339220479130745f, 0.0038853443693369627f, 0.0037127058021724224f, 0.0043457262217998505f, 0.004243141505867243f, 0.003885722951963544f, 0.002728695748373866f, 0.003554827533662319f, 0.003784031141549349f, 0.0032137553207576275f, 0.004221340175718069f, 0.0034346773754805326f, 0.0034282051492482424f, 0.004457652568817139f, 0.0034010722301900387f, 0.0051286714151501656f, 0.003920278046280146f, 0.004076372366398573f, 0.0031313153449445963f, 0.005173669662326574f, 0.0038369367830455303f, 0.00481299078091979f, 0.00333273783326149f, 0.004654009360820055f, 0.003045575227588415f, 0.004494937602430582f, 0.0033691197168082f, 0.004453086294233799f, 0.0033151584211736917f, 0.004878061357885599f, 0.003816701238974929f, 0.004675321746617556f, 0.004387896507978439f, 0.0044688014313578606f, 0.003727653529495001f, 0.004654414486140013f, 0.0051653459668159485f, 0.0037628517020493746f, 0.003435417078435421f, 0.004744119476526976f, 0.004876977764070034f, 0.004345899913460016f, 0.00428807083517313f, 0.004776396788656712f, 0.003134995000436902f, 0.004688965156674385f, 0.00356078427284956f, 0.003959620371460915f, 0.004767700098454952f, 0.0036247994285076857f, 0.004728484898805618f, 0.00395071180537343f, 0.0036519302520900965f, 0.00635336572304368f, 0.0035862894728779793f, 0.004939986392855644f, 0.004253171384334564f, 0.0036499209236353636f, 0.003323942655697465f, 0.0043733748607337475f, 0.003327037440612912f, 0.0042435587383806705f, 0.0037736757658421993f, 0.0038494409527629614f, 0.0029451693408191204f, 0.004465884063392878f, 0.003601119387894869f, 0.0038765002973377705f, 0.003540810663253069f, 0.0037223920226097107f, 0.004636705853044987f, 0.004103521816432476f, 0.0036050286144018173f, 0.004265816882252693f, 0.004369325004518032f, 0.003187122056260705f, 0.00350034493021667f, 0.003918723203241825f, 0.004452887456864119f, 0.004641587845981121f, 0.003126372816041112f, 0.003557069692760706f, 0.003624067874625325f, 0.003182121319696307f, 0.004757473710924387f, 0.003625268116593361f, 0.0041467188857495785f, 0.0034828241914510727f, 0.004419422708451748f, 0.005810571834445f, 0.004591299220919609f, 0.003860041731968522f, 0.004182892851531506f, 0.004073338583111763f, 0.0037422229070216417f, 0.004656444303691387f, 0.004218495916575193f, 0.004557777661830187f, 0.00461192149668932f, 0.003410814795643091f, 0.0032175055239349604f, 0.004563737194985151f, 0.004132851492613554f, 0.0038376539014279842f, 0.003952883183956146f, 0.005997150205075741f, 0.005964438430964947f, 0.004842408467084169f, 0.003503801068291068f, 0.003734012134373188f, 0.002802119357511401f);
static const ai_layer_format_type conv2d_57_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;



static const ai_u32 nl_60_t_in_0_shape_ch_prod_const_u32 = 10;
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
    ai_i8* conv2d_0_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 2240);
    ai_i16* conv2d_0_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 22448);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(0, 1, {(stai_ptr) conv2d_0_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_sssa8_ch(conv2d_0_t_in_0_ptr_const_s8, conv2d_0_t_in_0_shape_w_const_u16, conv2d_0_t_in_0_shape_h_const_u16, conv2d_0_t_in_0_shape_ch_const_u16, conv2d_0_t_weight_0_ptr_const_s8, conv2d_0_t_out_0_shape_ch_const_u16, conv2d_0_t_weight_0_shape_w_const_u16, conv2d_0_t_weight_0_shape_h_const_u16, conv2d_0_l_stride_1_const_u16, conv2d_0_l_stride_0_const_u16, conv2d_0_l_pad_W_0_const_s32, conv2d_0_l_pad_H_0_const_s32, conv2d_0_t_weight_1_ptr_const_s32, conv2d_0_t_in_0_fmt_zero_const_s8, conv2d_0_t_out_0_fmt_zero_const_s8, conv2d_0_t_in_0_fmt_scale_const_f32, conv2d_0_t_out_0_fmt_scale_const_f32, conv2d_0_t_weight_0_fmt_scale_const_f32, conv2d_0_l_out_ch_format_const_layer_format_type, conv2d_0_t_out_0_ptr_s8, conv2d_0_t_out_0_shape_w_const_u16, conv2d_0_t_out_0_shape_h_const_u16, 1, 804, conv2d_0_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(0, 1, {(stai_ptr) conv2d_0_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_0 */
  /* LITE_KERNEL_SECTION BEGIN nl_0_nl */
  {
    
  forward_lite_nl_0_nl(net_ctx);
  }
  /* LITE_KERNEL_SECTION END nl_0_nl */
  /* LITE_KERNEL_SECTION BEGIN pool_13 */
  {
    
  forward_lite_pool_13(net_ctx);
  }
  /* LITE_KERNEL_SECTION END pool_13 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_1 */
  {
      const ai_i8* conv2d_1_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 2240);
    const ai_i8* conv2d_1_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[2] + 0);
    const ai_i32* conv2d_1_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[3] + 0);
    ai_i8* conv2d_1_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1904);
    ai_i16* conv2d_1_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 25760);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(1, 1, {(stai_ptr) conv2d_1_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_1_t_in_0_ptr_const_s8, conv2d_1_t_in_0_shape_w_const_u16, conv2d_1_t_in_0_shape_h_const_u16, conv2d_1_l_stride_1_const_u16, conv2d_1_l_stride_0_const_u16, conv2d_1_t_in_0_shape_ch_const_u16, conv2d_1_t_weight_0_ptr_const_s8, conv2d_1_t_out_0_shape_ch_const_u16, conv2d_1_t_weight_1_ptr_const_s32, conv2d_1_t_in_0_fmt_zero_const_s8, conv2d_1_t_out_0_fmt_zero_const_s8, conv2d_1_t_in_0_fmt_scale_const_f32, conv2d_1_t_out_0_fmt_scale_const_f32, conv2d_1_t_weight_0_fmt_scale_const_f32, conv2d_1_l_out_ch_format_const_layer_format_type, conv2d_1_t_out_0_ptr_s8, 1, 216, conv2d_1_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(1, 1, {(stai_ptr) conv2d_1_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_1 */
  /* LITE_KERNEL_SECTION BEGIN transpose_6 */
  {
    
  forward_lite_transpose_6(net_ctx);
  }
  /* LITE_KERNEL_SECTION END transpose_6 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_11_pad_before */
  {
      const ai_ptr conv2d_11_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 11312);
    ai_ptr conv2d_11_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 9920);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(11, 1, {(stai_ptr) conv2d_11_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_11_pad_before_t_in_0_ptr_const_ptr, conv2d_11_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_11_pad_before_v_pad_constant_value_const_s8), conv2d_11_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_11_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(336), (ai_i32)(0), (ai_i32)(720), (ai_i32)(0), (ai_i32)(24));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(11, 1, {(stai_ptr) conv2d_11_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_11_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_11 */
  {
      const ai_i8* conv2d_11_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 9920);
    const ai_i8* conv2d_11_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[4] + 0);
    const ai_i32* conv2d_11_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[5] + 0);
    ai_i8* conv2d_11_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 2352);
    ai_i16* conv2d_11_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 1904);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(11, 1, {(stai_ptr) conv2d_11_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_11_t_in_0_ptr_const_s8, conv2d_11_t_in_0_shape_w_const_u16, conv2d_11_t_in_0_shape_h_const_u16, conv2d_11_t_in_0_shape_ch_const_u16, conv2d_11_t_weight_0_ptr_const_s8, conv2d_11_l_stride_1_const_u16, conv2d_11_l_stride_0_const_u16, conv2d_11_t_weight_1_ptr_const_s32, conv2d_11_t_in_0_fmt_zero_const_s8, conv2d_11_t_out_0_fmt_zero_const_s8, conv2d_11_t_in_0_fmt_scale_const_f32, conv2d_11_t_out_0_fmt_scale_const_f32, conv2d_11_t_weight_0_fmt_scale_const_f32, conv2d_11_t_out_0_ptr_s8, conv2d_11_t_out_0_shape_w_const_u16, conv2d_11_t_out_0_shape_h_const_u16, 0, 445, conv2d_11_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(11, 1, {(stai_ptr) conv2d_11_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_11 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_12 */
  {
      const ai_i8* conv2d_12_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 2352);
    const ai_i8* conv2d_12_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[6] + 0);
    const ai_i32* conv2d_12_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[7] + 0);
    ai_i8* conv2d_12_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 4704);
    ai_i16* conv2d_12_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 1904);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(12, 1, {(stai_ptr) conv2d_12_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_12_t_in_0_ptr_const_s8, conv2d_12_t_in_0_shape_w_const_u16, conv2d_12_t_in_0_shape_h_const_u16, conv2d_12_l_stride_1_const_u16, conv2d_12_l_stride_0_const_u16, conv2d_12_t_in_0_shape_ch_const_u16, conv2d_12_t_weight_0_ptr_const_s8, conv2d_12_t_out_0_shape_ch_const_u16, conv2d_12_t_weight_1_ptr_const_s32, conv2d_12_t_in_0_fmt_zero_const_s8, conv2d_12_t_out_0_fmt_zero_const_s8, conv2d_12_t_in_0_fmt_scale_const_f32, conv2d_12_t_out_0_fmt_scale_const_f32, conv2d_12_t_weight_0_fmt_scale_const_f32, conv2d_12_l_out_ch_format_const_layer_format_type, conv2d_12_t_out_0_ptr_s8, 1, 288, conv2d_12_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(12, 1, {(stai_ptr) conv2d_12_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_12 */
  /* LITE_KERNEL_SECTION BEGIN concat_14 */
  {
    
  forward_lite_concat_14(net_ctx);
  }
  /* LITE_KERNEL_SECTION END concat_14 */
  /* LITE_KERNEL_SECTION BEGIN nl_15 */
  {
    
  forward_lite_nl_15(net_ctx);
  }
  /* LITE_KERNEL_SECTION END nl_15 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_16 */
  {
      const ai_i8* conv2d_16_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 9408);
    const ai_i8* conv2d_16_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[8] + 0);
    const ai_i32* conv2d_16_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[9] + 0);
    ai_i8* conv2d_16_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1904);
    ai_i16* conv2d_16_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 18816);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(16, 1, {(stai_ptr) conv2d_16_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_16_t_in_0_ptr_const_s8, conv2d_16_t_in_0_shape_w_const_u16, conv2d_16_t_in_0_shape_h_const_u16, conv2d_16_l_stride_1_const_u16, conv2d_16_l_stride_0_const_u16, conv2d_16_t_in_0_shape_ch_const_u16, conv2d_16_t_weight_0_ptr_const_s8, conv2d_16_t_out_0_shape_ch_const_u16, conv2d_16_t_weight_1_ptr_const_s32, conv2d_16_t_in_0_fmt_zero_const_s8, conv2d_16_t_out_0_fmt_zero_const_s8, conv2d_16_t_in_0_fmt_scale_const_f32, conv2d_16_t_out_0_fmt_scale_const_f32, conv2d_16_t_weight_0_fmt_scale_const_f32, conv2d_16_l_out_ch_format_const_layer_format_type, conv2d_16_t_out_0_ptr_s8, 1, 312, conv2d_16_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(16, 1, {(stai_ptr) conv2d_16_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_16 */
  /* LITE_KERNEL_SECTION BEGIN transpose_21 */
  {
    
  forward_lite_transpose_21(net_ctx);
  }
  /* LITE_KERNEL_SECTION END transpose_21 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_26_pad_before */
  {
      const ai_ptr conv2d_26_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 7056);
    ai_ptr conv2d_26_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 18816);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(26, 1, {(stai_ptr) conv2d_26_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_26_pad_before_t_in_0_ptr_const_ptr, conv2d_26_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_26_pad_before_v_pad_constant_value_const_s8), conv2d_26_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_26_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(168), (ai_i32)(192), (ai_i32)(192), (ai_i32)(12), (ai_i32)(12));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(26, 1, {(stai_ptr) conv2d_26_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_26_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_26 */
  {
      const ai_i8* conv2d_26_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 18816);
    const ai_i8* conv2d_26_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[10] + 0);
    const ai_i32* conv2d_26_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[11] + 0);
    ai_i8* conv2d_26_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 23624);
    ai_i16* conv2d_26_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 1904);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(26, 1, {(stai_ptr) conv2d_26_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_26_t_in_0_ptr_const_s8, conv2d_26_t_in_0_shape_w_const_u16, conv2d_26_t_in_0_shape_h_const_u16, conv2d_26_t_in_0_shape_ch_const_u16, conv2d_26_t_weight_0_ptr_const_s8, conv2d_26_l_stride_1_const_u16, conv2d_26_l_stride_0_const_u16, conv2d_26_t_weight_1_ptr_const_s32, conv2d_26_t_in_0_fmt_zero_const_s8, conv2d_26_t_out_0_fmt_zero_const_s8, conv2d_26_t_in_0_fmt_scale_const_f32, conv2d_26_t_out_0_fmt_scale_const_f32, conv2d_26_t_weight_0_fmt_scale_const_f32, conv2d_26_t_out_0_ptr_s8, conv2d_26_t_out_0_shape_w_const_u16, conv2d_26_t_out_0_shape_h_const_u16, 0, 445, conv2d_26_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(26, 1, {(stai_ptr) conv2d_26_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_26 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_27 */
  {
      const ai_i8* conv2d_27_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 23624);
    const ai_i8* conv2d_27_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[12] + 0);
    const ai_i32* conv2d_27_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[13] + 0);
    ai_i8* conv2d_27_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_27_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 23096);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(27, 1, {(stai_ptr) conv2d_27_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_27_t_in_0_ptr_const_s8, conv2d_27_t_in_0_shape_w_const_u16, conv2d_27_t_in_0_shape_h_const_u16, conv2d_27_l_stride_1_const_u16, conv2d_27_l_stride_0_const_u16, conv2d_27_t_in_0_shape_ch_const_u16, conv2d_27_t_weight_0_ptr_const_s8, conv2d_27_t_out_0_shape_ch_const_u16, conv2d_27_t_weight_1_ptr_const_s32, conv2d_27_t_in_0_fmt_zero_const_s8, conv2d_27_t_out_0_fmt_zero_const_s8, conv2d_27_t_in_0_fmt_scale_const_f32, conv2d_27_t_out_0_fmt_scale_const_f32, conv2d_27_t_weight_0_fmt_scale_const_f32, conv2d_27_l_out_ch_format_const_layer_format_type, conv2d_27_t_out_0_ptr_s8, 1, 528, conv2d_27_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(27, 1, {(stai_ptr) conv2d_27_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_27 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_28 */
  {
    
  forward_lite_eltwise_28(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_28 */
  /* LITE_KERNEL_SECTION BEGIN nl_28_nl */
  {
    
  forward_lite_nl_28_nl(net_ctx);
  }
  /* LITE_KERNEL_SECTION END nl_28_nl */
  /* LITE_KERNEL_SECTION BEGIN pool_41 */
  {
    
  forward_lite_pool_41(net_ctx);
  }
  /* LITE_KERNEL_SECTION END pool_41 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_29 */
  {
      const ai_i8* conv2d_29_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_29_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[14] + 0);
    const ai_i32* conv2d_29_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[15] + 0);
    ai_i8* conv2d_29_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 12192);
    ai_i16* conv2d_29_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 11760);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(29, 1, {(stai_ptr) conv2d_29_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_29_t_in_0_ptr_const_s8, conv2d_29_t_in_0_shape_w_const_u16, conv2d_29_t_in_0_shape_h_const_u16, conv2d_29_l_stride_1_const_u16, conv2d_29_l_stride_0_const_u16, conv2d_29_t_in_0_shape_ch_const_u16, conv2d_29_t_weight_0_ptr_const_s8, conv2d_29_t_out_0_shape_ch_const_u16, conv2d_29_t_weight_1_ptr_const_s32, conv2d_29_t_in_0_fmt_zero_const_s8, conv2d_29_t_out_0_fmt_zero_const_s8, conv2d_29_t_in_0_fmt_scale_const_f32, conv2d_29_t_out_0_fmt_scale_const_f32, conv2d_29_t_weight_0_fmt_scale_const_f32, conv2d_29_l_out_ch_format_const_layer_format_type, conv2d_29_t_out_0_ptr_s8, 1, 432, conv2d_29_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(29, 1, {(stai_ptr) conv2d_29_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_29 */
  /* LITE_KERNEL_SECTION BEGIN transpose_34 */
  {
    
  forward_lite_transpose_34(net_ctx);
  }
  /* LITE_KERNEL_SECTION END transpose_34 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_39_pad_before */
  {
      const ai_ptr conv2d_39_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 0);
    ai_ptr conv2d_39_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 11760);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(39, 1, {(stai_ptr) conv2d_39_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_39_pad_before_t_in_0_ptr_const_ptr, conv2d_39_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_39_pad_before_v_pad_constant_value_const_s8), conv2d_39_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_39_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(336), (ai_i32)(0), (ai_i32)(768), (ai_i32)(0), (ai_i32)(48));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(39, 1, {(stai_ptr) conv2d_39_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_39_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_39 */
  {
      const ai_i8* conv2d_39_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 11760);
    const ai_i8* conv2d_39_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[16] + 0);
    const ai_i32* conv2d_39_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[17] + 0);
    ai_i8* conv2d_39_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 892);
    ai_i16* conv2d_39_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(39, 1, {(stai_ptr) conv2d_39_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_39_t_in_0_ptr_const_s8, conv2d_39_t_in_0_shape_w_const_u16, conv2d_39_t_in_0_shape_h_const_u16, conv2d_39_t_in_0_shape_ch_const_u16, conv2d_39_t_weight_0_ptr_const_s8, conv2d_39_l_stride_1_const_u16, conv2d_39_l_stride_0_const_u16, conv2d_39_t_weight_1_ptr_const_s32, conv2d_39_t_in_0_fmt_zero_const_s8, conv2d_39_t_out_0_fmt_zero_const_s8, conv2d_39_t_in_0_fmt_scale_const_f32, conv2d_39_t_out_0_fmt_scale_const_f32, conv2d_39_t_weight_0_fmt_scale_const_f32, conv2d_39_t_out_0_ptr_s8, conv2d_39_t_out_0_shape_w_const_u16, conv2d_39_t_out_0_shape_h_const_u16, 0, 889, conv2d_39_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(39, 1, {(stai_ptr) conv2d_39_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_39 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_40 */
  {
      const ai_i8* conv2d_40_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 892);
    const ai_i8* conv2d_40_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[18] + 0);
    const ai_i32* conv2d_40_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[19] + 0);
    ai_i8* conv2d_40_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 2068);
    ai_i16* conv2d_40_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(40, 1, {(stai_ptr) conv2d_40_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_40_t_in_0_ptr_const_s8, conv2d_40_t_in_0_shape_w_const_u16, conv2d_40_t_in_0_shape_h_const_u16, conv2d_40_l_stride_1_const_u16, conv2d_40_l_stride_0_const_u16, conv2d_40_t_in_0_shape_ch_const_u16, conv2d_40_t_weight_0_ptr_const_s8, conv2d_40_t_out_0_shape_ch_const_u16, conv2d_40_t_weight_1_ptr_const_s32, conv2d_40_t_in_0_fmt_zero_const_s8, conv2d_40_t_out_0_fmt_zero_const_s8, conv2d_40_t_in_0_fmt_scale_const_f32, conv2d_40_t_out_0_fmt_scale_const_f32, conv2d_40_t_weight_0_fmt_scale_const_f32, conv2d_40_l_out_ch_format_const_layer_format_type, conv2d_40_t_out_0_ptr_s8, 1, 576, conv2d_40_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(40, 1, {(stai_ptr) conv2d_40_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_40 */
  /* LITE_KERNEL_SECTION BEGIN concat_42 */
  {
    
  forward_lite_concat_42(net_ctx);
  }
  /* LITE_KERNEL_SECTION END concat_42 */
  /* LITE_KERNEL_SECTION BEGIN nl_43 */
  {
    
  forward_lite_nl_43(net_ctx);
  }
  /* LITE_KERNEL_SECTION END nl_43 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_44 */
  {
      const ai_i8* conv2d_44_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 9124);
    const ai_i8* conv2d_44_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[20] + 0);
    const ai_i32* conv2d_44_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[21] + 0);
    ai_i8* conv2d_44_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 624);
    ai_i16* conv2d_44_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(44, 1, {(stai_ptr) conv2d_44_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_44_t_in_0_ptr_const_s8, conv2d_44_t_in_0_shape_w_const_u16, conv2d_44_t_in_0_shape_h_const_u16, conv2d_44_l_stride_1_const_u16, conv2d_44_l_stride_0_const_u16, conv2d_44_t_in_0_shape_ch_const_u16, conv2d_44_t_weight_0_ptr_const_s8, conv2d_44_t_out_0_shape_ch_const_u16, conv2d_44_t_weight_1_ptr_const_s32, conv2d_44_t_in_0_fmt_zero_const_s8, conv2d_44_t_out_0_fmt_zero_const_s8, conv2d_44_t_in_0_fmt_scale_const_f32, conv2d_44_t_out_0_fmt_scale_const_f32, conv2d_44_t_weight_0_fmt_scale_const_f32, conv2d_44_l_out_ch_format_const_layer_format_type, conv2d_44_t_out_0_ptr_s8, 1, 624, conv2d_44_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(44, 1, {(stai_ptr) conv2d_44_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_44 */
  /* LITE_KERNEL_SECTION BEGIN transpose_49 */
  {
    
  forward_lite_transpose_49(net_ctx);
  }
  /* LITE_KERNEL_SECTION END transpose_49 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_54_pad_before */
  {
      const ai_ptr conv2d_54_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 1800);
    ai_ptr conv2d_54_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 2976);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(54, 1, {(stai_ptr) conv2d_54_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_54_pad_before_t_in_0_ptr_const_ptr, conv2d_54_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_54_pad_before_v_pad_constant_value_const_s8), conv2d_54_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_54_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(168), (ai_i32)(216), (ai_i32)(216), (ai_i32)(24), (ai_i32)(24));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(54, 1, {(stai_ptr) conv2d_54_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_54_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_54 */
  {
      const ai_i8* conv2d_54_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 2976);
    const ai_i8* conv2d_54_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[22] + 0);
    const ai_i32* conv2d_54_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[23] + 0);
    ai_i8* conv2d_54_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 892);
    ai_i16* conv2d_54_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(54, 1, {(stai_ptr) conv2d_54_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_54_t_in_0_ptr_const_s8, conv2d_54_t_in_0_shape_w_const_u16, conv2d_54_t_in_0_shape_h_const_u16, conv2d_54_t_in_0_shape_ch_const_u16, conv2d_54_t_weight_0_ptr_const_s8, conv2d_54_l_stride_1_const_u16, conv2d_54_l_stride_0_const_u16, conv2d_54_t_weight_1_ptr_const_s32, conv2d_54_t_in_0_fmt_zero_const_s8, conv2d_54_t_out_0_fmt_zero_const_s8, conv2d_54_t_in_0_fmt_scale_const_f32, conv2d_54_t_out_0_fmt_scale_const_f32, conv2d_54_t_weight_0_fmt_scale_const_f32, conv2d_54_t_out_0_ptr_s8, conv2d_54_t_out_0_shape_w_const_u16, conv2d_54_t_out_0_shape_h_const_u16, 0, 889, conv2d_54_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(54, 1, {(stai_ptr) conv2d_54_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_54 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_55 */
  {
      const ai_i8* conv2d_55_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 892);
    const ai_i8* conv2d_55_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[24] + 0);
    const ai_i32* conv2d_55_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[25] + 0);
    ai_i8* conv2d_55_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 3124);
    ai_i16* conv2d_55_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 2068);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(55, 1, {(stai_ptr) conv2d_55_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_55_t_in_0_ptr_const_s8, conv2d_55_t_in_0_shape_w_const_u16, conv2d_55_t_in_0_shape_h_const_u16, conv2d_55_l_stride_1_const_u16, conv2d_55_l_stride_0_const_u16, conv2d_55_t_in_0_shape_ch_const_u16, conv2d_55_t_weight_0_ptr_const_s8, conv2d_55_t_out_0_shape_ch_const_u16, conv2d_55_t_weight_1_ptr_const_s32, conv2d_55_t_in_0_fmt_zero_const_s8, conv2d_55_t_out_0_fmt_zero_const_s8, conv2d_55_t_in_0_fmt_scale_const_f32, conv2d_55_t_out_0_fmt_scale_const_f32, conv2d_55_t_weight_0_fmt_scale_const_f32, conv2d_55_l_out_ch_format_const_layer_format_type, conv2d_55_t_out_0_ptr_s8, 1, 1056, conv2d_55_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(55, 1, {(stai_ptr) conv2d_55_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_55 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_56 */
  {
    
  forward_lite_eltwise_56(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_56 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_57 */
  {
      const ai_i8* conv2d_57_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 13828);
    const ai_i8* conv2d_57_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[26] + 0);
    const ai_i32* conv2d_57_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[27] + 0);
    ai_i8* conv2d_57_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1664);
    ai_i16* conv2d_57_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(57, 1, {(stai_ptr) conv2d_57_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_57_t_in_0_ptr_const_s8, conv2d_57_t_in_0_shape_w_const_u16, conv2d_57_t_in_0_shape_h_const_u16, conv2d_57_l_stride_1_const_u16, conv2d_57_l_stride_0_const_u16, conv2d_57_t_in_0_shape_ch_const_u16, conv2d_57_t_weight_0_ptr_const_s8, conv2d_57_t_out_0_shape_ch_const_u16, conv2d_57_t_weight_1_ptr_const_s32, conv2d_57_t_in_0_fmt_zero_const_s8, conv2d_57_t_out_0_fmt_zero_const_s8, conv2d_57_t_in_0_fmt_scale_const_f32, conv2d_57_t_out_0_fmt_scale_const_f32, conv2d_57_t_weight_0_fmt_scale_const_f32, conv2d_57_l_out_ch_format_const_layer_format_type, conv2d_57_t_out_0_ptr_s8, 1, 1664, conv2d_57_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(57, 1, {(stai_ptr) conv2d_57_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_57 */
  /* LITE_KERNEL_SECTION BEGIN pool_58 */
  {
    
  forward_lite_pool_58(net_ctx);
  }
  /* LITE_KERNEL_SECTION END pool_58 */
  /* LITE_KERNEL_SECTION BEGIN gemm_59 */
  {
    
  forward_lite_gemm_59(net_ctx);
  }
  /* LITE_KERNEL_SECTION END gemm_59 */
  /* LITE_KERNEL_SECTION BEGIN nl_60 */
  {
      ai_i8* nl_60_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_outputs[0] + 0);
    const ai_i8* nl_60_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 484);
    ai_i32* nl_60_t_scratch_0_ptr_s32 = (ai_i32*)(net_ctx->_activations[0] + 496);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(60, 1, {(stai_ptr) nl_60_t_in_0_ptr_const_s8});
    
  forward_lite_nl_softmax_is8os8(nl_60_t_out_0_ptr_s8, nl_60_t_in_0_ptr_const_s8, nl_60_t_in_0_shape_ch_prod_const_u32, 1, 10, 1371922944, 24, -124, nl_60_t_scratch_0_ptr_s32);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(60, 1, {(stai_ptr) nl_60_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END nl_60 */
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
  net_ctx->_inputs[0] = activations[0] + 21664;

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

