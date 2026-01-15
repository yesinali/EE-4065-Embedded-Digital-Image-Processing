/**
  ******************************************************************************
  * @file    network.c
  * @author  AST Embedded Analytics Research Platform
  * @date    2026-01-15T18:41:25+0000
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
#define _STAI_NETWORK_MODEL_SIGNATURE     "0x7bfd705d85e1e343fa16d5069a8a6c68"
#define _STAI_NETWORK_DATETIME            "2026-01-15T18:41:25+0000"
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
      STAI_DECLARE_ARRAY(int32_t, 1, 35104),
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
      STAI_DECLARE_ARRAY(int32_t, 1, 2304),
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
      STAI_DECLARE_ARRAY(int32_t, 1, 2304),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_6_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_6_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 64),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_7_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_7_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 2304),
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
      STAI_DECLARE_ARRAY(int32_t, 1, 2304),
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
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_12_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_12_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 128),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_13_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_13_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 4608),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_14_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_14_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 128),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_15_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_15_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 9216),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_16_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_16_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 128),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_17_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_17_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 9216),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_18_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_18_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 128),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_19_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_19_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 9216),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_20_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_20_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 128),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_21_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_21_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 18432),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_22_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_22_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 256),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_23_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_23_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 36864),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_24_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_24_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 256),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_25_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_25_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 2048),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_26_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_26_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 256),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_27_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_27_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 36864),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_28_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_28_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 256),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_29_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_29_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 36864),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_30_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_30_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 256),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_31_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_31_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 640),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_32_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_32_SIZE_BYTES,
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
      (stai_ptr)g_network_conv2d_0_weights_array,(stai_ptr)g_network_conv2d_0_bias_array,(stai_ptr)g_network_conv2d_1_weights_array,(stai_ptr)g_network_conv2d_1_bias_array,(stai_ptr)g_network_conv2d_2_weights_array,(stai_ptr)g_network_conv2d_2_bias_array,(stai_ptr)g_network_conv2d_4_weights_array,(stai_ptr)g_network_conv2d_4_bias_array,(stai_ptr)g_network_conv2d_5_weights_array,(stai_ptr)g_network_conv2d_5_bias_array,(stai_ptr)g_network_conv2d_9_weights_array,(stai_ptr)g_network_conv2d_9_bias_array,(stai_ptr)g_network_conv2d_7_weights_array,(stai_ptr)g_network_conv2d_7_bias_array,(stai_ptr)g_network_conv2d_8_weights_array,(stai_ptr)g_network_conv2d_8_bias_array,(stai_ptr)g_network_conv2d_11_weights_array,(stai_ptr)g_network_conv2d_11_bias_array,(stai_ptr)g_network_conv2d_12_weights_array,(stai_ptr)g_network_conv2d_12_bias_array,(stai_ptr)g_network_conv2d_14_weights_array,(stai_ptr)g_network_conv2d_14_bias_array,(stai_ptr)g_network_conv2d_15_weights_array,(stai_ptr)g_network_conv2d_15_bias_array,(stai_ptr)g_network_conv2d_16_weights_array,(stai_ptr)g_network_conv2d_16_bias_array,(stai_ptr)g_network_conv2d_18_weights_array,(stai_ptr)g_network_conv2d_18_bias_array,(stai_ptr)g_network_conv2d_19_weights_array,(stai_ptr)g_network_conv2d_19_bias_array,(stai_ptr)g_network_gemm_22_weights_array,(stai_ptr)g_network_gemm_22_bias_array
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
    AI_PACK_INTQ_SCALE(0.022536076605319977f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #1 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_2_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.08001790195703506f),
    AI_PACK_INTQ_ZP(-24)))

/* Int quant #2 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_3_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.04742404446005821f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #3 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_5_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.09247681498527527f),
    AI_PACK_INTQ_ZP(-42)))

/* Int quant #4 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_6_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.06131451204419136f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #5 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_9_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0602550134062767f),
    AI_PACK_INTQ_ZP(-5)))

/* Int quant #6 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_8_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0719042494893074f),
    AI_PACK_INTQ_ZP(-23)))

/* Int quant #7 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_10_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.04220288619399071f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #8 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_12_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.08221554756164551f),
    AI_PACK_INTQ_ZP(-32)))

/* Int quant #9 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_13_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0511508472263813f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #10 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_15_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.06859904527664185f),
    AI_PACK_INTQ_ZP(-25)))

/* Int quant #11 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_16_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.05399082973599434f),
    AI_PACK_INTQ_ZP(12)))

/* Int quant #12 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_17_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.040968943387269974f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #13 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_19_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.08927126228809357f),
    AI_PACK_INTQ_ZP(-44)))

/* Int quant #14 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_20_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.05983635410666466f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #15 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(pool_21_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.018581321462988853f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #16 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_22_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.11622951924800873f),
    AI_PACK_INTQ_ZP(-45)))

/* Int quant #17 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_22_weights_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 10,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0041684359312057495f, 0.0038238822016865015f, 0.003978998865932226f, 0.003853512229397893f, 0.003760482417419553f, 0.004090399015694857f, 0.004152452573180199f, 0.003885186742991209f, 0.0036740824580192566f, 0.003492241259664297f),
    AI_PACK_INTQ_ZP(0, 0, 0, 0, 0, 0, 0, 0, 0, 0)))



/* Array#0 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_0_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 12544, AI_STATIC)

/* Array#1 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_2_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 12544, AI_STATIC)

/* Array#2 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_3_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 12544, AI_STATIC)

/* Array#3 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_5_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 12544, AI_STATIC)

/* Array#4 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_6_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 12544, AI_STATIC)

/* Array#5 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_9_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 6272, AI_STATIC)

/* Array#6 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_8_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 6272, AI_STATIC)

/* Array#7 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_10_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 6272, AI_STATIC)

/* Array#8 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_12_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 6272, AI_STATIC)

/* Array#9 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_13_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 6272, AI_STATIC)

/* Array#10 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_15_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 3136, AI_STATIC)

/* Array#11 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_16_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 3136, AI_STATIC)

/* Array#12 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_17_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 3136, AI_STATIC)

/* Array#13 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_19_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 3136, AI_STATIC)

/* Array#14 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_20_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 3136, AI_STATIC)

/* Array#15 */
AI_ARRAY_OBJ_DECLARE(
  pool_21_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 64, AI_STATIC)

/* Array#16 */
AI_ARRAY_OBJ_DECLARE(
  gemm_22_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 10, AI_STATIC)

/* Array#17 */
AI_ARRAY_OBJ_DECLARE(
  gemm_22_weights_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 640, AI_STATIC)

/* Array#18 */
AI_ARRAY_OBJ_DECLARE(
  gemm_22_bias_array, AI_ARRAY_FORMAT_S32,
  NULL, NULL, 10, AI_STATIC)

/* Array#19 */
AI_ARRAY_OBJ_DECLARE(
  gemm_22_scratch0_array, AI_ARRAY_FORMAT_S16,
  NULL, NULL, 114, AI_STATIC)



/* Tensor #0 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_0_output, AI_STATIC,
  1, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 28, 28), AI_STRIDE_INIT(4, 1, 1, 16, 448),
  1, &conv2d_0_output_array, &conv2d_0_output_array_intq)

/* Tensor #1 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_2_output, AI_STATIC,
  44, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 28, 28), AI_STRIDE_INIT(4, 1, 1, 16, 448),
  1, &conv2d_2_output_array, &conv2d_2_output_array_intq)

/* Tensor #2 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_3_output, AI_STATIC,
  75, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 28, 28), AI_STRIDE_INIT(4, 1, 1, 16, 448),
  1, &eltwise_3_output_array, &eltwise_3_output_array_intq)

/* Tensor #3 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_5_output, AI_STATIC,
  54, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 28, 28), AI_STRIDE_INIT(4, 1, 1, 16, 448),
  1, &conv2d_5_output_array, &conv2d_5_output_array_intq)

/* Tensor #4 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_6_output, AI_STATIC,
  76, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 28, 28), AI_STRIDE_INIT(4, 1, 1, 16, 448),
  1, &eltwise_6_output_array, &eltwise_6_output_array_intq)

/* Tensor #5 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_8_output, AI_STATIC,
  63, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 1, 1, 32, 448),
  1, &conv2d_8_output_array, &conv2d_8_output_array_intq)

/* Tensor #6 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_9_output, AI_STATIC,
  68, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 1, 1, 32, 448),
  1, &conv2d_9_output_array, &conv2d_9_output_array_intq)

/* Tensor #7 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_10_output, AI_STATIC,
  71, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 1, 1, 32, 448),
  1, &eltwise_10_output_array, &eltwise_10_output_array_intq)

/* Tensor #8 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_12_output, AI_STATIC,
  10, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 1, 1, 32, 448),
  1, &conv2d_12_output_array, &conv2d_12_output_array_intq)

/* Tensor #9 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_13_output, AI_STATIC,
  72, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 1, 1, 32, 448),
  1, &eltwise_13_output_array, &eltwise_13_output_array_intq)

/* Tensor #10 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_15_output, AI_STATIC,
  20, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 1, 1, 64, 448),
  1, &conv2d_15_output_array, &conv2d_15_output_array_intq)

/* Tensor #11 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_16_output, AI_STATIC,
  25, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 1, 1, 64, 448),
  1, &conv2d_16_output_array, &conv2d_16_output_array_intq)

/* Tensor #12 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_17_output, AI_STATIC,
  73, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 1, 1, 64, 448),
  1, &eltwise_17_output_array, &eltwise_17_output_array_intq)

/* Tensor #13 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_19_output, AI_STATIC,
  34, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 1, 1, 64, 448),
  1, &conv2d_19_output_array, &conv2d_19_output_array_intq)

/* Tensor #14 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_20_output, AI_STATIC,
  74, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 1, 1, 64, 448),
  1, &eltwise_20_output_array, &eltwise_20_output_array_intq)

/* Tensor #15 */
AI_TENSOR_OBJ_DECLARE(
  pool_21_output, AI_STATIC,
  83, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 1, 1), AI_STRIDE_INIT(4, 1, 1, 64, 64),
  1, &pool_21_output_array, &pool_21_output_array_intq)

