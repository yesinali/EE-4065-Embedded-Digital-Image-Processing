/**
  ******************************************************************************
  * @file    network.c
  * @author  AST Embedded Analytics Research Platform
  * @date    2026-01-15T18:48:59+0000
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
#define _STAI_NETWORK_MODEL_SIGNATURE     "0xf63c4e1bf77f421087c605a3900b9542"
#define _STAI_NETWORK_DATETIME            "2026-01-15T18:48:59+0000"
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
      STAI_DECLARE_ARRAY(int32_t, 1, 30436),
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
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_6_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_6_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 128),
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
      STAI_DECLARE_ARRAY(int32_t, 1, 128),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_9_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_9_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1024),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_10_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_10_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 128),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_11_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_11_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 288),
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
      STAI_DECLARE_ARRAY(int32_t, 1, 2048),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_14_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_14_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 256),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_15_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_15_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 576),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_16_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_16_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 256),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_17_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_17_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 4096),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_18_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_18_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 256),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_19_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_19_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 576),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_20_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_20_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 256),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_21_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_21_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 8192),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_22_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_22_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_23_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_23_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1152),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_24_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_24_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_25_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_25_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 16384),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_26_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_26_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_27_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_27_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1280),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_28_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_28_SIZE_BYTES,
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
      (stai_ptr)g_network_conv2d_0_weights_array,(stai_ptr)g_network_conv2d_0_bias_array,(stai_ptr)g_network_conv2d_1_weights_array,(stai_ptr)g_network_conv2d_1_bias_array,(stai_ptr)g_network_conv2d_2_weights_array,(stai_ptr)g_network_conv2d_2_bias_array,(stai_ptr)g_network_conv2d_3_weights_array,(stai_ptr)g_network_conv2d_3_bias_array,(stai_ptr)g_network_conv2d_4_weights_array,(stai_ptr)g_network_conv2d_4_bias_array,(stai_ptr)g_network_conv2d_5_weights_array,(stai_ptr)g_network_conv2d_5_bias_array,(stai_ptr)g_network_conv2d_6_weights_array,(stai_ptr)g_network_conv2d_6_bias_array,(stai_ptr)g_network_conv2d_7_weights_array,(stai_ptr)g_network_conv2d_7_bias_array,(stai_ptr)g_network_conv2d_8_weights_array,(stai_ptr)g_network_conv2d_8_bias_array,(stai_ptr)g_network_conv2d_9_weights_array,(stai_ptr)g_network_conv2d_9_bias_array,(stai_ptr)g_network_conv2d_10_weights_array,(stai_ptr)g_network_conv2d_10_bias_array,(stai_ptr)g_network_conv2d_11_weights_array,(stai_ptr)g_network_conv2d_11_bias_array,(stai_ptr)g_network_conv2d_12_weights_array,(stai_ptr)g_network_conv2d_12_bias_array,(stai_ptr)g_network_gemm_14_weights_array,(stai_ptr)g_network_gemm_14_bias_array
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
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_12_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0235294122248888f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #1 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(pool_13_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.012195399031043053f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #2 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_14_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.13701234757900238f),
    AI_PACK_INTQ_ZP(-26)))

/* Int quant #3 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_14_weights_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 10,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.003835520474240184f, 0.003442654386162758f, 0.003998173866420984f, 0.0032646891195327044f, 0.004049011040478945f, 0.0034510826226323843f, 0.0037698615342378616f, 0.004172537941485643f, 0.003694626037031412f, 0.003962635528296232f),
    AI_PACK_INTQ_ZP(0, 0, 0, 0, 0, 0, 0, 0, 0, 0)))



/* Array#0 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_12_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 6272, AI_STATIC)

/* Array#1 */
AI_ARRAY_OBJ_DECLARE(
  pool_13_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 128, AI_STATIC)

/* Array#2 */
AI_ARRAY_OBJ_DECLARE(
  gemm_14_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 10, AI_STATIC)

/* Array#3 */
AI_ARRAY_OBJ_DECLARE(
  gemm_14_weights_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 1280, AI_STATIC)

/* Array#4 */
AI_ARRAY_OBJ_DECLARE(
  gemm_14_bias_array, AI_ARRAY_FORMAT_S32,
  NULL, NULL, 10, AI_STATIC)

/* Array#5 */
AI_ARRAY_OBJ_DECLARE(
  gemm_14_scratch0_array, AI_ARRAY_FORMAT_S16,
  NULL, NULL, 178, AI_STATIC)



/* Tensor #0 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_12_output, AI_STATIC,
  14, 0x1,
  AI_SHAPE_INIT(4, 1, 128, 7, 7), AI_STRIDE_INIT(4, 1, 1, 128, 896),
  1, &conv2d_12_output_array, &conv2d_12_output_array_intq)

/* Tensor #1 */
AI_TENSOR_OBJ_DECLARE(
  pool_13_output, AI_STATIC,
  64, 0x1,
  AI_SHAPE_INIT(4, 1, 128, 1, 1), AI_STRIDE_INIT(4, 1, 1, 128, 128),
  1, &pool_13_output_array, &pool_13_output_array_intq)

/* Tensor #2 */
AI_TENSOR_OBJ_DECLARE(
  gemm_14_bias, AI_STATIC,
  58, 0x0,
  AI_SHAPE_INIT(4, 1, 10, 1, 1), AI_STRIDE_INIT(4, 4, 4, 40, 40),
  1, &gemm_14_bias_array, NULL)

/* Tensor #3 */
AI_TENSOR_OBJ_DECLARE(
  gemm_14_output, AI_STATIC,
  59, 0x1,
  AI_SHAPE_INIT(4, 1, 10, 1, 1), AI_STRIDE_INIT(4, 1, 1, 10, 10),
  1, &gemm_14_output_array, &gemm_14_output_array_intq)

/* Tensor #4 */
AI_TENSOR_OBJ_DECLARE(
  gemm_14_scratch0, AI_STATIC,
  60, 0x0,
  AI_SHAPE_INIT(4, 1, 178, 1, 1), AI_STRIDE_INIT(4, 2, 2, 356, 356),
  1, &gemm_14_scratch0_array, NULL)

/* Tensor #5 */
AI_TENSOR_OBJ_DECLARE(
  gemm_14_weights, AI_STATIC,
  61, 0x1,
  AI_SHAPE_INIT(4, 128, 10, 1, 1), AI_STRIDE_INIT(4, 1, 128, 1280, 1280),
  1, &gemm_14_weights_array, &gemm_14_weights_array_intq)


AI_TENSOR_CHAIN_OBJ_DECLARE(
  pool_13_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_12_output),
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
  .pool_size = AI_SHAPE_2D_INIT(7, 7), 
  .pool_stride = AI_SHAPE_2D_INIT(7, 7), 
  .pool_pad = AI_SHAPE_INIT(4, 0, 0, 0, 0), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  gemm_14_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &pool_13_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gemm_14_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &gemm_14_weights, &gemm_14_bias),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gemm_14_scratch0)
)

AI_LAYER_OBJ_DECLARE(
  gemm_14_layer, 14,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense_integer_SSSA_ch,
  &gemm_14_chain,
  NULL, &gemm_14_layer, AI_STATIC, 
)
/**  Hybrid layers declarations section  *************************************/
void forward_lite_pool_13(_stai_network_context* net_ctx)
{
  conv2d_12_output_array.data = AI_PTR(net_ctx->_activations[0] + 1792);
  conv2d_12_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1792);
  pool_13_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  pool_13_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(13, 1, { conv2d_12_output.data->data});
  forward_ap_integer_INT8(&pool_13_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(13, 1, { pool_13_output.data->data});
}
void forward_lite_gemm_14(_stai_network_context* net_ctx)
{
  pool_13_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  pool_13_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  gemm_14_weights_array.data = AI_PTR(net_ctx->_weights[26] + 0);
  gemm_14_weights_array.data_start = AI_PTR(net_ctx->_weights[26] + 0);
  gemm_14_bias_array.data = AI_PTR(net_ctx->_weights[27] + 0);
  gemm_14_bias_array.data_start = AI_PTR(net_ctx->_weights[27] + 0);
  gemm_14_scratch0_array.data = AI_PTR(net_ctx->_activations[0] + 128);
  gemm_14_scratch0_array.data_start = AI_PTR(net_ctx->_activations[0] + 128);
  gemm_14_output_array.data = AI_PTR(net_ctx->_activations[0] + 484);
  gemm_14_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 484);
  _STAI_NETWORK_EVENT_NODE_START_CB(14, 1, { pool_13_output.data->data});
  forward_dense_integer_SSSA_ch(&gemm_14_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(14, 1, { gemm_14_output.data->data});
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
static const ai_float conv2d_0_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.016631856560707092f, 0.015645483508706093f, 0.021489864215254784f, 0.011345558799803257f, 0.023122070357203484f, 0.015494473278522491f, 0.014307460747659206f, 0.011248513124883175f, 0.008475961163640022f, 0.014635547064244747f, 0.013201148249208927f, 0.011414816603064537f, 0.019759664312005043f, 0.014171683229506016f, 0.012010454200208187f, 0.018505673855543137f);
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
static const ai_float conv2d_1_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.017631670460104942f, 0.012719548307359219f, 0.003459502477198839f, 0.005828699097037315f, 0.0070993732661008835f, 0.003890833118930459f, 0.003917890600860119f, 0.003211350878700614f, 0.016826536506414413f, 0.006902334745973349f, 0.01178794912993908f, 0.01990080624818802f, 0.009074381552636623f, 0.006255020387470722f, 0.015320269390940666f, 0.006278319749981165f);
static const ai_u16 conv2d_1_t_out_0_shape_w_const_u16 = 28;
static const ai_u16 conv2d_1_t_out_0_shape_h_const_u16 = 28;

static const ai_u16 conv2d_2_t_in_0_shape_w_const_u16 = 28;
static const ai_u16 conv2d_2_t_in_0_shape_h_const_u16 = 28;
static const ai_u16 conv2d_2_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_2_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_2_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_2_t_out_0_shape_ch_const_u16 = 32;
static const ai_i8 conv2d_2_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_2_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_2_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_2_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_2_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0055862851440906525f, 0.00812962930649519f, 0.0036249475087970495f, 0.004955493379384279f, 0.005395202897489071f, 0.00586110120639205f, 0.008279701694846153f, 0.005509430076926947f, 0.00629026722162962f, 0.004688826855272055f, 0.00517981918528676f, 0.005946110934019089f, 0.005136845167726278f, 0.004213749431073666f, 0.004723230842500925f, 0.008778613992035389f, 0.0056543173268437386f, 0.006074847187846899f, 0.008851276710629463f, 0.005902639590203762f, 0.007100114598870277f, 0.006425905507057905f, 0.004175057169049978f, 0.003175552934408188f, 0.0057358513586223125f, 0.004351613111793995f, 0.007779660169035196f, 0.00812937505543232f, 0.003813049988821149f, 0.004909753333777189f, 0.009173190221190453f, 0.010791211389005184f);
static const ai_layer_format_type conv2d_2_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_3_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_3_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_3_pad_before_t_in_0_shape_h_const_u32 = 28;

static const ai_u16 conv2d_3_t_in_0_shape_w_const_u16 = 30;
static const ai_u16 conv2d_3_t_in_0_shape_h_const_u16 = 30;
static const ai_u16 conv2d_3_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_3_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_3_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_3_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_3_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_3_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_3_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_3_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.012316988781094551f, 0.007220108527690172f, 0.007407720200717449f, 0.010219315066933632f, 0.0097049530595541f, 0.008247454650700092f, 0.004579163156449795f, 0.00673799030482769f, 0.012764830142259598f, 0.008874264545738697f, 0.004726902581751347f, 0.007052722852677107f, 0.007740688510239124f, 0.005421684123575687f, 0.006715660914778709f, 0.005931289400905371f, 0.003121505957096815f, 0.00835044588893652f, 0.0047228955663740635f, 0.005792939569801092f, 0.0030668729450553656f, 0.004263008013367653f, 0.005664670839905739f, 0.010315963067114353f, 0.004905298352241516f, 0.009617204777896404f, 0.005786566995084286f, 0.0062029133550822735f, 0.011013749055564404f, 0.018895581364631653f, 0.003311851527541876f, 0.004230480641126633f);
static const ai_u16 conv2d_3_t_out_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_3_t_out_0_shape_h_const_u16 = 14;

static const ai_u16 conv2d_4_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_4_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_4_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_4_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_4_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_4_t_out_0_shape_ch_const_u16 = 32;
static const ai_i8 conv2d_4_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_4_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_4_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_4_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_4_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0026600281707942486f, 0.0023787894751876593f, 0.0038180090487003326f, 0.0033616041764616966f, 0.003933456726372242f, 0.002973539289087057f, 0.002613004529848695f, 0.003295578993856907f, 0.0028750374913215637f, 0.0038158423267304897f, 0.0031386371701955795f, 0.0027349756564944983f, 0.004586104303598404f, 0.003745662746950984f, 0.0037594600580632687f, 0.004029849544167519f, 0.004062875639647245f, 0.004782481584697962f, 0.004096385091543198f, 0.002892437856644392f, 0.004816149361431599f, 0.0039049056358635426f, 0.002871881704777479f, 0.004337700549513102f, 0.003968681208789349f, 0.0042157904244959354f, 0.002653757343068719f, 0.0036540075670927763f, 0.003908295650035143f, 0.002982423873618245f, 0.0034137191250920296f, 0.003171968972310424f);
static const ai_layer_format_type conv2d_4_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_5_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_5_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_5_pad_before_t_in_0_shape_h_const_u32 = 14;

static const ai_u16 conv2d_5_t_in_0_shape_w_const_u16 = 16;
static const ai_u16 conv2d_5_t_in_0_shape_h_const_u16 = 16;
static const ai_u16 conv2d_5_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_5_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_5_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_5_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_5_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_5_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_5_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_5_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.011317689903080463f, 0.007022790145128965f, 0.007882785052061081f, 0.0046167513355612755f, 0.005774105899035931f, 0.0052019753493368626f, 0.006429477594792843f, 0.008828799240291119f, 0.008289673365652561f, 0.012212403118610382f, 0.004092074930667877f, 0.0050339712761342525f, 0.008129267953336239f, 0.007184235379099846f, 0.0069421338848769665f, 0.014049968682229519f, 0.0040635475888848305f, 0.004233799874782562f, 0.006266176234930754f, 0.011521119624376297f, 0.005443653557449579f, 0.0063981423154473305f, 0.006416989024728537f, 0.006140426266938448f, 0.006265553180128336f, 0.005329334642738104f, 0.004424900282174349f, 0.006088298745453358f, 0.0073065729811787605f, 0.0052791619673371315f, 0.007768826559185982f, 0.0038730744272470474f);
static const ai_u16 conv2d_5_t_out_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_5_t_out_0_shape_h_const_u16 = 14;