/* Tensor #16 */
AI_TENSOR_OBJ_DECLARE(
  gemm_22_bias, AI_STATIC,
  77, 0x0,
  AI_SHAPE_INIT(4, 1, 10, 1, 1), AI_STRIDE_INIT(4, 4, 4, 40, 40),
  1, &gemm_22_bias_array, NULL)

/* Tensor #17 */
AI_TENSOR_OBJ_DECLARE(
  gemm_22_output, AI_STATIC,
  78, 0x1,
  AI_SHAPE_INIT(4, 1, 10, 1, 1), AI_STRIDE_INIT(4, 1, 1, 10, 10),
  1, &gemm_22_output_array, &gemm_22_output_array_intq)

/* Tensor #18 */
AI_TENSOR_OBJ_DECLARE(
  gemm_22_scratch0, AI_STATIC,
  79, 0x0,
  AI_SHAPE_INIT(4, 1, 114, 1, 1), AI_STRIDE_INIT(4, 2, 2, 228, 228),
  1, &gemm_22_scratch0_array, NULL)

/* Tensor #19 */
AI_TENSOR_OBJ_DECLARE(
  gemm_22_weights, AI_STATIC,
  80, 0x1,
  AI_SHAPE_INIT(4, 64, 10, 1, 1), AI_STRIDE_INIT(4, 1, 64, 640, 640),
  1, &gemm_22_weights_array, &gemm_22_weights_array_intq)


AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_3_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_2_output, &conv2d_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_3_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  eltwise_3_layer, 3,
  ELTWISE_INTEGER_TYPE, 0x0, NULL,
  eltwise_integer, forward_eltwise_integer_INT8,
  &eltwise_3_chain,
  NULL, &eltwise_3_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_INT8, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_6_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_5_output, &eltwise_3_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_6_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  eltwise_6_layer, 6,
  ELTWISE_INTEGER_TYPE, 0x0, NULL,
  eltwise_integer, forward_eltwise_integer_INT8,
  &eltwise_6_chain,
  NULL, &eltwise_6_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_INT8, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_10_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_8_output, &conv2d_9_output),
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
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_INT8, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_13_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_12_output, &eltwise_10_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_13_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  eltwise_13_layer, 13,
  ELTWISE_INTEGER_TYPE, 0x0, NULL,
  eltwise_integer, forward_eltwise_integer_INT8,
  &eltwise_13_chain,
  NULL, &eltwise_13_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_INT8, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_17_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_15_output, &conv2d_16_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_17_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  eltwise_17_layer, 17,
  ELTWISE_INTEGER_TYPE, 0x0, NULL,
  eltwise_integer, forward_eltwise_integer_INT8,
  &eltwise_17_chain,
  NULL, &eltwise_17_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_INT8, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_20_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_19_output, &eltwise_17_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_20_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  eltwise_20_layer, 20,
  ELTWISE_INTEGER_TYPE, 0x0, NULL,
  eltwise_integer, forward_eltwise_integer_INT8,
  &eltwise_20_chain,
  NULL, &eltwise_20_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_INT8, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  pool_21_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_20_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &pool_21_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  pool_21_layer, 21,
  POOL_TYPE, 0x0, NULL,
  pool, forward_ap_integer_INT8,
  &pool_21_chain,
  NULL, &pool_21_layer, AI_STATIC, 
  .pool_size = AI_SHAPE_2D_INIT(7, 7), 
  .pool_stride = AI_SHAPE_2D_INIT(7, 7), 
  .pool_pad = AI_SHAPE_INIT(4, 0, 0, 0, 0), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  gemm_22_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &pool_21_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gemm_22_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &gemm_22_weights, &gemm_22_bias),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gemm_22_scratch0)
)

AI_LAYER_OBJ_DECLARE(
  gemm_22_layer, 22,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense_integer_SSSA_ch,
  &gemm_22_chain,
  NULL, &gemm_22_layer, AI_STATIC, 
)
/**  Hybrid layers declarations section  *************************************/
void forward_lite_eltwise_3(_stai_network_context* net_ctx)
{
  conv2d_2_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  conv2d_2_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  conv2d_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 17152);
  conv2d_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 17152);
  eltwise_3_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  eltwise_3_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(3, 2, { conv2d_2_output.data->data,conv2d_0_output.data->data});
  forward_eltwise_integer_INT8(&eltwise_3_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(3, 1, { eltwise_3_output.data->data});
}
void forward_lite_eltwise_6(_stai_network_context* net_ctx)
{
  conv2d_5_output_array.data = AI_PTR(net_ctx->_activations[0] + 17952);
  conv2d_5_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 17952);
  eltwise_3_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  eltwise_3_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  eltwise_6_output_array.data = AI_PTR(net_ctx->_activations[0] + 17952);
  eltwise_6_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 17952);
  _STAI_NETWORK_EVENT_NODE_START_CB(6, 2, { conv2d_5_output.data->data,eltwise_3_output.data->data});
  forward_eltwise_integer_INT8(&eltwise_6_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(6, 1, { eltwise_6_output.data->data});
}
void forward_lite_eltwise_10(_stai_network_context* net_ctx)
{
  conv2d_8_output_array.data = AI_PTR(net_ctx->_activations[0] + 22720);
  conv2d_8_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 22720);
  conv2d_9_output_array.data = AI_PTR(net_ctx->_activations[0] + 1536);
  conv2d_9_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1536);
  eltwise_10_output_array.data = AI_PTR(net_ctx->_activations[0] + 7808);
  eltwise_10_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 7808);
  _STAI_NETWORK_EVENT_NODE_START_CB(10, 2, { conv2d_8_output.data->data,conv2d_9_output.data->data});
  forward_eltwise_integer_INT8(&eltwise_10_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(10, 1, { eltwise_10_output.data->data});
}
void forward_lite_eltwise_13(_stai_network_context* net_ctx)
{
  conv2d_12_output_array.data = AI_PTR(net_ctx->_activations[0] + 22272);
  conv2d_12_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 22272);
  eltwise_10_output_array.data = AI_PTR(net_ctx->_activations[0] + 7808);
  eltwise_10_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 7808);
  eltwise_13_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  eltwise_13_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(13, 2, { conv2d_12_output.data->data,eltwise_10_output.data->data});
  forward_eltwise_integer_INT8(&eltwise_13_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(13, 1, { eltwise_13_output.data->data});
}
void forward_lite_eltwise_17(_stai_network_context* net_ctx)
{
  conv2d_15_output_array.data = AI_PTR(net_ctx->_activations[0] + 19776);
  conv2d_15_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 19776);
  conv2d_16_output_array.data = AI_PTR(net_ctx->_activations[0] + 7040);
  conv2d_16_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 7040);
  eltwise_17_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  eltwise_17_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(17, 2, { conv2d_15_output.data->data,conv2d_16_output.data->data});
  forward_eltwise_integer_INT8(&eltwise_17_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(17, 1, { eltwise_17_output.data->data});
}
void forward_lite_eltwise_20(_stai_network_context* net_ctx)
{
  conv2d_19_output_array.data = AI_PTR(net_ctx->_activations[0] + 16640);
  conv2d_19_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16640);
  eltwise_17_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  eltwise_17_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  eltwise_20_output_array.data = AI_PTR(net_ctx->_activations[0] + 3136);
  eltwise_20_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 3136);
  _STAI_NETWORK_EVENT_NODE_START_CB(20, 2, { conv2d_19_output.data->data,eltwise_17_output.data->data});
  forward_eltwise_integer_INT8(&eltwise_20_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(20, 1, { eltwise_20_output.data->data});
}
void forward_lite_pool_21(_stai_network_context* net_ctx)
{
  eltwise_20_output_array.data = AI_PTR(net_ctx->_activations[0] + 3136);
  eltwise_20_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 3136);
  pool_21_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  pool_21_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(21, 1, { eltwise_20_output.data->data});
  forward_ap_integer_INT8(&pool_21_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(21, 1, { pool_21_output.data->data});
}
void forward_lite_gemm_22(_stai_network_context* net_ctx)
{
  pool_21_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  pool_21_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  gemm_22_weights_array.data = AI_PTR(net_ctx->_weights[30] + 0);
  gemm_22_weights_array.data_start = AI_PTR(net_ctx->_weights[30] + 0);
  gemm_22_bias_array.data = AI_PTR(net_ctx->_weights[31] + 0);
  gemm_22_bias_array.data_start = AI_PTR(net_ctx->_weights[31] + 0);
  gemm_22_scratch0_array.data = AI_PTR(net_ctx->_activations[0] + 64);
  gemm_22_scratch0_array.data_start = AI_PTR(net_ctx->_activations[0] + 64);
  gemm_22_output_array.data = AI_PTR(net_ctx->_activations[0] + 292);
  gemm_22_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 292);
  _STAI_NETWORK_EVENT_NODE_START_CB(22, 1, { pool_21_output.data->data});
  forward_dense_integer_SSSA_ch(&gemm_22_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(22, 1, { gemm_22_output.data->data});
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
static const ai_float conv2d_0_t_out_0_fmt_scale_const_f32 = 0.022536076605319977f;
static const ai_float conv2d_0_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.02361239679157734f, 0.024488380178809166f, 0.012214486487209797f, 0.020738523453474045f, 0.024939248338341713f, 0.013386322185397148f, 0.019034424796700478f, 0.014105631969869137f, 0.024858100339770317f, 0.01725928857922554f, 0.013160302303731441f, 0.012501816265285015f, 0.019919678568840027f, 0.013091085478663445f, 0.011129847727715969f, 0.014046605676412582f);
static const ai_layer_format_type conv2d_0_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_0_t_out_0_shape_w_const_u16 = 28;
static const ai_u16 conv2d_0_t_out_0_shape_h_const_u16 = 28;