static const ai_u16 conv2d_6_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_6_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_6_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_6_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_6_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_6_t_out_0_shape_ch_const_u16 = 64;
static const ai_i8 conv2d_6_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_6_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_6_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_6_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_6_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0060265203937888145f, 0.003267314750701189f, 0.0031033996492624283f, 0.0044295042753219604f, 0.0034360517747700214f, 0.0031854102853685617f, 0.003518975805491209f, 0.0028640078380703926f, 0.004788654390722513f, 0.0038299644365906715f, 0.0038280608132481575f, 0.004587801638990641f, 0.005974519532173872f, 0.004235770087689161f, 0.0032480901572853327f, 0.003569927765056491f, 0.004208456724882126f, 0.004210974555462599f, 0.004974230658262968f, 0.003109073033556342f, 0.003965709824115038f, 0.0039291903376579285f, 0.0033522227313369513f, 0.004599559120833874f, 0.00295549095608294f, 0.0034906293731182814f, 0.0028189262375235558f, 0.004021151922643185f, 0.00372503069229424f, 0.004500185139477253f, 0.002850012620911002f, 0.003191229421645403f, 0.005014511290937662f, 0.004498068708926439f, 0.00369056174531579f, 0.002760443137958646f, 0.003937104716897011f, 0.00414835661649704f, 0.004585367161780596f, 0.0027196360751986504f, 0.00224333256483078f, 0.0031094211153686047f, 0.0038152174092829227f, 0.003680110676214099f, 0.0038487426936626434f, 0.005076681729406118f, 0.0025267296005040407f, 0.0034605765249580145f, 0.0027693144511431456f, 0.0056005516089499f, 0.0040132333524525166f, 0.0036583184264600277f, 0.004179530311375856f, 0.003840345423668623f, 0.005821210332214832f, 0.003718328895047307f, 0.00428828876465559f, 0.004591037519276142f, 0.004186307080090046f, 0.0038637774996459484f, 0.0033729977440088987f, 0.003517728066071868f, 0.00422630226239562f, 0.006051977630704641f);
static const ai_layer_format_type conv2d_6_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_7_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_7_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_7_pad_before_t_in_0_shape_h_const_u32 = 14;

static const ai_u16 conv2d_7_t_in_0_shape_w_const_u16 = 16;
static const ai_u16 conv2d_7_t_in_0_shape_h_const_u16 = 16;
static const ai_u16 conv2d_7_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_7_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_7_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_7_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_7_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_7_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_7_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_7_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0037742152344435453f, 0.0036582015454769135f, 0.009181046858429909f, 0.008349807001650333f, 0.008662843145430088f, 0.004401044454425573f, 0.004502745345234871f, 0.006270622368901968f, 0.007468298077583313f, 0.00703441072255373f, 0.006932588294148445f, 0.0036764214746654034f, 0.006164835300296545f, 0.00422286381945014f, 0.006445667706429958f, 0.0074564674869179726f, 0.005883366335183382f, 0.0050619193352758884f, 0.002758993301540613f, 0.015974704176187515f, 0.0057168444618582726f, 0.005520970094949007f, 0.007549686823040247f, 0.006593544501811266f, 0.006630854681134224f, 0.005267704371362925f, 0.008578083477914333f, 0.006913649383932352f, 0.007863075472414494f, 0.005145752336829901f, 0.006433514412492514f, 0.009186416864395142f, 0.005830648820847273f, 0.005174259189516306f, 0.01071979757398367f, 0.004221739713102579f, 0.005382535047829151f, 0.011358464136719704f, 0.006253756117075682f, 0.00963882077485323f, 0.006554109510034323f, 0.005289684981107712f, 0.006543981377035379f, 0.004310406744480133f, 0.00561669422313571f, 0.004641838371753693f, 0.008516104891896248f, 0.0061342972330749035f, 0.0028171890880912542f, 0.004444940481334925f, 0.004572034813463688f, 0.010022951290011406f, 0.004458628594875336f, 0.008193478919565678f, 0.007201366126537323f, 0.008109231479465961f, 0.006215026136487722f, 0.0054930527694523335f, 0.006607845891267061f, 0.009061993099749088f, 0.004098611883819103f, 0.005055071320384741f, 0.007188564166426659f, 0.0043256403878331184f);
static const ai_u16 conv2d_7_t_out_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_7_t_out_0_shape_h_const_u16 = 7;

static const ai_u16 conv2d_8_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_8_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_8_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_8_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_8_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_8_t_out_0_shape_ch_const_u16 = 64;
static const ai_i8 conv2d_8_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_8_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_8_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_8_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_8_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.00326141738332808f, 0.002109822118654847f, 0.0027309395372867584f, 0.00313995941542089f, 0.002902975305914879f, 0.0030252293217927217f, 0.002546448027715087f, 0.0031965943053364754f, 0.003045730059966445f, 0.0024326839484274387f, 0.0032316900324076414f, 0.003123925067484379f, 0.002676204778254032f, 0.0024065584875643253f, 0.003122723428532481f, 0.0024914350360631943f, 0.0031837697606533766f, 0.003483945271000266f, 0.002441286575049162f, 0.0033711723517626524f, 0.002540219807997346f, 0.0034621800296008587f, 0.003416850231587887f, 0.002629409544169903f, 0.002664838684722781f, 0.0018948061624541879f, 0.0024783071130514145f, 0.003182625165209174f, 0.0021321987733244896f, 0.002590169431641698f, 0.0026075730565935373f, 0.0024359768722206354f, 0.0042670792900025845f, 0.002429279265925288f, 0.0034218700602650642f, 0.0028430535458028316f, 0.0017166166799142957f, 0.0037065723445266485f, 0.0028151636943221092f, 0.003090986516326666f, 0.00279137771576643f, 0.002051996299996972f, 0.0023037921637296677f, 0.002851533005014062f, 0.003713448764756322f, 0.0026432713493704796f, 0.0031878906302154064f, 0.0027264785021543503f, 0.00305666564963758f, 0.0031239890959113836f, 0.0021576492581516504f, 0.003189935116097331f, 0.0036312867887318134f, 0.002228855388239026f, 0.0026393223088234663f, 0.0030669982079416513f, 0.0032160822302103043f, 0.002841710578650236f, 0.003160269698128104f, 0.0027546186465770006f, 0.0024798153899610043f, 0.003208933863788843f, 0.0021575025748461485f, 0.0018012503860518336f);
static const ai_layer_format_type conv2d_8_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_9_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_9_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_9_pad_before_t_in_0_shape_h_const_u32 = 7;