static const ai_i8 conv2d_1_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_1_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_1_pad_before_t_in_0_shape_h_const_u32 = 28;

static const ai_u16 conv2d_1_t_in_0_shape_w_const_u16 = 30;
static const ai_u16 conv2d_1_t_in_0_shape_h_const_u16 = 30;
static const ai_u16 conv2d_1_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_1_t_out_0_shape_ch_const_u16 = 16;
static const ai_i8 conv2d_1_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_1_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_1_t_in_0_fmt_scale_const_f32 = 0.022536076605319977f;
static const ai_float conv2d_1_t_out_0_fmt_scale_const_f32 = 0.031599391251802444f;
static const ai_float conv2d_1_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.001897704554721713f, 0.0024696788750588894f, 0.0016271682688966393f, 0.0027150253299623728f, 0.002809860510751605f, 0.0023080548271536827f, 0.0014358613407239318f, 0.002086521126329899f, 0.0015692374436184764f, 0.003379842499271035f, 0.002048700349405408f, 0.001741862972266972f, 0.0012049569049850106f, 0.0019266708986833692f, 0.0026609678752720356f, 0.0030953853856772184f);
static const ai_layer_format_type conv2d_1_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_1_t_out_0_shape_w_const_u16 = 28;
static const ai_u16 conv2d_1_t_out_0_shape_h_const_u16 = 28;

static const ai_i8 conv2d_2_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_2_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_2_pad_before_t_in_0_shape_h_const_u32 = 28;

static const ai_u16 conv2d_2_t_in_0_shape_w_const_u16 = 30;
static const ai_u16 conv2d_2_t_in_0_shape_h_const_u16 = 30;
static const ai_u16 conv2d_2_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_2_t_out_0_shape_ch_const_u16 = 16;
static const ai_i8 conv2d_2_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_2_t_out_0_fmt_zero_const_s8 = -24;
static const ai_float conv2d_2_t_in_0_fmt_scale_const_f32 = 0.031599391251802444f;
static const ai_float conv2d_2_t_out_0_fmt_scale_const_f32 = 0.08001790195703506f;
static const ai_float conv2d_2_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0025061292108148336f, 0.0026970976032316685f, 0.0018857162212952971f, 0.0013925008242949843f, 0.00210802280344069f, 0.001983293564990163f, 0.0015966423088684678f, 0.0028610306326299906f, 0.0028999221976846457f, 0.0020616704132407904f, 0.0018574767746031284f, 0.0018422809662297368f, 0.0013702989090234041f, 0.0028750284109264612f, 0.0017097349045798182f, 0.0025168131105601788f);
static const ai_layer_format_type conv2d_2_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_2_t_out_0_shape_w_const_u16 = 28;
static const ai_u16 conv2d_2_t_out_0_shape_h_const_u16 = 28;


static const ai_i8 conv2d_4_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_4_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_4_pad_before_t_in_0_shape_h_const_u32 = 28;

static const ai_u16 conv2d_4_t_in_0_shape_w_const_u16 = 30;
static const ai_u16 conv2d_4_t_in_0_shape_h_const_u16 = 30;
static const ai_u16 conv2d_4_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_4_t_out_0_shape_ch_const_u16 = 16;
static const ai_i8 conv2d_4_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_4_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_4_t_in_0_fmt_scale_const_f32 = 0.04742404446005821f;
static const ai_float conv2d_4_t_out_0_fmt_scale_const_f32 = 0.03659798204898834f;
static const ai_float conv2d_4_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0007586728315800428f, 0.0011139254784211516f, 0.001271884306333959f, 0.0014098652172833681f, 0.0008858674555085599f, 0.0012371897464618087f, 0.0013097982155159116f, 0.0012378091923892498f, 0.000996492337435484f, 0.0012931146193295717f, 0.0007919100462459028f, 0.001114408951252699f, 0.0008477179217152297f, 0.0014040098758414388f, 0.0012855257373303175f, 0.001014751149341464f);
static const ai_layer_format_type conv2d_4_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_4_t_out_0_shape_w_const_u16 = 28;
static const ai_u16 conv2d_4_t_out_0_shape_h_const_u16 = 28;

static const ai_i8 conv2d_5_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_5_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_5_pad_before_t_in_0_shape_h_const_u32 = 28;

static const ai_u16 conv2d_5_t_in_0_shape_w_const_u16 = 30;
static const ai_u16 conv2d_5_t_in_0_shape_h_const_u16 = 30;
static const ai_u16 conv2d_5_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_5_t_out_0_shape_ch_const_u16 = 16;
static const ai_i8 conv2d_5_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_5_t_out_0_fmt_zero_const_s8 = -42;
static const ai_float conv2d_5_t_in_0_fmt_scale_const_f32 = 0.03659798204898834f;
static const ai_float conv2d_5_t_out_0_fmt_scale_const_f32 = 0.09247681498527527f;
static const ai_float conv2d_5_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0015442491276189685f, 0.0017071538604795933f, 0.0016568619757890701f, 0.0022507994435727596f, 0.0021943342871963978f, 0.001995423110201955f, 0.002217001048848033f, 0.0010286574251949787f, 0.0014880858361721039f, 0.0010777667630463839f, 0.0017838795902207494f, 0.001625376520678401f, 0.00132322171702981f, 0.002187455305829644f, 0.0012500587617978454f, 0.0013795265695080161f);
static const ai_layer_format_type conv2d_5_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_5_t_out_0_shape_w_const_u16 = 28;
static const ai_u16 conv2d_5_t_out_0_shape_h_const_u16 = 28;


static const ai_u16 conv2d_9_t_in_0_shape_w_const_u16 = 28;
static const ai_u16 conv2d_9_t_in_0_shape_h_const_u16 = 28;
static const ai_u16 conv2d_9_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_9_t_out_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_9_t_weight_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_9_t_weight_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_9_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_9_l_stride_0_const_u16 = 2;
static const ai_i32 conv2d_9_l_pad_W_0_const_s32 = 0;
static const ai_i32 conv2d_9_l_pad_H_0_const_s32 = 0;
static const ai_i8 conv2d_9_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_9_t_out_0_fmt_zero_const_s8 = -5;
static const ai_float conv2d_9_t_in_0_fmt_scale_const_f32 = 0.06131451204419136f;
static const ai_float conv2d_9_t_out_0_fmt_scale_const_f32 = 0.0602550134062767f;
static const ai_float conv2d_9_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.002376520773395896f, 0.0018161031184718013f, 0.0014619337162002921f, 0.0023746842052787542f, 0.002543338341638446f, 0.0018875139066949487f, 0.0014225095510482788f, 0.0023623467423021793f, 0.002904806286096573f, 0.0023777177557349205f, 0.001994677586480975f, 0.0025958644691854715f, 0.0019485651282593608f, 0.00232474016956985f, 0.0016030633123591542f, 0.0028741469141095877f, 0.0024920101277530193f, 0.0021629086695611477f, 0.0029109963215887547f, 0.002821016125380993f, 0.002245515352115035f, 0.002481714589521289f, 0.0026114957872778177f, 0.00207670871168375f, 0.0012422369327396154f, 0.002998401178047061f, 0.002025446156039834f, 0.001374142011627555f, 0.0018564159981906414f, 0.0022129297722131014f, 0.0020410828292369843f, 0.0013702784199267626f);
static const ai_layer_format_type conv2d_9_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_9_t_out_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_9_t_out_0_shape_h_const_u16 = 14;

static const ai_u16 conv2d_7_t_in_0_shape_w_const_u16 = 28;
static const ai_u16 conv2d_7_t_in_0_shape_h_const_u16 = 28;
static const ai_u16 conv2d_7_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_7_t_out_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_7_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_7_t_weight_0_shape_h_const_u16 = 3;
static const ai_u16 conv2d_7_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_7_l_stride_0_const_u16 = 2;
static const ai_i32 conv2d_7_l_pad_W_0_const_s32 = 0;
static const ai_i32 conv2d_7_l_pad_H_0_const_s32 = 0;
static const ai_i8 conv2d_7_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_7_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_7_t_in_0_fmt_scale_const_f32 = 0.06131451204419136f;
static const ai_float conv2d_7_t_out_0_fmt_scale_const_f32 = 0.03198391944169998f;
static const ai_float conv2d_7_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.000870176125317812f, 0.0006339572137221694f, 0.0008433535695075989f, 0.0006498390575870872f, 0.0006917036953382194f, 0.0007228246540762484f, 0.000775940075982362f, 0.0005637688445858657f, 0.0008138103876262903f, 0.0008849892183206975f, 0.0011310961563140154f, 0.0006953426636755466f, 0.0011294991709291935f, 0.0011981116840615869f, 0.0007315531838685274f, 0.0006199972121976316f, 0.0006391709903255105f, 0.0007680494454689324f, 0.0013538218336179852f, 0.0006254379404708743f, 0.0006774155772291124f, 0.0007320667500607669f, 0.0006992202834226191f, 0.0006098581361584365f, 0.0007180345710366964f, 0.0009698474314063787f, 0.0007701840368099511f, 0.0006767695304006338f, 0.000713811838068068f, 0.000779567111749202f, 0.0011446975404396653f, 0.0005318765761330724f);
static const ai_layer_format_type conv2d_7_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_7_t_out_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_7_t_out_0_shape_h_const_u16 = 14;

static const ai_i8 conv2d_8_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_8_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_8_pad_before_t_in_0_shape_h_const_u32 = 14;

static const ai_u16 conv2d_8_t_in_0_shape_w_const_u16 = 16;
static const ai_u16 conv2d_8_t_in_0_shape_h_const_u16 = 16;
static const ai_u16 conv2d_8_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_8_t_out_0_shape_ch_const_u16 = 32;
static const ai_i8 conv2d_8_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_8_t_out_0_fmt_zero_const_s8 = -23;
static const ai_float conv2d_8_t_in_0_fmt_scale_const_f32 = 0.03198391944169998f;
static const ai_float conv2d_8_t_out_0_fmt_scale_const_f32 = 0.0719042494893074f;
static const ai_float conv2d_8_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0015937103889882565f, 0.001460529281757772f, 0.0013655138900503516f, 0.0009862156584858894f, 0.0009906614432111382f, 0.0013232623459771276f, 0.001593925291672349f, 0.0012566731311380863f, 0.0011224555782973766f, 0.0012995610013604164f, 0.0013125116238370538f, 0.0010870791738852859f, 0.001233225455507636f, 0.0012154493015259504f, 0.0016669468022882938f, 0.001273279427550733f, 0.0013016551965847611f, 0.0012340499088168144f, 0.0017822011141106486f, 0.0014227037318050861f, 0.0016563701210543513f, 0.0014327592216432095f, 0.0011967374011874199f, 0.0010261712595820427f, 0.001330742728896439f, 0.0012193563161417842f, 0.0015792076010257006f, 0.0018801548285409808f, 0.0011367433471605182f, 0.0015040404396131635f, 0.001473411452025175f, 0.001728956587612629f);
static const ai_layer_format_type conv2d_8_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_8_t_out_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_8_t_out_0_shape_h_const_u16 = 14;


static const ai_i8 conv2d_11_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_11_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_11_pad_before_t_in_0_shape_h_const_u32 = 14;

static const ai_u16 conv2d_11_t_in_0_shape_w_const_u16 = 16;
static const ai_u16 conv2d_11_t_in_0_shape_h_const_u16 = 16;
static const ai_u16 conv2d_11_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_11_t_out_0_shape_ch_const_u16 = 32;
static const ai_i8 conv2d_11_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_11_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_11_t_in_0_fmt_scale_const_f32 = 0.04220288619399071f;
static const ai_float conv2d_11_t_out_0_fmt_scale_const_f32 = 0.029274458065629005f;
static const ai_float conv2d_11_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0010359163861721754f, 0.0008679598686285317f, 0.0014983178116381168f, 0.0010938856285065413f, 0.0009146169177256525f, 0.0013622441329061985f, 0.0013029188849031925f, 0.001098737819120288f, 0.00112195557449013f, 0.0011371521977707744f, 0.001311965985223651f, 0.0013967896811664104f, 0.0012905099429190159f, 0.0008044139831326902f, 0.0009276047348976135f, 0.0011327809188514948f, 0.00094414601335302f, 0.0010967820417135954f, 0.0013270759955048561f, 0.0010312448721379042f, 0.0012557413429021835f, 0.0009746053256094456f, 0.0011103951837867498f, 0.0013651555636897683f, 0.0014906830620020628f, 0.0012233959278091788f, 0.0012217110488563776f, 0.001142409397289157f, 0.00133862579241395f, 0.0011245787609368563f, 0.0011151902144774795f, 0.0008934323559515178f);
static const ai_layer_format_type conv2d_11_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_11_t_out_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_11_t_out_0_shape_h_const_u16 = 14;

static const ai_i8 conv2d_12_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_12_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_12_pad_before_t_in_0_shape_h_const_u32 = 14;

static const ai_u16 conv2d_12_t_in_0_shape_w_const_u16 = 16;
static const ai_u16 conv2d_12_t_in_0_shape_h_const_u16 = 16;
static const ai_u16 conv2d_12_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_12_t_out_0_shape_ch_const_u16 = 32;
static const ai_i8 conv2d_12_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_12_t_out_0_fmt_zero_const_s8 = -32;
static const ai_float conv2d_12_t_in_0_fmt_scale_const_f32 = 0.029274458065629005f;
static const ai_float conv2d_12_t_out_0_fmt_scale_const_f32 = 0.08221554756164551f;
static const ai_float conv2d_12_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0014308384852483869f, 0.0016716434620320797f, 0.0016013410640880466f, 0.0015510632656514645f, 0.0018108593067154288f, 0.0013094001915305853f, 0.0014946697046980262f, 0.0018132771365344524f, 0.0015711705200374126f, 0.0015137495938688517f, 0.001716558588668704f, 0.0017191574443131685f, 0.0016127103008329868f, 0.0013917001197114587f, 0.0012445125030353665f, 0.0019957716576755047f, 0.0017376625910401344f, 0.0020035342313349247f, 0.0016112430021166801f, 0.0018765846034511924f, 0.00147920788731426f, 0.0012789604952558875f, 0.0013014200376346707f, 0.0014253213303163648f, 0.0012709127040579915f, 0.001678189029917121f, 0.0016498053446412086f, 0.001370018464513123f, 0.0013957212213426828f, 0.0027977379504591227f, 0.0016076245810836554f, 0.0014255605638027191f);
static const ai_layer_format_type conv2d_12_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_12_t_out_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_12_t_out_0_shape_h_const_u16 = 14;


static const ai_i8 conv2d_14_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_14_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_14_pad_before_t_in_0_shape_h_const_u32 = 14;

static const ai_u16 conv2d_14_t_in_0_shape_w_const_u16 = 16;
static const ai_u16 conv2d_14_t_in_0_shape_h_const_u16 = 16;
static const ai_u16 conv2d_14_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_14_t_out_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_14_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_14_t_weight_0_shape_h_const_u16 = 3;
static const ai_u16 conv2d_14_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_14_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_14_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_14_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_14_t_in_0_fmt_scale_const_f32 = 0.0511508472263813f;
static const ai_float conv2d_14_t_out_0_fmt_scale_const_f32 = 0.026751190423965454f;
static const ai_float conv2d_14_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0007457927567884326f, 0.0006909809890203178f, 0.0008527543977834284f, 0.0006797265959903598f, 0.0011479513486847281f, 0.0005431998870335519f, 0.0009471873054280877f, 0.0008263665367849171f, 0.001281192060559988f, 0.0005294450675137341f, 0.0008121287683025002f, 0.0007589549641124904f, 0.0007094712927937508f, 0.0009787828894332051f, 0.0006834181258454919f, 0.0005865190760232508f, 0.0008283562492579222f, 0.0008233709959313273f, 0.000875112134963274f, 0.0008012215257622302f, 0.0012386395828798413f, 0.0007529156864620745f, 0.0009227290283888578f, 0.0007576706120744348f, 0.0007504231762140989f, 0.0005970405181869864f, 0.0009407795732840896f, 0.0008947312016971409f, 0.0010379351442679763f, 0.0008785816607996821f, 0.0007332151872105896f, 0.0009256456978619099f, 0.0008350041462108493f, 0.0007250552298501134f, 0.000811356701888144f, 0.000736930058337748f, 0.0008674577111378312f, 0.0009340864489786327f, 0.001054135151207447f, 0.0008066484006121755f, 0.0006833851803094149f, 0.0007748216157779098f, 0.0008558935369364917f, 0.000892908894456923f, 0.0007654160144738853f, 0.0008241594186984003f, 0.001097602886147797f, 0.0008565601892769337f, 0.0007512646843679249f, 0.0009043586906045675f, 0.0007484303787350655f, 0.0009630665299482644f, 0.0005633875844068825f, 0.0005942421848885715f, 0.0011076127411797643f, 0.0007451695273630321f, 0.0010800271993502975f, 0.0009122429764829576f, 0.0007936788024380803f, 0.0006465346086770296f, 0.0008356752805411816f, 0.0008358541526831686f, 0.0006301664980128407f, 0.0009345817961730063f);
static const ai_layer_format_type conv2d_14_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_14_t_out_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_14_t_out_0_shape_h_const_u16 = 7;

static const ai_i8 conv2d_15_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_15_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_15_pad_before_t_in_0_shape_h_const_u32 = 7;