static const ai_u16 conv2d_9_t_in_0_shape_w_const_u16 = 9;
static const ai_u16 conv2d_9_t_in_0_shape_h_const_u16 = 9;
static const ai_u16 conv2d_9_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_9_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_9_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_9_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_9_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_9_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_9_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_9_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.007993829436600208f, 0.009542692452669144f, 0.004514975473284721f, 0.005105510354042053f, 0.010525934398174286f, 0.005955362692475319f, 0.008680155500769615f, 0.005695235915482044f, 0.009763980284333229f, 0.005284619051963091f, 0.0064674485474824905f, 0.005358987022191286f, 0.009277098812162876f, 0.004881821572780609f, 0.008435280062258244f, 0.005081870127469301f, 0.00806943979114294f, 0.009734800085425377f, 0.00633216230198741f, 0.003450399497523904f, 0.013499671593308449f, 0.007596201729029417f, 0.006581957917660475f, 0.00890861265361309f, 0.007714326027780771f, 0.014972737058997154f, 0.00734878983348608f, 0.008747904561460018f, 0.01280156895518303f, 0.007989821024239063f, 0.006043364759534597f, 0.005321776028722525f, 0.00661778450012207f, 0.014145582914352417f, 0.0037234860938042402f, 0.005253336858004332f, 0.0058521428145468235f, 0.00675569474697113f, 0.008232205174863338f, 0.007715845946222544f, 0.008451228961348534f, 0.006569020915776491f, 0.006954765412956476f, 0.0061295730993151665f, 0.007030684966593981f, 0.004956505727022886f, 0.004445059224963188f, 0.006357397418469191f, 0.009766019880771637f, 0.005152205936610699f, 0.006711211986839771f, 0.007464773021638393f, 0.006185928359627724f, 0.009157666936516762f, 0.007907429710030556f, 0.00850497093051672f, 0.011073896661400795f, 0.005258189979940653f, 0.008996179327368736f, 0.005784939043223858f, 0.006255338899791241f, 0.005285923834890127f, 0.010449416004121304f, 0.009725787676870823f);
static const ai_u16 conv2d_9_t_out_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_9_t_out_0_shape_h_const_u16 = 7;

static const ai_u16 conv2d_10_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_10_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_10_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_10_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_10_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_10_t_out_0_shape_ch_const_u16 = 128;
static const ai_i8 conv2d_10_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_10_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_10_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_10_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_10_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0033220830373466015f, 0.00469374330714345f, 0.0046712495386600494f, 0.004251891281455755f, 0.002772651147097349f, 0.003099048975855112f, 0.0038558333180844784f, 0.0027469806373119354f, 0.0032248999923467636f, 0.0029705532360821962f, 0.003040679730474949f, 0.003421126864850521f, 0.002709027612581849f, 0.0026349348481744528f, 0.005335696507245302f, 0.004133170936256647f, 0.0035315777640789747f, 0.004855698440223932f, 0.003962740767747164f, 0.0033324691466987133f, 0.003581294557079673f, 0.003364172065630555f, 0.003560260869562626f, 0.0032941570971161127f, 0.004062470979988575f, 0.0037631778977811337f, 0.003255341202020645f, 0.004747927188873291f, 0.002157678594812751f, 0.0041162967681884766f, 0.004288696218281984f, 0.0044030784629285336f, 0.0025568834971636534f, 0.0034110581036657095f, 0.003834170987829566f, 0.0031248691957443953f, 0.004107858520001173f, 0.004742057994008064f, 0.002463086973875761f, 0.004451145883649588f, 0.0045021940022706985f, 0.0024240720085799694f, 0.0036252057179808617f, 0.003201194806024432f, 0.0031580342911183834f, 0.0026706024073064327f, 0.0030149950180202723f, 0.0029445618856698275f, 0.0044137113727629185f, 0.00488372053951025f, 0.004239269066601992f, 0.0030148124787956476f, 0.003419482847675681f, 0.004903580993413925f, 0.004526522010564804f, 0.0026191032957285643f, 0.0024024874437600374f, 0.003812540555372834f, 0.005031639710068703f, 0.0030690624844282866f, 0.0033463395666331053f, 0.0032964947167783976f, 0.0036038898397237062f, 0.003431529738008976f, 0.0039019619580358267f, 0.0025228981394320726f, 0.00414228392764926f, 0.0029468636494129896f, 0.004573025740683079f, 0.004039141349494457f, 0.0036023352295160294f, 0.00438155559822917f, 0.002790958620607853f, 0.003505564061924815f, 0.0037365001626312733f, 0.0033592297695577145f, 0.003486590227112174f, 0.0030535117257386446f, 0.003989721182733774f, 0.0023547732271254063f, 0.0028339766431599855f, 0.00402877340093255f, 0.0027677579782903194f, 0.002714718459174037f, 0.0035815804731100798f, 0.002734233159571886f, 0.004409090615808964f, 0.0030990999657660723f, 0.0033107015769928694f, 0.00395508436486125f, 0.0033103704918175936f, 0.003167959162965417f, 0.004138485994189978f, 0.003449540352448821f, 0.0032881246879696846f, 0.003647939534857869f, 0.0031659032683819532f, 0.0027353677432984114f, 0.003585922997444868f, 0.0030903422739356756f, 0.002681154990568757f, 0.0032082756515592337f, 0.003338603302836418f, 0.0030819487292319536f, 0.0028098905459046364f, 0.003990954719483852f, 0.004492198117077351f, 0.0035463206004351377f, 0.004717727191746235f, 0.003870373358950019f, 0.0034956836607307196f, 0.0026408019475638866f, 0.0034808048512786627f, 0.002560179214924574f, 0.0033398547675460577f, 0.0034004405606538057f, 0.002850551391020417f, 0.0033353485632687807f, 0.004009234253317118f, 0.0029620942659676075f, 0.0028503474313765764f, 0.004860762972384691f, 0.003535917727276683f, 0.0037941313348710537f, 0.0036503414157778025f, 0.0030077046249061823f, 0.0033507554326206446f, 0.003827781416475773f);
static const ai_layer_format_type conv2d_10_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_11_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_11_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_11_pad_before_t_in_0_shape_h_const_u32 = 7;