static const ai_u16 conv2d_15_t_in_0_shape_w_const_u16 = 9;
static const ai_u16 conv2d_15_t_in_0_shape_h_const_u16 = 9;
static const ai_u16 conv2d_15_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_15_t_out_0_shape_ch_const_u16 = 64;
static const ai_i8 conv2d_15_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_15_t_out_0_fmt_zero_const_s8 = -25;
static const ai_float conv2d_15_t_in_0_fmt_scale_const_f32 = 0.026751190423965454f;
static const ai_float conv2d_15_t_out_0_fmt_scale_const_f32 = 0.06859904527664185f;
static const ai_float conv2d_15_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0013983664102852345f, 0.0010365467751398683f, 0.0012032950762659311f, 0.0007998638902790844f, 0.0012716342462226748f, 0.001391872065141797f, 0.0013401645701378584f, 0.0014500614488497376f, 0.0012345124268904328f, 0.0017947119195014238f, 0.0014733042335137725f, 0.0011289151152595878f, 0.001020232797600329f, 0.001326464582234621f, 0.0011720091570168734f, 0.0015531640965491533f, 0.0012387618189677596f, 0.001429088180884719f, 0.0011432913597673178f, 0.0015900348080322146f, 0.0009535847348161042f, 0.0012560745235532522f, 0.0010712174698710442f, 0.0016264052828773856f, 0.0007588427979499102f, 0.0011419681832194328f, 0.001410981873050332f, 0.001284848665818572f, 0.000961841200478375f, 0.001478377846069634f, 0.0012382555287331343f, 0.0009716027416288853f, 0.0013961303047835827f, 0.0012316383654251695f, 0.001185423112474382f, 0.0012744043488055468f, 0.001166689209640026f, 0.0012700491352006793f, 0.0016115702455863357f, 0.0011568787740543485f, 0.001008629216812551f, 0.0007718232809565961f, 0.0012642770307138562f, 0.0013244104338809848f, 0.0012718733632937074f, 0.001282369950786233f, 0.0011543022701516747f, 0.0012544047785922885f, 0.0009335745708085597f, 0.0013667693128809333f, 0.001397737767547369f, 0.0015637518372386694f, 0.0011449574958533049f, 0.0010428326204419136f, 0.0011876904172822833f, 0.0014225515769794583f, 0.0012254406465217471f, 0.001301522133871913f, 0.001209470210596919f, 0.0013622987316921353f, 0.0012764957500621676f, 0.001113076345063746f, 0.001097230240702629f, 0.0012977203587070107f);
static const ai_layer_format_type conv2d_15_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_15_t_out_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_15_t_out_0_shape_h_const_u16 = 7;

static const ai_u16 conv2d_16_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_16_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_16_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_16_l_stride_0_const_u16 = 2;
static const ai_u16 conv2d_16_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_16_t_out_0_shape_ch_const_u16 = 64;
static const ai_i8 conv2d_16_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_16_t_out_0_fmt_zero_const_s8 = 12;
static const ai_float conv2d_16_t_in_0_fmt_scale_const_f32 = 0.0511508472263813f;
static const ai_float conv2d_16_t_out_0_fmt_scale_const_f32 = 0.05399082973599434f;
static const ai_float conv2d_16_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0024650159757584333f, 0.00279154977761209f, 0.002198621863499284f, 0.0021414190996438265f, 0.002953312126919627f, 0.0032936048228293657f, 0.002462906762957573f, 0.0017481145914644003f, 0.0017835355829447508f, 0.0023651618976145983f, 0.0017063772538676858f, 0.00251399795524776f, 0.0017325174994766712f, 0.0028429292142391205f, 0.002959402510896325f, 0.0018229836132377386f, 0.002606532070785761f, 0.002149414038285613f, 0.0023229660000652075f, 0.0018546193605288863f, 0.0021985683124512434f, 0.0028265919536352158f, 0.0026113935746252537f, 0.002439104253426194f, 0.001724223606288433f, 0.0023580826818943024f, 0.0021739546209573746f, 0.0019026844529435039f, 0.0017388904234394431f, 0.0017265635542571545f, 0.0029916693456470966f, 0.0015994779532775283f, 0.002429459011182189f, 0.0027870561461895704f, 0.0015065452316775918f, 0.0019263066351413727f, 0.002722066128626466f, 0.0021695198956876993f, 0.001809755340218544f, 0.0022599324584007263f, 0.0018834082875400782f, 0.0027797152288258076f, 0.0022811805829405785f, 0.0016987372655421495f, 0.0018362917471677065f, 0.0017064433777704835f, 0.002066795015707612f, 0.0025177509523928165f, 0.002129698870703578f, 0.00210694782435894f, 0.0022553999442607164f, 0.0026313248090445995f, 0.002425253624096513f, 0.0016440829494968057f, 0.0021167383529245853f, 0.0023219892755150795f, 0.0017980632837861776f, 0.0015430868370458484f, 0.0017672155518084764f, 0.002879668492823839f, 0.002431395696476102f, 0.0024205786176025867f, 0.002241255482658744f, 0.0020208260975778103f);
static const ai_layer_format_type conv2d_16_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;


static const ai_i8 conv2d_18_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_18_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_18_pad_before_t_in_0_shape_h_const_u32 = 7;

static const ai_u16 conv2d_18_t_in_0_shape_w_const_u16 = 9;
static const ai_u16 conv2d_18_t_in_0_shape_h_const_u16 = 9;
static const ai_u16 conv2d_18_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_18_t_out_0_shape_ch_const_u16 = 64;
static const ai_i8 conv2d_18_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_18_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_18_t_in_0_fmt_scale_const_f32 = 0.040968943387269974f;
static const ai_float conv2d_18_t_out_0_fmt_scale_const_f32 = 0.02445421740412712f;
static const ai_float conv2d_18_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0008584379102103412f, 0.0007464423542842269f, 0.0007537099882028997f, 0.0008742653881199658f, 0.0009566397638991475f, 0.0005899923853576183f, 0.0007687596953473985f, 0.0005937697715125978f, 0.0005885642021894455f, 0.0007713481318205595f, 0.0007522817468270659f, 0.0007463326910510659f, 0.0008963558357208967f, 0.0006776677328161895f, 0.0010622319532558322f, 0.0007041209028102458f, 0.0007360766758210957f, 0.0007192803896032274f, 0.0008225309429690242f, 0.0007465930539183319f, 0.0006925943307578564f, 0.0006700953235849738f, 0.000725161749869585f, 0.0006046103080734611f, 0.0007730369106866419f, 0.0006781135452911258f, 0.0006195135065354407f, 0.0005402570241130888f, 0.0007406334625557065f, 0.0007537566707469523f, 0.0008891658508218825f, 0.0006599345942959189f, 0.0008316377643495798f, 0.0008966985624283552f, 0.0006326085422188044f, 0.0006323757697828114f, 0.0006748588057234883f, 0.0009010272333398461f, 0.0008354639867320657f, 0.0008969942573457956f, 0.0010909968987107277f, 0.000984075479209423f, 0.0006291529280133545f, 0.0007184090791270137f, 0.0008601840818300843f, 0.0007433566497638822f, 0.000861565989907831f, 0.0006355036748573184f, 0.0010366737842559814f, 0.00095055450219661f, 0.0006979501340538263f, 0.000717854592949152f, 0.000889246934093535f, 0.0006451594526879489f, 0.0006671577575616539f, 0.0007296113763004541f, 0.0007210579933598638f, 0.0007841850747354329f, 0.0006897143903188407f, 0.0009115133434534073f, 0.0007459656335413456f, 0.0006647756090387702f, 0.00093284179456532f, 0.0008373858290724456f);
static const ai_layer_format_type conv2d_18_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_18_t_out_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_18_t_out_0_shape_h_const_u16 = 7;

static const ai_i8 conv2d_19_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_19_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_19_pad_before_t_in_0_shape_h_const_u32 = 7;

static const ai_u16 conv2d_19_t_in_0_shape_w_const_u16 = 9;
static const ai_u16 conv2d_19_t_in_0_shape_h_const_u16 = 9;
static const ai_u16 conv2d_19_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_19_t_out_0_shape_ch_const_u16 = 64;
static const ai_i8 conv2d_19_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_19_t_out_0_fmt_zero_const_s8 = -44;
static const ai_float conv2d_19_t_in_0_fmt_scale_const_f32 = 0.02445421740412712f;
static const ai_float conv2d_19_t_out_0_fmt_scale_const_f32 = 0.08927126228809357f;
static const ai_float conv2d_19_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0010867733508348465f, 0.0010854772990569472f, 0.001375229679979384f, 0.0011251242831349373f, 0.0011012335307896137f, 0.0011977447429671884f, 0.0011605892796069384f, 0.0011773979058489203f, 0.0010138166835531592f, 0.0008513047359883785f, 0.001146633643656969f, 0.0012615268351510167f, 0.0011217654682695866f, 0.0014238518197089434f, 0.0013029001420363784f, 0.001474803895689547f, 0.0012089221272617579f, 0.0012544404016807675f, 0.001246216124854982f, 0.0012117072474211454f, 0.001257691066712141f, 0.0010066108079627156f, 0.0011692496482282877f, 0.0011020662495866418f, 0.001250220462679863f, 0.0012419885024428368f, 0.0012349458411335945f, 0.001157825463451445f, 0.0011343341320753098f, 0.001158492872491479f, 0.0012767771258950233f, 0.0009318728116340935f, 0.0010388647206127644f, 0.0014596732798963785f, 0.0010899463668465614f, 0.0011230484815314412f, 0.001079814275726676f, 0.0011627024505287409f, 0.001100008375942707f, 0.0013050942216068506f, 0.0010315352119505405f, 0.000983038335107267f, 0.001134789315983653f, 0.0011089665349572897f, 0.0011844501132145524f, 0.0010424340143799782f, 0.0013565850676968694f, 0.0015187972458079457f, 0.0011831165757030249f, 0.0009251382434740663f, 0.0009060949669219553f, 0.001109856879338622f, 0.0010190425673499703f, 0.0012822475982829928f, 0.001321699470281601f, 0.0010994343319907784f, 0.0012227881234139204f, 0.0012042062589898705f, 0.0010734390234574676f, 0.0013171258615329862f, 0.0010425903601571918f, 0.0011394282337278128f, 0.0012812743661925197f, 0.000949315435718745f);
static const ai_layer_format_type conv2d_19_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_19_t_out_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_19_t_out_0_shape_h_const_u16 = 7;