static const ai_u16 conv2d_11_t_in_0_shape_w_const_u16 = 9;
static const ai_u16 conv2d_11_t_in_0_shape_h_const_u16 = 9;
static const ai_u16 conv2d_11_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_11_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_11_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_11_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_11_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_11_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_11_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_11_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.005002518650144339f, 0.0035889630671590567f, 0.0040745907463133335f, 0.0039761546067893505f, 0.003426605835556984f, 0.005393620114773512f, 0.00605353619903326f, 0.01160826813429594f, 0.007692322600632906f, 0.005927375052124262f, 0.0040303622372448444f, 0.00625165831297636f, 0.00476936437189579f, 0.00757654057815671f, 0.005194855388253927f, 0.003840639255940914f, 0.005928208585828543f, 0.005518753081560135f, 0.0034851348027586937f, 0.004872217774391174f, 0.004782097414135933f, 0.0045281364582479f, 0.008423853665590286f, 0.005278605967760086f, 0.005048992577940226f, 0.0036789688747376204f, 0.003985997289419174f, 0.008556832559406757f, 0.004623401444405317f, 0.00453969556838274f, 0.00757019966840744f, 0.005239690653979778f, 0.0057610743679106236f, 0.005025077611207962f, 0.005732075776904821f, 0.006142402533441782f, 0.0066034081391990185f, 0.0038426623214036226f, 0.005473630968481302f, 0.006180092226713896f, 0.005871961824595928f, 0.0072052436880767345f, 0.004663863219320774f, 0.002947490196675062f, 0.008268830366432667f, 0.004207253456115723f, 0.009320233017206192f, 0.00792867410928011f, 0.005531846079975367f, 0.008467819541692734f, 0.005994909908622503f, 0.007201637141406536f, 0.008019582368433475f, 0.006355046294629574f, 0.007456695195287466f, 0.004341169260442257f, 0.00966614205390215f, 0.008229680359363556f, 0.005322094541043043f, 0.00785135105252266f, 0.004365625791251659f, 0.010391429997980595f, 0.0033711493015289307f, 0.005461636930704117f, 0.006438062526285648f, 0.005862241145223379f, 0.004975253716111183f, 0.0057851215824484825f, 0.005735159385949373f, 0.0065146819688379765f, 0.004735841881483793f, 0.009740674868226051f, 0.0036090672947466373f, 0.007219132501631975f, 0.009565549902617931f, 0.005600962322205305f, 0.00487772049382329f, 0.006702912971377373f, 0.00453862315043807f, 0.008209921419620514f, 0.008872317150235176f, 0.007175838109105825f, 0.013203340582549572f, 0.00608463678508997f, 0.0033008616883307695f, 0.0047670588828623295f, 0.004557324573397636f, 0.012976760976016521f, 0.0059942458756268024f, 0.005062103737145662f, 0.005223679356276989f, 0.004012039862573147f, 0.008191426284611225f, 0.004372403956949711f, 0.0037229161243885756f, 0.006269266363233328f, 0.004562954418361187f, 0.006945926230400801f, 0.0065505350939929485f, 0.0065203518606722355f, 0.01152438297867775f, 0.008551955223083496f, 0.0066978298127651215f, 0.007063583936542273f, 0.005488029681146145f, 0.005031961016356945f, 0.004654948599636555f, 0.005116392392665148f, 0.005898959469050169f, 0.0047799814492464066f, 0.006993359886109829f, 0.005618932656943798f, 0.005995535291731358f, 0.004002466797828674f, 0.009968079626560211f, 0.005706842988729477f, 0.006858022417873144f, 0.0037753265351057053f, 0.006169829983264208f, 0.009811819531023502f, 0.006884577218443155f, 0.003912453539669514f, 0.005038685165345669f, 0.005559736862778664f, 0.0053998613730072975f, 0.006529853213578463f, 0.005089144222438335f, 0.004805389326065779f);
static const ai_u16 conv2d_11_t_out_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_11_t_out_0_shape_h_const_u16 = 7;

static const ai_u16 conv2d_12_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_12_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_12_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_12_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_12_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_12_t_out_0_shape_ch_const_u16 = 128;
static const ai_i8 conv2d_12_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_12_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_12_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_12_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_12_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0035385906230658293f, 0.00478470791131258f, 0.0033901669085025787f, 0.003767494112253189f, 0.0038646808825433254f, 0.004339620471000671f, 0.004334621597081423f, 0.003541473299264908f, 0.004176007583737373f, 0.0048954966478049755f, 0.003519848920404911f, 0.003790206741541624f, 0.0032462370581924915f, 0.003054178087040782f, 0.0031569518614560366f, 0.003966407384723425f, 0.003325240919366479f, 0.003966724965721369f, 0.0041352855041623116f, 0.004332618787884712f, 0.004604259040206671f, 0.0026958466041833162f, 0.0028604536782950163f, 0.0032458093483000994f, 0.003226612228900194f, 0.004055128898471594f, 0.0038684173487126827f, 0.003779167775064707f, 0.0031643081456422806f, 0.003458953695371747f, 0.0031696499790996313f, 0.0032418821938335896f, 0.0036609626840800047f, 0.0037525102961808443f, 0.0032856285106390715f, 0.003330359933897853f, 0.0032517320942133665f, 0.0037703136913478374f, 0.003759918501600623f, 0.0034770111087709665f, 0.004659817088395357f, 0.0036982877645641565f, 0.003866775892674923f, 0.003872094675898552f, 0.004073974676430225f, 0.0036046854220330715f, 0.003792225383222103f, 0.003078261623159051f, 0.0037252474576234818f, 0.004292150493711233f, 0.003085298463702202f, 0.004243828821927309f, 0.003515056101605296f, 0.003140663728117943f, 0.0032676332630217075f, 0.003848901018500328f, 0.004837517160922289f, 0.0033449626062065363f, 0.003706097835674882f, 0.0038588966708630323f, 0.0033067299518734217f, 0.003349836217239499f, 0.003896715585142374f, 0.003869440406560898f, 0.004055653233081102f, 0.0037652316968888044f, 0.0030530961230397224f, 0.0031822051387280226f, 0.0032712984830141068f, 0.0028226077556610107f, 0.003737379563972354f, 0.0039787134155631065f, 0.003927130252122879f, 0.0035412167198956013f, 0.0032662705052644014f, 0.003796761855483055f, 0.00352372694760561f, 0.0033204187639057636f, 0.003908132668584585f, 0.003829477820545435f, 0.0036051813513040543f, 0.0038467717822641134f, 0.0033177288714796305f, 0.0036774014588445425f, 0.003423696616664529f, 0.0037254909984767437f, 0.0032543078996241093f, 0.003722229739651084f, 0.004069639835506678f, 0.00428022863343358f, 0.0038336203433573246f, 0.003318407339975238f, 0.0029798573814332485f, 0.0037228430155664682f, 0.0033511831425130367f, 0.0038271890953183174f, 0.003888737643137574f, 0.003185979323461652f, 0.00369390775449574f, 0.00382672599516809f, 0.0034937551245093346f, 0.003341092262417078f, 0.003664181334897876f, 0.00401141494512558f, 0.0037474220152944326f, 0.0048959157429635525f, 0.0036706416867673397f, 0.0027530884835869074f, 0.0032034111209213734f, 0.0047203716821968555f, 0.003965391311794519f, 0.0036479008849710226f, 0.0031161296647042036f, 0.004280665423721075f, 0.005551006179302931f, 0.002879900159314275f, 0.00387416104786098f, 0.0036391073372215033f, 0.004418122116476297f, 0.0043647135607898235f, 0.003412629012018442f, 0.0036856657825410366f, 0.003047662554308772f, 0.004003593698143959f, 0.003543794620782137f, 0.0032549931202083826f, 0.003357733367010951f, 0.004525919444859028f);
static const ai_layer_format_type conv2d_12_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;



static const ai_u32 nl_15_t_in_0_shape_ch_prod_const_u32 = 10;
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
    ai_i8* conv2d_0_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 17600);
    ai_i16* conv2d_0_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 17052);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(0, 1, {(stai_ptr) conv2d_0_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_sssa8_ch(conv2d_0_t_in_0_ptr_const_s8, conv2d_0_t_in_0_shape_w_const_u16, conv2d_0_t_in_0_shape_h_const_u16, conv2d_0_t_in_0_shape_ch_const_u16, conv2d_0_t_weight_0_ptr_const_s8, conv2d_0_t_out_0_shape_ch_const_u16, conv2d_0_t_weight_0_shape_w_const_u16, conv2d_0_t_weight_0_shape_h_const_u16, conv2d_0_l_stride_1_const_u16, conv2d_0_l_stride_0_const_u16, conv2d_0_l_pad_W_0_const_s32, conv2d_0_l_pad_H_0_const_s32, conv2d_0_t_weight_1_ptr_const_s32, conv2d_0_t_in_0_fmt_zero_const_s8, conv2d_0_t_out_0_fmt_zero_const_s8, conv2d_0_t_in_0_fmt_scale_const_f32, conv2d_0_t_out_0_fmt_scale_const_f32, conv2d_0_t_weight_0_fmt_scale_const_f32, conv2d_0_l_out_ch_format_const_layer_format_type, conv2d_0_t_out_0_ptr_s8, conv2d_0_t_out_0_shape_w_const_u16, conv2d_0_t_out_0_shape_h_const_u16, 1, 548, conv2d_0_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(0, 1, {(stai_ptr) conv2d_0_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_0 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_1_pad_before */
  {
      const ai_ptr conv2d_1_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 17600);
    ai_ptr conv2d_1_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 3200);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(1, 1, {(stai_ptr) conv2d_1_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_1_pad_before_t_in_0_ptr_const_ptr, conv2d_1_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_1_pad_before_v_pad_constant_value_const_s8), conv2d_1_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_1_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(448), (ai_i32)(480), (ai_i32)(480), (ai_i32)(16), (ai_i32)(16));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(1, 1, {(stai_ptr) conv2d_1_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_1_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_1 */
  {
      const ai_i8* conv2d_1_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 3200);
    const ai_i8* conv2d_1_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[2] + 0);
    const ai_i32* conv2d_1_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[3] + 0);
    ai_i8* conv2d_1_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 17600);
    ai_i16* conv2d_1_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 2604);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(1, 1, {(stai_ptr) conv2d_1_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_1_t_in_0_ptr_const_s8, conv2d_1_t_in_0_shape_w_const_u16, conv2d_1_t_in_0_shape_h_const_u16, conv2d_1_t_in_0_shape_ch_const_u16, conv2d_1_t_weight_0_ptr_const_s8, conv2d_1_l_stride_1_const_u16, conv2d_1_l_stride_0_const_u16, conv2d_1_t_weight_1_ptr_const_s32, conv2d_1_t_in_0_fmt_zero_const_s8, conv2d_1_t_out_0_fmt_zero_const_s8, conv2d_1_t_in_0_fmt_scale_const_f32, conv2d_1_t_out_0_fmt_scale_const_f32, conv2d_1_t_weight_0_fmt_scale_const_f32, conv2d_1_t_out_0_ptr_s8, conv2d_1_t_out_0_shape_w_const_u16, conv2d_1_t_out_0_shape_h_const_u16, 0, 593, conv2d_1_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(1, 1, {(stai_ptr) conv2d_1_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_1 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_2 */
  {
      const ai_i8* conv2d_2_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 17600);
    const ai_i8* conv2d_2_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[4] + 0);
    const ai_i32* conv2d_2_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[5] + 0);
    ai_i8* conv2d_2_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 4160);
    ai_i16* conv2d_2_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 2604);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(2, 1, {(stai_ptr) conv2d_2_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_2_t_in_0_ptr_const_s8, conv2d_2_t_in_0_shape_w_const_u16, conv2d_2_t_in_0_shape_h_const_u16, conv2d_2_l_stride_1_const_u16, conv2d_2_l_stride_0_const_u16, conv2d_2_t_in_0_shape_ch_const_u16, conv2d_2_t_weight_0_ptr_const_s8, conv2d_2_t_out_0_shape_ch_const_u16, conv2d_2_t_weight_1_ptr_const_s32, conv2d_2_t_in_0_fmt_zero_const_s8, conv2d_2_t_out_0_fmt_zero_const_s8, conv2d_2_t_in_0_fmt_scale_const_f32, conv2d_2_t_out_0_fmt_scale_const_f32, conv2d_2_t_weight_0_fmt_scale_const_f32, conv2d_2_l_out_ch_format_const_layer_format_type, conv2d_2_t_out_0_ptr_s8, 1, 384, conv2d_2_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(2, 1, {(stai_ptr) conv2d_2_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_2 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_3_pad_before */
  {
      const ai_ptr conv2d_3_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 4160);
    ai_ptr conv2d_3_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 448);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(3, 1, {(stai_ptr) conv2d_3_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_3_pad_before_t_in_0_ptr_const_ptr, conv2d_3_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_3_pad_before_v_pad_constant_value_const_s8), conv2d_3_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_3_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(896), (ai_i32)(0), (ai_i32)(1920), (ai_i32)(0), (ai_i32)(64));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(3, 1, {(stai_ptr) conv2d_3_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_3_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_3 */
  {
      const ai_i8* conv2d_3_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 448);
    const ai_i8* conv2d_3_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[6] + 0);
    const ai_i32* conv2d_3_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[7] + 0);
    ai_i8* conv2d_3_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_3_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 29248);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(3, 1, {(stai_ptr) conv2d_3_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_3_t_in_0_ptr_const_s8, conv2d_3_t_in_0_shape_w_const_u16, conv2d_3_t_in_0_shape_h_const_u16, conv2d_3_t_in_0_shape_ch_const_u16, conv2d_3_t_weight_0_ptr_const_s8, conv2d_3_l_stride_1_const_u16, conv2d_3_l_stride_0_const_u16, conv2d_3_t_weight_1_ptr_const_s32, conv2d_3_t_in_0_fmt_zero_const_s8, conv2d_3_t_out_0_fmt_zero_const_s8, conv2d_3_t_in_0_fmt_scale_const_f32, conv2d_3_t_out_0_fmt_scale_const_f32, conv2d_3_t_weight_0_fmt_scale_const_f32, conv2d_3_t_out_0_ptr_s8, conv2d_3_t_out_0_shape_w_const_u16, conv2d_3_t_out_0_shape_h_const_u16, 0, 1185, conv2d_3_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(3, 1, {(stai_ptr) conv2d_3_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_3 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_4 */
  {
      const ai_i8* conv2d_4_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_4_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[8] + 0);
    const ai_i32* conv2d_4_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[9] + 0);
    ai_i8* conv2d_4_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 6720);
    ai_i16* conv2d_4_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 6272);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(4, 1, {(stai_ptr) conv2d_4_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_4_t_in_0_ptr_const_s8, conv2d_4_t_in_0_shape_w_const_u16, conv2d_4_t_in_0_shape_h_const_u16, conv2d_4_l_stride_1_const_u16, conv2d_4_l_stride_0_const_u16, conv2d_4_t_in_0_shape_ch_const_u16, conv2d_4_t_weight_0_ptr_const_s8, conv2d_4_t_out_0_shape_ch_const_u16, conv2d_4_t_weight_1_ptr_const_s32, conv2d_4_t_in_0_fmt_zero_const_s8, conv2d_4_t_out_0_fmt_zero_const_s8, conv2d_4_t_in_0_fmt_scale_const_f32, conv2d_4_t_out_0_fmt_scale_const_f32, conv2d_4_t_weight_0_fmt_scale_const_f32, conv2d_4_l_out_ch_format_const_layer_format_type, conv2d_4_t_out_0_ptr_s8, 1, 448, conv2d_4_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(4, 1, {(stai_ptr) conv2d_4_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_4 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_5_pad_before */
  {
      const ai_ptr conv2d_5_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 6720);
    ai_ptr conv2d_5_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 12992);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(5, 1, {(stai_ptr) conv2d_5_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_5_pad_before_t_in_0_ptr_const_ptr, conv2d_5_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_5_pad_before_v_pad_constant_value_const_s8), conv2d_5_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_5_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(448), (ai_i32)(512), (ai_i32)(512), (ai_i32)(32), (ai_i32)(32));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(5, 1, {(stai_ptr) conv2d_5_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_5_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_5 */
  {
      const ai_i8* conv2d_5_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 12992);
    const ai_i8* conv2d_5_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[10] + 0);
    const ai_i32* conv2d_5_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[11] + 0);
    ai_i8* conv2d_5_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1188);
    ai_i16* conv2d_5_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(5, 1, {(stai_ptr) conv2d_5_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_5_t_in_0_ptr_const_s8, conv2d_5_t_in_0_shape_w_const_u16, conv2d_5_t_in_0_shape_h_const_u16, conv2d_5_t_in_0_shape_ch_const_u16, conv2d_5_t_weight_0_ptr_const_s8, conv2d_5_l_stride_1_const_u16, conv2d_5_l_stride_0_const_u16, conv2d_5_t_weight_1_ptr_const_s32, conv2d_5_t_in_0_fmt_zero_const_s8, conv2d_5_t_out_0_fmt_zero_const_s8, conv2d_5_t_in_0_fmt_scale_const_f32, conv2d_5_t_out_0_fmt_scale_const_f32, conv2d_5_t_weight_0_fmt_scale_const_f32, conv2d_5_t_out_0_ptr_s8, conv2d_5_t_out_0_shape_w_const_u16, conv2d_5_t_out_0_shape_h_const_u16, 0, 1185, conv2d_5_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(5, 1, {(stai_ptr) conv2d_5_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_5 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_6 */
  {
      const ai_i8* conv2d_6_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1188);
    const ai_i8* conv2d_6_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[12] + 0);
    const ai_i32* conv2d_6_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[13] + 0);
    ai_i8* conv2d_6_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 7460);
    ai_i16* conv2d_6_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(6, 1, {(stai_ptr) conv2d_6_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_6_t_in_0_ptr_const_s8, conv2d_6_t_in_0_shape_w_const_u16, conv2d_6_t_in_0_shape_h_const_u16, conv2d_6_l_stride_1_const_u16, conv2d_6_l_stride_0_const_u16, conv2d_6_t_in_0_shape_ch_const_u16, conv2d_6_t_weight_0_ptr_const_s8, conv2d_6_t_out_0_shape_ch_const_u16, conv2d_6_t_weight_1_ptr_const_s32, conv2d_6_t_in_0_fmt_zero_const_s8, conv2d_6_t_out_0_fmt_zero_const_s8, conv2d_6_t_in_0_fmt_scale_const_f32, conv2d_6_t_out_0_fmt_scale_const_f32, conv2d_6_t_weight_0_fmt_scale_const_f32, conv2d_6_l_out_ch_format_const_layer_format_type, conv2d_6_t_out_0_ptr_s8, 1, 768, conv2d_6_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(6, 1, {(stai_ptr) conv2d_6_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_6 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_7_pad_before */
  {
      const ai_ptr conv2d_7_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 7460);
    ai_ptr conv2d_7_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 3620);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(7, 1, {(stai_ptr) conv2d_7_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_7_pad_before_t_in_0_ptr_const_ptr, conv2d_7_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_7_pad_before_v_pad_constant_value_const_s8), conv2d_7_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_7_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(896), (ai_i32)(0), (ai_i32)(2048), (ai_i32)(0), (ai_i32)(128));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(7, 1, {(stai_ptr) conv2d_7_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_7_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_7 */
  {
      const ai_i8* conv2d_7_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 3620);
    const ai_i8* conv2d_7_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[14] + 0);
    const ai_i32* conv2d_7_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[15] + 0);
    ai_i8* conv2d_7_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 20004);
    ai_i16* conv2d_7_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(7, 1, {(stai_ptr) conv2d_7_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_7_t_in_0_ptr_const_s8, conv2d_7_t_in_0_shape_w_const_u16, conv2d_7_t_in_0_shape_h_const_u16, conv2d_7_t_in_0_shape_ch_const_u16, conv2d_7_t_weight_0_ptr_const_s8, conv2d_7_l_stride_1_const_u16, conv2d_7_l_stride_0_const_u16, conv2d_7_t_weight_1_ptr_const_s32, conv2d_7_t_in_0_fmt_zero_const_s8, conv2d_7_t_out_0_fmt_zero_const_s8, conv2d_7_t_in_0_fmt_scale_const_f32, conv2d_7_t_out_0_fmt_scale_const_f32, conv2d_7_t_weight_0_fmt_scale_const_f32, conv2d_7_t_out_0_ptr_s8, conv2d_7_t_out_0_shape_w_const_u16, conv2d_7_t_out_0_shape_h_const_u16, 0, 2369, conv2d_7_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(7, 1, {(stai_ptr) conv2d_7_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_7 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_8 */
  {
      const ai_i8* conv2d_8_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 20004);
    const ai_i8* conv2d_8_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[16] + 0);
    const ai_i32* conv2d_8_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[17] + 0);
    ai_i8* conv2d_8_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 896);
    ai_i16* conv2d_8_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(8, 1, {(stai_ptr) conv2d_8_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_8_t_in_0_ptr_const_s8, conv2d_8_t_in_0_shape_w_const_u16, conv2d_8_t_in_0_shape_h_const_u16, conv2d_8_l_stride_1_const_u16, conv2d_8_l_stride_0_const_u16, conv2d_8_t_in_0_shape_ch_const_u16, conv2d_8_t_weight_0_ptr_const_s8, conv2d_8_t_out_0_shape_ch_const_u16, conv2d_8_t_weight_1_ptr_const_s32, conv2d_8_t_in_0_fmt_zero_const_s8, conv2d_8_t_out_0_fmt_zero_const_s8, conv2d_8_t_in_0_fmt_scale_const_f32, conv2d_8_t_out_0_fmt_scale_const_f32, conv2d_8_t_weight_0_fmt_scale_const_f32, conv2d_8_l_out_ch_format_const_layer_format_type, conv2d_8_t_out_0_ptr_s8, 1, 896, conv2d_8_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(8, 1, {(stai_ptr) conv2d_8_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_8 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_9_pad_before */
  {
      const ai_ptr conv2d_9_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 896);
    ai_ptr conv2d_9_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 4032);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(9, 1, {(stai_ptr) conv2d_9_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_9_pad_before_t_in_0_ptr_const_ptr, conv2d_9_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_9_pad_before_v_pad_constant_value_const_s8), conv2d_9_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_9_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(448), (ai_i32)(576), (ai_i32)(576), (ai_i32)(64), (ai_i32)(64));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(9, 1, {(stai_ptr) conv2d_9_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_9_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_9 */
  {
      const ai_i8* conv2d_9_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 4032);
    const ai_i8* conv2d_9_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[18] + 0);
    const ai_i32* conv2d_9_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[19] + 0);
    ai_i8* conv2d_9_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 9216);
    ai_i16* conv2d_9_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(9, 1, {(stai_ptr) conv2d_9_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_9_t_in_0_ptr_const_s8, conv2d_9_t_in_0_shape_w_const_u16, conv2d_9_t_in_0_shape_h_const_u16, conv2d_9_t_in_0_shape_ch_const_u16, conv2d_9_t_weight_0_ptr_const_s8, conv2d_9_l_stride_1_const_u16, conv2d_9_l_stride_0_const_u16, conv2d_9_t_weight_1_ptr_const_s32, conv2d_9_t_in_0_fmt_zero_const_s8, conv2d_9_t_out_0_fmt_zero_const_s8, conv2d_9_t_in_0_fmt_scale_const_f32, conv2d_9_t_out_0_fmt_scale_const_f32, conv2d_9_t_weight_0_fmt_scale_const_f32, conv2d_9_t_out_0_ptr_s8, conv2d_9_t_out_0_shape_w_const_u16, conv2d_9_t_out_0_shape_h_const_u16, 0, 2369, conv2d_9_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(9, 1, {(stai_ptr) conv2d_9_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_9 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_10 */
  {
      const ai_i8* conv2d_10_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 9216);
    const ai_i8* conv2d_10_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[20] + 0);
    const ai_i32* conv2d_10_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[21] + 0);
    ai_i8* conv2d_10_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1536);
    ai_i16* conv2d_10_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(10, 1, {(stai_ptr) conv2d_10_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_10_t_in_0_ptr_const_s8, conv2d_10_t_in_0_shape_w_const_u16, conv2d_10_t_in_0_shape_h_const_u16, conv2d_10_l_stride_1_const_u16, conv2d_10_l_stride_0_const_u16, conv2d_10_t_in_0_shape_ch_const_u16, conv2d_10_t_weight_0_ptr_const_s8, conv2d_10_t_out_0_shape_ch_const_u16, conv2d_10_t_weight_1_ptr_const_s32, conv2d_10_t_in_0_fmt_zero_const_s8, conv2d_10_t_out_0_fmt_zero_const_s8, conv2d_10_t_in_0_fmt_scale_const_f32, conv2d_10_t_out_0_fmt_scale_const_f32, conv2d_10_t_weight_0_fmt_scale_const_f32, conv2d_10_l_out_ch_format_const_layer_format_type, conv2d_10_t_out_0_ptr_s8, 1, 1536, conv2d_10_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(10, 1, {(stai_ptr) conv2d_10_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_10 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_11_pad_before */
  {
      const ai_ptr conv2d_11_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 1536);
    ai_ptr conv2d_11_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 7808);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(11, 1, {(stai_ptr) conv2d_11_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_11_pad_before_t_in_0_ptr_const_ptr, conv2d_11_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_11_pad_before_v_pad_constant_value_const_s8), conv2d_11_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_11_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(896), (ai_i32)(1152), (ai_i32)(1152), (ai_i32)(128), (ai_i32)(128));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(11, 1, {(stai_ptr) conv2d_11_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_11_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_11 */
  {
      const ai_i8* conv2d_11_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 7808);
    const ai_i8* conv2d_11_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[22] + 0);
    const ai_i32* conv2d_11_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[23] + 0);
    ai_i8* conv2d_11_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 18176);
    ai_i16* conv2d_11_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(11, 1, {(stai_ptr) conv2d_11_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_11_t_in_0_ptr_const_s8, conv2d_11_t_in_0_shape_w_const_u16, conv2d_11_t_in_0_shape_h_const_u16, conv2d_11_t_in_0_shape_ch_const_u16, conv2d_11_t_weight_0_ptr_const_s8, conv2d_11_l_stride_1_const_u16, conv2d_11_l_stride_0_const_u16, conv2d_11_t_weight_1_ptr_const_s32, conv2d_11_t_in_0_fmt_zero_const_s8, conv2d_11_t_out_0_fmt_zero_const_s8, conv2d_11_t_in_0_fmt_scale_const_f32, conv2d_11_t_out_0_fmt_scale_const_f32, conv2d_11_t_weight_0_fmt_scale_const_f32, conv2d_11_t_out_0_ptr_s8, conv2d_11_t_out_0_shape_w_const_u16, conv2d_11_t_out_0_shape_h_const_u16, 0, 4737, conv2d_11_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(11, 1, {(stai_ptr) conv2d_11_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_11 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_12 */
  {
      const ai_i8* conv2d_12_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 18176);
    const ai_i8* conv2d_12_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[24] + 0);
    const ai_i32* conv2d_12_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[25] + 0);
    ai_i8* conv2d_12_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1792);
    ai_i16* conv2d_12_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(12, 1, {(stai_ptr) conv2d_12_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_12_t_in_0_ptr_const_s8, conv2d_12_t_in_0_shape_w_const_u16, conv2d_12_t_in_0_shape_h_const_u16, conv2d_12_l_stride_1_const_u16, conv2d_12_l_stride_0_const_u16, conv2d_12_t_in_0_shape_ch_const_u16, conv2d_12_t_weight_0_ptr_const_s8, conv2d_12_t_out_0_shape_ch_const_u16, conv2d_12_t_weight_1_ptr_const_s32, conv2d_12_t_in_0_fmt_zero_const_s8, conv2d_12_t_out_0_fmt_zero_const_s8, conv2d_12_t_in_0_fmt_scale_const_f32, conv2d_12_t_out_0_fmt_scale_const_f32, conv2d_12_t_weight_0_fmt_scale_const_f32, conv2d_12_l_out_ch_format_const_layer_format_type, conv2d_12_t_out_0_ptr_s8, 1, 1792, conv2d_12_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(12, 1, {(stai_ptr) conv2d_12_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_12 */
  /* LITE_KERNEL_SECTION BEGIN pool_13 */
  {
    
  forward_lite_pool_13(net_ctx);
  }
  /* LITE_KERNEL_SECTION END pool_13 */
  /* LITE_KERNEL_SECTION BEGIN gemm_14 */
  {
    
  forward_lite_gemm_14(net_ctx);
  }
  /* LITE_KERNEL_SECTION END gemm_14 */
  /* LITE_KERNEL_SECTION BEGIN nl_15 */
  {
      ai_i8* nl_15_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_outputs[0] + 0);
    const ai_i8* nl_15_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 484);
    ai_i32* nl_15_t_scratch_0_ptr_s32 = (ai_i32*)(net_ctx->_activations[0] + 496);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(15, 1, {(stai_ptr) nl_15_t_in_0_ptr_const_s8});
    
  forward_lite_nl_softmax_is8os8(nl_15_t_out_0_ptr_s8, nl_15_t_in_0_ptr_const_s8, nl_15_t_in_0_shape_ch_prod_const_u32, 1, 10, 1176927104, 24, -124, nl_15_t_scratch_0_ptr_s32);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(15, 1, {(stai_ptr) nl_15_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END nl_15 */
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
  net_ctx->_inputs[0] = activations[0] + 16268;

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