static const ai_u32 nl_23_t_in_0_shape_ch_prod_const_u32 = 10;
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
    ai_i8* conv2d_0_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 17152);
    ai_i16* conv2d_0_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 16604);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(0, 1, {(stai_ptr) conv2d_0_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_sssa8_ch(conv2d_0_t_in_0_ptr_const_s8, conv2d_0_t_in_0_shape_w_const_u16, conv2d_0_t_in_0_shape_h_const_u16, conv2d_0_t_in_0_shape_ch_const_u16, conv2d_0_t_weight_0_ptr_const_s8, conv2d_0_t_out_0_shape_ch_const_u16, conv2d_0_t_weight_0_shape_w_const_u16, conv2d_0_t_weight_0_shape_h_const_u16, conv2d_0_l_stride_1_const_u16, conv2d_0_l_stride_0_const_u16, conv2d_0_l_pad_W_0_const_s32, conv2d_0_l_pad_H_0_const_s32, conv2d_0_t_weight_1_ptr_const_s32, conv2d_0_t_in_0_fmt_zero_const_s8, conv2d_0_t_out_0_fmt_zero_const_s8, conv2d_0_t_in_0_fmt_scale_const_f32, conv2d_0_t_out_0_fmt_scale_const_f32, conv2d_0_t_weight_0_fmt_scale_const_f32, conv2d_0_l_out_ch_format_const_layer_format_type, conv2d_0_t_out_0_ptr_s8, conv2d_0_t_out_0_shape_w_const_u16, conv2d_0_t_out_0_shape_h_const_u16, 1, 548, conv2d_0_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(0, 1, {(stai_ptr) conv2d_0_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_0 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_1_pad_before */
  {
      const ai_ptr conv2d_1_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 17152);
    ai_ptr conv2d_1_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 2752);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(1, 1, {(stai_ptr) conv2d_1_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_1_pad_before_t_in_0_ptr_const_ptr, conv2d_1_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_1_pad_before_v_pad_constant_value_const_s8), conv2d_1_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_1_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(448), (ai_i32)(480), (ai_i32)(480), (ai_i32)(16), (ai_i32)(16));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(1, 1, {(stai_ptr) conv2d_1_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_1_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_1 */
  {
      const ai_i8* conv2d_1_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 2752);
    const ai_i8* conv2d_1_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[2] + 0);
    const ai_i32* conv2d_1_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[3] + 0);
    ai_i8* conv2d_1_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 2304);
    ai_i16* conv2d_1_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 29696);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(1, 1, {(stai_ptr) conv2d_1_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_1_t_in_0_ptr_const_s8, conv2d_1_t_in_0_shape_w_const_u16, conv2d_1_t_in_0_shape_h_const_u16, conv2d_1_t_in_0_shape_ch_const_u16, conv2d_1_t_weight_0_ptr_const_s8, conv2d_1_t_out_0_shape_ch_const_u16, conv2d_1_t_weight_1_ptr_const_s32, conv2d_1_t_in_0_fmt_zero_const_s8, conv2d_1_t_out_0_fmt_zero_const_s8, conv2d_1_t_in_0_fmt_scale_const_f32, conv2d_1_t_out_0_fmt_scale_const_f32, conv2d_1_t_weight_0_fmt_scale_const_f32, conv2d_1_l_out_ch_format_const_layer_format_type, conv2d_1_t_out_0_ptr_s8, conv2d_1_t_out_0_shape_w_const_u16, conv2d_1_t_out_0_shape_h_const_u16, 1, 5408, conv2d_1_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(1, 1, {(stai_ptr) conv2d_1_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_1 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_2_pad_before */
  {
      const ai_ptr conv2d_2_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 2304);
    ai_ptr conv2d_2_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 448);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(2, 1, {(stai_ptr) conv2d_2_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_2_pad_before_t_in_0_ptr_const_ptr, conv2d_2_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_2_pad_before_v_pad_constant_value_const_s8), conv2d_2_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_2_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(448), (ai_i32)(480), (ai_i32)(480), (ai_i32)(16), (ai_i32)(16));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(2, 1, {(stai_ptr) conv2d_2_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_2_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_2 */
  {
      const ai_i8* conv2d_2_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 448);
    const ai_i8* conv2d_2_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[4] + 0);
    const ai_i32* conv2d_2_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[5] + 0);
    ai_i8* conv2d_2_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_2_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 29696);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(2, 1, {(stai_ptr) conv2d_2_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_2_t_in_0_ptr_const_s8, conv2d_2_t_in_0_shape_w_const_u16, conv2d_2_t_in_0_shape_h_const_u16, conv2d_2_t_in_0_shape_ch_const_u16, conv2d_2_t_weight_0_ptr_const_s8, conv2d_2_t_out_0_shape_ch_const_u16, conv2d_2_t_weight_1_ptr_const_s32, conv2d_2_t_in_0_fmt_zero_const_s8, conv2d_2_t_out_0_fmt_zero_const_s8, conv2d_2_t_in_0_fmt_scale_const_f32, conv2d_2_t_out_0_fmt_scale_const_f32, conv2d_2_t_weight_0_fmt_scale_const_f32, conv2d_2_l_out_ch_format_const_layer_format_type, conv2d_2_t_out_0_ptr_s8, conv2d_2_t_out_0_shape_w_const_u16, conv2d_2_t_out_0_shape_h_const_u16, 1, 5408, conv2d_2_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(2, 1, {(stai_ptr) conv2d_2_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_2 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_3 */
  {
    
  forward_lite_eltwise_3(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_3 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_4_pad_before */
  {
      const ai_ptr conv2d_4_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 0);
    ai_ptr conv2d_4_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 20704);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(4, 1, {(stai_ptr) conv2d_4_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_4_pad_before_t_in_0_ptr_const_ptr, conv2d_4_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_4_pad_before_v_pad_constant_value_const_s8), conv2d_4_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_4_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(448), (ai_i32)(480), (ai_i32)(480), (ai_i32)(16), (ai_i32)(16));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(4, 1, {(stai_ptr) conv2d_4_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_4_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_4 */
  {
      const ai_i8* conv2d_4_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 20704);
    const ai_i8* conv2d_4_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[6] + 0);
    const ai_i32* conv2d_4_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[7] + 0);
    ai_i8* conv2d_4_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 20256);
    ai_i16* conv2d_4_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 12544);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(4, 1, {(stai_ptr) conv2d_4_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_4_t_in_0_ptr_const_s8, conv2d_4_t_in_0_shape_w_const_u16, conv2d_4_t_in_0_shape_h_const_u16, conv2d_4_t_in_0_shape_ch_const_u16, conv2d_4_t_weight_0_ptr_const_s8, conv2d_4_t_out_0_shape_ch_const_u16, conv2d_4_t_weight_1_ptr_const_s32, conv2d_4_t_in_0_fmt_zero_const_s8, conv2d_4_t_out_0_fmt_zero_const_s8, conv2d_4_t_in_0_fmt_scale_const_f32, conv2d_4_t_out_0_fmt_scale_const_f32, conv2d_4_t_weight_0_fmt_scale_const_f32, conv2d_4_l_out_ch_format_const_layer_format_type, conv2d_4_t_out_0_ptr_s8, conv2d_4_t_out_0_shape_w_const_u16, conv2d_4_t_out_0_shape_h_const_u16, 1, 5408, conv2d_4_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(4, 1, {(stai_ptr) conv2d_4_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_4 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_5_pad_before */
  {
      const ai_ptr conv2d_5_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 20256);
    ai_ptr conv2d_5_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 18400);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(5, 1, {(stai_ptr) conv2d_5_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_5_pad_before_t_in_0_ptr_const_ptr, conv2d_5_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_5_pad_before_v_pad_constant_value_const_s8), conv2d_5_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_5_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(448), (ai_i32)(480), (ai_i32)(480), (ai_i32)(16), (ai_i32)(16));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(5, 1, {(stai_ptr) conv2d_5_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_5_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_5 */
  {
      const ai_i8* conv2d_5_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 18400);
    const ai_i8* conv2d_5_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[8] + 0);
    const ai_i32* conv2d_5_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[9] + 0);
    ai_i8* conv2d_5_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 17952);
    ai_i16* conv2d_5_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 12544);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(5, 1, {(stai_ptr) conv2d_5_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_5_t_in_0_ptr_const_s8, conv2d_5_t_in_0_shape_w_const_u16, conv2d_5_t_in_0_shape_h_const_u16, conv2d_5_t_in_0_shape_ch_const_u16, conv2d_5_t_weight_0_ptr_const_s8, conv2d_5_t_out_0_shape_ch_const_u16, conv2d_5_t_weight_1_ptr_const_s32, conv2d_5_t_in_0_fmt_zero_const_s8, conv2d_5_t_out_0_fmt_zero_const_s8, conv2d_5_t_in_0_fmt_scale_const_f32, conv2d_5_t_out_0_fmt_scale_const_f32, conv2d_5_t_weight_0_fmt_scale_const_f32, conv2d_5_l_out_ch_format_const_layer_format_type, conv2d_5_t_out_0_ptr_s8, conv2d_5_t_out_0_shape_w_const_u16, conv2d_5_t_out_0_shape_h_const_u16, 1, 5408, conv2d_5_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(5, 1, {(stai_ptr) conv2d_5_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_5 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_6 */
  {
    
  forward_lite_eltwise_6(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_6 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_9 */
  {
      const ai_i8* conv2d_9_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 17952);
    const ai_i8* conv2d_9_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[10] + 0);
    const ai_i32* conv2d_9_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[11] + 0);
    ai_i8* conv2d_9_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1536);
    ai_i16* conv2d_9_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(9, 1, {(stai_ptr) conv2d_9_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_sssa8_ch(conv2d_9_t_in_0_ptr_const_s8, conv2d_9_t_in_0_shape_w_const_u16, conv2d_9_t_in_0_shape_h_const_u16, conv2d_9_t_in_0_shape_ch_const_u16, conv2d_9_t_weight_0_ptr_const_s8, conv2d_9_t_out_0_shape_ch_const_u16, conv2d_9_t_weight_0_shape_w_const_u16, conv2d_9_t_weight_0_shape_h_const_u16, conv2d_9_l_stride_1_const_u16, conv2d_9_l_stride_0_const_u16, conv2d_9_l_pad_W_0_const_s32, conv2d_9_l_pad_H_0_const_s32, conv2d_9_t_weight_1_ptr_const_s32, conv2d_9_t_in_0_fmt_zero_const_s8, conv2d_9_t_out_0_fmt_zero_const_s8, conv2d_9_t_in_0_fmt_scale_const_f32, conv2d_9_t_out_0_fmt_scale_const_f32, conv2d_9_t_weight_0_fmt_scale_const_f32, conv2d_9_l_out_ch_format_const_layer_format_type, conv2d_9_t_out_0_ptr_s8, conv2d_9_t_out_0_shape_w_const_u16, conv2d_9_t_out_0_shape_h_const_u16, 1, 1536, conv2d_9_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(9, 1, {(stai_ptr) conv2d_9_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_9 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_7 */
  {
      const ai_i8* conv2d_7_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 17952);
    const ai_i8* conv2d_7_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[12] + 0);
    const ai_i32* conv2d_7_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[13] + 0);
    ai_i8* conv2d_7_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 17472);
    ai_i16* conv2d_7_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 7808);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(7, 1, {(stai_ptr) conv2d_7_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_sssa8_ch(conv2d_7_t_in_0_ptr_const_s8, conv2d_7_t_in_0_shape_w_const_u16, conv2d_7_t_in_0_shape_h_const_u16, conv2d_7_t_in_0_shape_ch_const_u16, conv2d_7_t_weight_0_ptr_const_s8, conv2d_7_t_out_0_shape_ch_const_u16, conv2d_7_t_weight_0_shape_w_const_u16, conv2d_7_t_weight_0_shape_h_const_u16, conv2d_7_l_stride_1_const_u16, conv2d_7_l_stride_0_const_u16, conv2d_7_l_pad_W_0_const_s32, conv2d_7_l_pad_H_0_const_s32, conv2d_7_t_weight_1_ptr_const_s32, conv2d_7_t_in_0_fmt_zero_const_s8, conv2d_7_t_out_0_fmt_zero_const_s8, conv2d_7_t_in_0_fmt_scale_const_f32, conv2d_7_t_out_0_fmt_scale_const_f32, conv2d_7_t_weight_0_fmt_scale_const_f32, conv2d_7_l_out_ch_format_const_layer_format_type, conv2d_7_t_out_0_ptr_s8, conv2d_7_t_out_0_shape_w_const_u16, conv2d_7_t_out_0_shape_h_const_u16, 1, 6144, conv2d_7_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(7, 1, {(stai_ptr) conv2d_7_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_7 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_8_pad_before */
  {
      const ai_ptr conv2d_8_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 17472);
    ai_ptr conv2d_8_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 7808);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(8, 1, {(stai_ptr) conv2d_8_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_8_pad_before_t_in_0_ptr_const_ptr, conv2d_8_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_8_pad_before_v_pad_constant_value_const_s8), conv2d_8_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_8_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(448), (ai_i32)(512), (ai_i32)(512), (ai_i32)(32), (ai_i32)(32));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(8, 1, {(stai_ptr) conv2d_8_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_8_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_8 */
  {
      const ai_i8* conv2d_8_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 7808);
    const ai_i8* conv2d_8_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[14] + 0);
    const ai_i32* conv2d_8_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[15] + 0);
    ai_i8* conv2d_8_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 22720);
    ai_i16* conv2d_8_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 16000);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(8, 1, {(stai_ptr) conv2d_8_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_8_t_in_0_ptr_const_s8, conv2d_8_t_in_0_shape_w_const_u16, conv2d_8_t_in_0_shape_h_const_u16, conv2d_8_t_in_0_shape_ch_const_u16, conv2d_8_t_weight_0_ptr_const_s8, conv2d_8_t_out_0_shape_ch_const_u16, conv2d_8_t_weight_1_ptr_const_s32, conv2d_8_t_in_0_fmt_zero_const_s8, conv2d_8_t_out_0_fmt_zero_const_s8, conv2d_8_t_in_0_fmt_scale_const_f32, conv2d_8_t_out_0_fmt_scale_const_f32, conv2d_8_t_weight_0_fmt_scale_const_f32, conv2d_8_l_out_ch_format_const_layer_format_type, conv2d_8_t_out_0_ptr_s8, conv2d_8_t_out_0_shape_w_const_u16, conv2d_8_t_out_0_shape_h_const_u16, 1, 6720, conv2d_8_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(8, 1, {(stai_ptr) conv2d_8_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_8 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_10 */
  {
    
  forward_lite_eltwise_10(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_10 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_11_pad_before */
  {
      const ai_ptr conv2d_11_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 7808);
    ai_ptr conv2d_11_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 14080);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(11, 1, {(stai_ptr) conv2d_11_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_11_pad_before_t_in_0_ptr_const_ptr, conv2d_11_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_11_pad_before_v_pad_constant_value_const_s8), conv2d_11_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_11_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(448), (ai_i32)(512), (ai_i32)(512), (ai_i32)(32), (ai_i32)(32));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(11, 1, {(stai_ptr) conv2d_11_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_11_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_11 */
  {
      const ai_i8* conv2d_11_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 14080);
    const ai_i8* conv2d_11_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[16] + 0);
    const ai_i32* conv2d_11_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[17] + 0);
    ai_i8* conv2d_11_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 22272);
    ai_i16* conv2d_11_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(11, 1, {(stai_ptr) conv2d_11_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_11_t_in_0_ptr_const_s8, conv2d_11_t_in_0_shape_w_const_u16, conv2d_11_t_in_0_shape_h_const_u16, conv2d_11_t_in_0_shape_ch_const_u16, conv2d_11_t_weight_0_ptr_const_s8, conv2d_11_t_out_0_shape_ch_const_u16, conv2d_11_t_weight_1_ptr_const_s32, conv2d_11_t_in_0_fmt_zero_const_s8, conv2d_11_t_out_0_fmt_zero_const_s8, conv2d_11_t_in_0_fmt_scale_const_f32, conv2d_11_t_out_0_fmt_scale_const_f32, conv2d_11_t_weight_0_fmt_scale_const_f32, conv2d_11_l_out_ch_format_const_layer_format_type, conv2d_11_t_out_0_ptr_s8, conv2d_11_t_out_0_shape_w_const_u16, conv2d_11_t_out_0_shape_h_const_u16, 1, 6720, conv2d_11_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(11, 1, {(stai_ptr) conv2d_11_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_11 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_12_pad_before */
  {
      const ai_ptr conv2d_12_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 22272);
    ai_ptr conv2d_12_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 14080);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(12, 1, {(stai_ptr) conv2d_12_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_12_pad_before_t_in_0_ptr_const_ptr, conv2d_12_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_12_pad_before_v_pad_constant_value_const_s8), conv2d_12_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_12_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(448), (ai_i32)(512), (ai_i32)(512), (ai_i32)(32), (ai_i32)(32));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(12, 1, {(stai_ptr) conv2d_12_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_12_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_12 */
  {
      const ai_i8* conv2d_12_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 14080);
    const ai_i8* conv2d_12_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[18] + 0);
    const ai_i32* conv2d_12_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[19] + 0);
    ai_i8* conv2d_12_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 22272);
    ai_i16* conv2d_12_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(12, 1, {(stai_ptr) conv2d_12_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_12_t_in_0_ptr_const_s8, conv2d_12_t_in_0_shape_w_const_u16, conv2d_12_t_in_0_shape_h_const_u16, conv2d_12_t_in_0_shape_ch_const_u16, conv2d_12_t_weight_0_ptr_const_s8, conv2d_12_t_out_0_shape_ch_const_u16, conv2d_12_t_weight_1_ptr_const_s32, conv2d_12_t_in_0_fmt_zero_const_s8, conv2d_12_t_out_0_fmt_zero_const_s8, conv2d_12_t_in_0_fmt_scale_const_f32, conv2d_12_t_out_0_fmt_scale_const_f32, conv2d_12_t_weight_0_fmt_scale_const_f32, conv2d_12_l_out_ch_format_const_layer_format_type, conv2d_12_t_out_0_ptr_s8, conv2d_12_t_out_0_shape_w_const_u16, conv2d_12_t_out_0_shape_h_const_u16, 1, 6720, conv2d_12_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(12, 1, {(stai_ptr) conv2d_12_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_12 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_13 */
  {
    
  forward_lite_eltwise_13(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_13 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_14_pad_before */
  {
      const ai_ptr conv2d_14_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 0);
    ai_ptr conv2d_14_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 6272);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(14, 1, {(stai_ptr) conv2d_14_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_14_pad_before_t_in_0_ptr_const_ptr, conv2d_14_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_14_pad_before_v_pad_constant_value_const_s8), conv2d_14_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_14_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(448), (ai_i32)(0), (ai_i32)(1024), (ai_i32)(0), (ai_i32)(64));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(14, 1, {(stai_ptr) conv2d_14_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_14_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_14 */
  {
      const ai_i8* conv2d_14_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 6272);
    const ai_i8* conv2d_14_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[20] + 0);
    const ai_i32* conv2d_14_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[21] + 0);
    ai_i8* conv2d_14_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 21632);
    ai_i16* conv2d_14_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 14464);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(14, 1, {(stai_ptr) conv2d_14_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_sssa8_ch(conv2d_14_t_in_0_ptr_const_s8, conv2d_14_t_in_0_shape_w_const_u16, conv2d_14_t_in_0_shape_h_const_u16, conv2d_14_t_in_0_shape_ch_const_u16, conv2d_14_t_weight_0_ptr_const_s8, conv2d_14_t_out_0_shape_ch_const_u16, conv2d_14_t_weight_0_shape_w_const_u16, conv2d_14_t_weight_0_shape_h_const_u16, conv2d_14_l_stride_1_const_u16, conv2d_14_l_stride_0_const_u16, conv2d_14_t_weight_1_ptr_const_s32, conv2d_14_t_in_0_fmt_zero_const_s8, conv2d_14_t_out_0_fmt_zero_const_s8, conv2d_14_t_in_0_fmt_scale_const_f32, conv2d_14_t_out_0_fmt_scale_const_f32, conv2d_14_t_weight_0_fmt_scale_const_f32, conv2d_14_l_out_ch_format_const_layer_format_type, conv2d_14_t_out_0_ptr_s8, conv2d_14_t_out_0_shape_w_const_u16, conv2d_14_t_out_0_shape_h_const_u16, 1, 1, 7168, conv2d_14_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(14, 1, {(stai_ptr) conv2d_14_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_14 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_15_pad_before */
  {
      const ai_ptr conv2d_15_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 21632);
    ai_ptr conv2d_15_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 6272);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(15, 1, {(stai_ptr) conv2d_15_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_15_pad_before_t_in_0_ptr_const_ptr, conv2d_15_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_15_pad_before_v_pad_constant_value_const_s8), conv2d_15_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_15_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(448), (ai_i32)(576), (ai_i32)(576), (ai_i32)(64), (ai_i32)(64));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(15, 1, {(stai_ptr) conv2d_15_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_15_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_15 */
  {
      const ai_i8* conv2d_15_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 6272);
    const ai_i8* conv2d_15_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[22] + 0);
    const ai_i32* conv2d_15_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[23] + 0);
    ai_i8* conv2d_15_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 19776);
    ai_i16* conv2d_15_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 11456);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(15, 1, {(stai_ptr) conv2d_15_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_15_t_in_0_ptr_const_s8, conv2d_15_t_in_0_shape_w_const_u16, conv2d_15_t_in_0_shape_h_const_u16, conv2d_15_t_in_0_shape_ch_const_u16, conv2d_15_t_weight_0_ptr_const_s8, conv2d_15_t_out_0_shape_ch_const_u16, conv2d_15_t_weight_1_ptr_const_s32, conv2d_15_t_in_0_fmt_zero_const_s8, conv2d_15_t_out_0_fmt_zero_const_s8, conv2d_15_t_in_0_fmt_scale_const_f32, conv2d_15_t_out_0_fmt_scale_const_f32, conv2d_15_t_weight_0_fmt_scale_const_f32, conv2d_15_l_out_ch_format_const_layer_format_type, conv2d_15_t_out_0_ptr_s8, conv2d_15_t_out_0_shape_w_const_u16, conv2d_15_t_out_0_shape_h_const_u16, 1, 8320, conv2d_15_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(15, 1, {(stai_ptr) conv2d_15_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_15 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_16 */
  {
      const ai_i8* conv2d_16_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_16_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[24] + 0);
    const ai_i32* conv2d_16_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[25] + 0);
    ai_i8* conv2d_16_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 7040);
    ai_i16* conv2d_16_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 6272);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(16, 1, {(stai_ptr) conv2d_16_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_16_t_in_0_ptr_const_s8, conv2d_16_t_in_0_shape_w_const_u16, conv2d_16_t_in_0_shape_h_const_u16, conv2d_16_l_stride_1_const_u16, conv2d_16_l_stride_0_const_u16, conv2d_16_t_in_0_shape_ch_const_u16, conv2d_16_t_weight_0_ptr_const_s8, conv2d_16_t_out_0_shape_ch_const_u16, conv2d_16_t_weight_1_ptr_const_s32, conv2d_16_t_in_0_fmt_zero_const_s8, conv2d_16_t_out_0_fmt_zero_const_s8, conv2d_16_t_in_0_fmt_scale_const_f32, conv2d_16_t_out_0_fmt_scale_const_f32, conv2d_16_t_weight_0_fmt_scale_const_f32, conv2d_16_l_out_ch_format_const_layer_format_type, conv2d_16_t_out_0_ptr_s8, 1, 768, conv2d_16_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(16, 1, {(stai_ptr) conv2d_16_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_16 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_17 */
  {
    
  forward_lite_eltwise_17(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_17 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_18_pad_before */
  {
      const ai_ptr conv2d_18_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 0);
    ai_ptr conv2d_18_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 3136);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(18, 1, {(stai_ptr) conv2d_18_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_18_pad_before_t_in_0_ptr_const_ptr, conv2d_18_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_18_pad_before_v_pad_constant_value_const_s8), conv2d_18_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_18_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(448), (ai_i32)(576), (ai_i32)(576), (ai_i32)(64), (ai_i32)(64));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(18, 1, {(stai_ptr) conv2d_18_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_18_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_18 */
  {
      const ai_i8* conv2d_18_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 3136);
    const ai_i8* conv2d_18_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[26] + 0);
    const ai_i32* conv2d_18_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[27] + 0);
    ai_i8* conv2d_18_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 16640);
    ai_i16* conv2d_18_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 8320);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(18, 1, {(stai_ptr) conv2d_18_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_18_t_in_0_ptr_const_s8, conv2d_18_t_in_0_shape_w_const_u16, conv2d_18_t_in_0_shape_h_const_u16, conv2d_18_t_in_0_shape_ch_const_u16, conv2d_18_t_weight_0_ptr_const_s8, conv2d_18_t_out_0_shape_ch_const_u16, conv2d_18_t_weight_1_ptr_const_s32, conv2d_18_t_in_0_fmt_zero_const_s8, conv2d_18_t_out_0_fmt_zero_const_s8, conv2d_18_t_in_0_fmt_scale_const_f32, conv2d_18_t_out_0_fmt_scale_const_f32, conv2d_18_t_weight_0_fmt_scale_const_f32, conv2d_18_l_out_ch_format_const_layer_format_type, conv2d_18_t_out_0_ptr_s8, conv2d_18_t_out_0_shape_w_const_u16, conv2d_18_t_out_0_shape_h_const_u16, 1, 8320, conv2d_18_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(18, 1, {(stai_ptr) conv2d_18_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_18 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_19_pad_before */
  {
      const ai_ptr conv2d_19_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 16640);
    ai_ptr conv2d_19_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 3136);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(19, 1, {(stai_ptr) conv2d_19_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_19_pad_before_t_in_0_ptr_const_ptr, conv2d_19_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_19_pad_before_v_pad_constant_value_const_s8), conv2d_19_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_19_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(448), (ai_i32)(576), (ai_i32)(576), (ai_i32)(64), (ai_i32)(64));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(19, 1, {(stai_ptr) conv2d_19_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_19_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_19 */
  {
      const ai_i8* conv2d_19_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 3136);
    const ai_i8* conv2d_19_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[28] + 0);
    const ai_i32* conv2d_19_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[29] + 0);
    ai_i8* conv2d_19_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 16640);
    ai_i16* conv2d_19_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 8320);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(19, 1, {(stai_ptr) conv2d_19_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_19_t_in_0_ptr_const_s8, conv2d_19_t_in_0_shape_w_const_u16, conv2d_19_t_in_0_shape_h_const_u16, conv2d_19_t_in_0_shape_ch_const_u16, conv2d_19_t_weight_0_ptr_const_s8, conv2d_19_t_out_0_shape_ch_const_u16, conv2d_19_t_weight_1_ptr_const_s32, conv2d_19_t_in_0_fmt_zero_const_s8, conv2d_19_t_out_0_fmt_zero_const_s8, conv2d_19_t_in_0_fmt_scale_const_f32, conv2d_19_t_out_0_fmt_scale_const_f32, conv2d_19_t_weight_0_fmt_scale_const_f32, conv2d_19_l_out_ch_format_const_layer_format_type, conv2d_19_t_out_0_ptr_s8, conv2d_19_t_out_0_shape_w_const_u16, conv2d_19_t_out_0_shape_h_const_u16, 1, 8320, conv2d_19_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(19, 1, {(stai_ptr) conv2d_19_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_19 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_20 */
  {
    
  forward_lite_eltwise_20(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_20 */
  /* LITE_KERNEL_SECTION BEGIN pool_21 */
  {
    
  forward_lite_pool_21(net_ctx);
  }
  /* LITE_KERNEL_SECTION END pool_21 */
  /* LITE_KERNEL_SECTION BEGIN gemm_22 */
  {
    
  forward_lite_gemm_22(net_ctx);
  }
  /* LITE_KERNEL_SECTION END gemm_22 */
  /* LITE_KERNEL_SECTION BEGIN nl_23 */
  {
      ai_i8* nl_23_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_outputs[0] + 0);
    const ai_i8* nl_23_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 292);
    ai_i32* nl_23_t_scratch_0_ptr_s32 = (ai_i32*)(net_ctx->_activations[0] + 304);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(23, 1, {(stai_ptr) nl_23_t_in_0_ptr_const_s8});
    
  forward_lite_nl_softmax_is8os8(nl_23_t_out_0_ptr_s8, nl_23_t_in_0_ptr_const_s8, nl_23_t_in_0_shape_ch_prod_const_u32, 1, 10, 1996807936, 23, -248, nl_23_t_scratch_0_ptr_s32);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(23, 1, {(stai_ptr) nl_23_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END nl_23 */
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
  net_ctx->_inputs[0] = activations[0] + 15820;

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

