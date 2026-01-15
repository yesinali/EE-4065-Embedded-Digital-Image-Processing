/**
  ******************************************************************************
  * @file    network.c
  * @author  AST Embedded Analytics Research Platform
  * @date    2026-01-15T18:04:57+0000
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
#define _STAI_NETWORK_MODEL_SIGNATURE     "0x31e752ba188bd5e7694630c41ef81bfc"
#define _STAI_NETWORK_DATETIME            "2026-01-15T18:04:57+0000"
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
      STAI_DECLARE_ARRAY(int32_t, 1, 12544),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },
  .weights = (stai_tensor[STAI_NETWORK_WEIGHTS_NUM]) {
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_1_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 288),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_2_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_2_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 128),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_3_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_3_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 256),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_4_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_4_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 32),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_5_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_5_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1152),
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
      STAI_DECLARE_ARRAY(int32_t, 1, 128),
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
      STAI_DECLARE_ARRAY(int32_t, 1, 32),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_11_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_11_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1152),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_12_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_12_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 64),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_13_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_13_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 128),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_14_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_14_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 64),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_15_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_15_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_16_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_16_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 64),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_17_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_17_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 4608),
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
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
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
      STAI_DECLARE_ARRAY(int32_t, 1, 1024),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_22_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_22_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 64),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_23_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_23_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 4608),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_24_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_24_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 128),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_25_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_25_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_26_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_26_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 128),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_27_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_27_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 640),
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
      (stai_ptr)g_network_conv2d_0_weights_array,(stai_ptr)g_network_conv2d_0_bias_array,(stai_ptr)g_network_conv2d_2_weights_array,(stai_ptr)g_network_conv2d_2_bias_array,(stai_ptr)g_network_conv2d_4_weights_array,(stai_ptr)g_network_conv2d_4_bias_array,(stai_ptr)g_network_conv2d_3_weights_array,(stai_ptr)g_network_conv2d_3_bias_array,(stai_ptr)g_network_conv2d_6_weights_array,(stai_ptr)g_network_conv2d_6_bias_array,(stai_ptr)g_network_conv2d_8_weights_array,(stai_ptr)g_network_conv2d_8_bias_array,(stai_ptr)g_network_conv2d_7_weights_array,(stai_ptr)g_network_conv2d_7_bias_array,(stai_ptr)g_network_conv2d_11_weights_array,(stai_ptr)g_network_conv2d_11_bias_array,(stai_ptr)g_network_conv2d_13_weights_array,(stai_ptr)g_network_conv2d_13_bias_array,(stai_ptr)g_network_conv2d_12_weights_array,(stai_ptr)g_network_conv2d_12_bias_array,(stai_ptr)g_network_conv2d_15_weights_array,(stai_ptr)g_network_conv2d_15_bias_array,(stai_ptr)g_network_conv2d_17_weights_array,(stai_ptr)g_network_conv2d_17_bias_array,(stai_ptr)g_network_conv2d_16_weights_array,(stai_ptr)g_network_conv2d_16_bias_array,(stai_ptr)g_network_conv2d_19_weights_array,(stai_ptr)g_network_conv2d_19_bias_array
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
AI_INTQ_INFO_LIST_OBJ_DECLARE(serving_default_input0_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.003921568859368563f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #1 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_0_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.006514836568385363f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #2 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_0_weights_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 32,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0016388361109420657f, 0.004979453049600124f, 0.0025967846158891916f, 0.0025812294334173203f, 0.002803719835355878f, 0.0057953037321567535f, 0.0036449325270950794f, 0.001941205933690071f, 0.00250781187787652f, 0.0015595603035762906f, 0.004197416361421347f, 0.0028307035099714994f, 0.004728712607175112f, 0.0019270800985395908f, 0.002919477643445134f, 0.003751588985323906f, 0.001871448359452188f, 0.004265526309609413f, 0.0034240747336298227f, 0.003932925872504711f, 0.0028952083084732294f, 0.0024170679971575737f, 0.002995193237438798f, 0.0026956289075315f, 0.0026441437657922506f, 0.00321062165312469f, 0.0021831633057445288f, 0.002831352176144719f, 0.002666066400706768f, 0.0035520580131560564f, 0.0029196911491453648f, 0.003044595243409276f),
    AI_PACK_INTQ_ZP(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)))

/* Int quant #3 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_0_scratch1_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.006514836568385363f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #4 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_4_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.05573930963873863f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #5 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_3_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.05573930963873863f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #6 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(concat_5_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.05573930963873863f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #7 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_8_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.11481409519910812f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #8 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_7_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.11481409519910812f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #9 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(concat_9_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.11481409519910812f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #10 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_13_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.3459133505821228f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #11 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_12_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.3459133505821228f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #12 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(concat_14_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.3459133505821228f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #13 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_17_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.5669575333595276f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #14 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_16_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.5669575333595276f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #15 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(concat_18_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.5669575333595276f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #16 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_19_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.6213035583496094f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #17 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(pool_20_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.17450441420078278f),
    AI_PACK_INTQ_ZP(-128)))



/* Array#0 */
AI_ARRAY_OBJ_DECLARE(
  serving_default_input0_output_array, AI_ARRAY_FORMAT_S8|AI_FMT_FLAG_IS_IO,
  NULL, NULL, 784, AI_STATIC)

/* Array#1 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_0_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 6272, AI_STATIC)

/* Array#2 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_0_weights_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 288, AI_STATIC)

/* Array#3 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_0_bias_array, AI_ARRAY_FORMAT_S32,
  NULL, NULL, 32, AI_STATIC)

/* Array#4 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_0_scratch0_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 1060, AI_STATIC)

/* Array#5 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_0_scratch1_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 1792, AI_STATIC)

/* Array#6 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_4_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 3136, AI_STATIC)

/* Array#7 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_3_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 3136, AI_STATIC)

/* Array#8 */
AI_ARRAY_OBJ_DECLARE(
  concat_5_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 6272, AI_STATIC)

/* Array#9 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_8_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 3136, AI_STATIC)

/* Array#10 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_7_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 3136, AI_STATIC)

/* Array#11 */
AI_ARRAY_OBJ_DECLARE(
  concat_9_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 6272, AI_STATIC)

/* Array#12 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_13_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 1568, AI_STATIC)

/* Array#13 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_12_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 1568, AI_STATIC)

/* Array#14 */
AI_ARRAY_OBJ_DECLARE(
  concat_14_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 3136, AI_STATIC)

/* Array#15 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_17_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 1568, AI_STATIC)

/* Array#16 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_16_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 1568, AI_STATIC)

/* Array#17 */
AI_ARRAY_OBJ_DECLARE(
  concat_18_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 3136, AI_STATIC)

/* Array#18 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_19_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 490, AI_STATIC)

/* Array#19 */
AI_ARRAY_OBJ_DECLARE(
  pool_20_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 10, AI_STATIC)



/* Tensor #0 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_0_bias, AI_STATIC,
  4, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 1), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &conv2d_0_bias_array, NULL)

/* Tensor #1 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_0_output, AI_STATIC,
  5, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 1, 1, 32, 448),
  1, &conv2d_0_output_array, &conv2d_0_output_array_intq)

/* Tensor #2 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_0_scratch0, AI_STATIC,
  6, 0x0,
  AI_SHAPE_INIT(4, 1, 1060, 1, 1), AI_STRIDE_INIT(4, 1, 1, 1060, 1060),
  1, &conv2d_0_scratch0_array, NULL)

/* Tensor #3 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_0_scratch1, AI_STATIC,
  7, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 28, 2), AI_STRIDE_INIT(4, 1, 1, 32, 896),
  1, &conv2d_0_scratch1_array, &conv2d_0_scratch1_array_intq)

/* Tensor #4 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_0_weights, AI_STATIC,
  8, 0x1,
  AI_SHAPE_INIT(4, 1, 3, 3, 32), AI_STRIDE_INIT(4, 1, 1, 32, 96),
  1, &conv2d_0_weights_array, &conv2d_0_weights_array_intq)

/* Tensor #5 */
AI_TENSOR_OBJ_DECLARE(
  serving_default_input0_output, AI_STATIC,
  67, 0x1,
  AI_SHAPE_INIT(4, 1, 1, 28, 28), AI_STRIDE_INIT(4, 1, 1, 1, 28),
  1, &serving_default_input0_output_array, &serving_default_input0_output_array_intq)

/* Tensor #6 */
AI_TENSOR_OBJ_DECLARE(
  concat_5_output, AI_STATIC,
  2, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 1, 1, 32, 448),
  1, &concat_5_output_array, &concat_5_output_array_intq)

/* Tensor #7 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_3_output, AI_STATIC,
  44, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 14, 14), AI_STRIDE_INIT(4, 1, 1, 16, 224),
  1, &conv2d_3_output_array, &conv2d_3_output_array_intq)

/* Tensor #8 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_4_output, AI_STATIC,
  48, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 14, 14), AI_STRIDE_INIT(4, 1, 1, 16, 224),
  1, &conv2d_4_output_array, &conv2d_4_output_array_intq)

/* Tensor #9 */
AI_TENSOR_OBJ_DECLARE(
  concat_9_output, AI_STATIC,
  3, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 1, 1, 32, 448),
  1, &concat_9_output_array, &concat_9_output_array_intq)

/* Tensor #10 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_7_output, AI_STATIC,
  56, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 14, 14), AI_STRIDE_INIT(4, 1, 1, 16, 224),
  1, &conv2d_7_output_array, &conv2d_7_output_array_intq)

/* Tensor #11 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_8_output, AI_STATIC,
  60, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 14, 14), AI_STRIDE_INIT(4, 1, 1, 16, 224),
  1, &conv2d_8_output_array, &conv2d_8_output_array_intq)

/* Tensor #12 */
AI_TENSOR_OBJ_DECLARE(
  concat_14_output, AI_STATIC,
  0, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 1, 1, 64, 448),
  1, &concat_14_output_array, &concat_14_output_array_intq)

/* Tensor #13 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_12_output, AI_STATIC,
  14, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 7, 7), AI_STRIDE_INIT(4, 1, 1, 32, 224),
  1, &conv2d_12_output_array, &conv2d_12_output_array_intq)

/* Tensor #14 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_13_output, AI_STATIC,
  18, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 7, 7), AI_STRIDE_INIT(4, 1, 1, 32, 224),
  1, &conv2d_13_output_array, &conv2d_13_output_array_intq)

/* Tensor #15 */
AI_TENSOR_OBJ_DECLARE(
  concat_18_output, AI_STATIC,
  1, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 1, 1, 64, 448),
  1, &concat_18_output_array, &concat_18_output_array_intq)

/* Tensor #16 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_16_output, AI_STATIC,
  27, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 7, 7), AI_STRIDE_INIT(4, 1, 1, 32, 224),
  1, &conv2d_16_output_array, &conv2d_16_output_array_intq)

/* Tensor #17 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_17_output, AI_STATIC,
  31, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 7, 7), AI_STRIDE_INIT(4, 1, 1, 32, 224),
  1, &conv2d_17_output_array, &conv2d_17_output_array_intq)

/* Tensor #18 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_19_output, AI_STATIC,
  36, 0x1,
  AI_SHAPE_INIT(4, 1, 10, 7, 7), AI_STRIDE_INIT(4, 1, 1, 10, 70),
  1, &conv2d_19_output_array, &conv2d_19_output_array_intq)

/* Tensor #19 */
AI_TENSOR_OBJ_DECLARE(
  pool_20_output, AI_STATIC,
  66, 0x1,
  AI_SHAPE_INIT(4, 1, 10, 1, 1), AI_STRIDE_INIT(4, 1, 1, 10, 10),
  1, &pool_20_output_array, &pool_20_output_array_intq)


AI_TENSOR_CHAIN_OBJ_DECLARE(
  conv2d_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &serving_default_input0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &conv2d_0_weights, &conv2d_0_bias, NULL),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_0_scratch0, &conv2d_0_scratch1)
)

AI_LAYER_OBJ_DECLARE(
  conv2d_0_layer, 1,
  OPTIMIZED_CONV2D_TYPE, 0x0, NULL,
  conv2d_nl_pool, forward_conv2d_sssa8_ch_nl_pool,
  &conv2d_0_chain,
  NULL, &conv2d_0_layer, AI_STATIC, 
  .groups = 1, 
  .filter_stride = AI_SHAPE_2D_INIT(1, 1), 
  .dilation = AI_SHAPE_2D_INIT(1, 1), 
  .filter_pad = AI_SHAPE_INIT(4, 1, 1, 1, 1), 
  .pool_size = AI_SHAPE_2D_INIT(2, 2), 
  .pool_stride = AI_SHAPE_2D_INIT(2, 2), 
  .pool_pad = AI_SHAPE_INIT(4, 0, 0, 0, 0), 
  .pool_func = AI_HANDLE_PTR(pool_func_mp_array_integer_INT8), 
  .in_ch_format = AI_LAYER_FORMAT_CHANNEL_LAST_SAME, 
  .out_ch_format = AI_LAYER_FORMAT_CHANNEL_LAST_VALID, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  concat_5_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_3_output, &conv2d_4_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &concat_5_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  concat_5_layer, 5,
  CONCAT_TYPE, 0x0, NULL,
  concat, forward_concat,
  &concat_5_chain,
  NULL, &concat_5_layer, AI_STATIC, 
  .axis = AI_SHAPE_CHANNEL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  concat_9_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_7_output, &conv2d_8_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &concat_9_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  concat_9_layer, 9,
  CONCAT_TYPE, 0x0, NULL,
  concat, forward_concat,
  &concat_9_chain,
  NULL, &concat_9_layer, AI_STATIC, 
  .axis = AI_SHAPE_CHANNEL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  concat_14_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_12_output, &conv2d_13_output),
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

AI_TENSOR_CHAIN_OBJ_DECLARE(
  concat_18_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_16_output, &conv2d_17_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &concat_18_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  concat_18_layer, 18,
  CONCAT_TYPE, 0x0, NULL,
  concat, forward_concat,
  &concat_18_chain,
  NULL, &concat_18_layer, AI_STATIC, 
  .axis = AI_SHAPE_CHANNEL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  pool_20_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_19_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &pool_20_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  pool_20_layer, 20,
  POOL_TYPE, 0x0, NULL,
  pool, forward_ap_integer_INT8,
  &pool_20_chain,
  NULL, &pool_20_layer, AI_STATIC, 
  .pool_size = AI_SHAPE_2D_INIT(7, 7), 
  .pool_stride = AI_SHAPE_2D_INIT(7, 7), 
  .pool_pad = AI_SHAPE_INIT(4, 0, 0, 0, 0), 
)
/**  Hybrid layers declarations section  *************************************/
void forward_lite_conv2d_0(_stai_network_context* net_ctx)
{
  serving_default_input0_output_array.data = AI_PTR(net_ctx->_inputs[0] + 0);
  serving_default_input0_output_array.data_start = AI_PTR(net_ctx->_inputs[0] + 0);
  conv2d_0_weights_array.data = AI_PTR(net_ctx->_weights[0] + 0);
  conv2d_0_weights_array.data_start = AI_PTR(net_ctx->_weights[0] + 0);
  conv2d_0_bias_array.data = AI_PTR(net_ctx->_weights[1] + 0);
  conv2d_0_bias_array.data_start = AI_PTR(net_ctx->_weights[1] + 0);
  conv2d_0_scratch0_array.data = AI_PTR(net_ctx->_activations[0] + 10848);
  conv2d_0_scratch0_array.data_start = AI_PTR(net_ctx->_activations[0] + 10848);
  conv2d_0_scratch1_array.data = AI_PTR(net_ctx->_activations[0] + 8272);
  conv2d_0_scratch1_array.data_start = AI_PTR(net_ctx->_activations[0] + 8272);
  conv2d_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 2000);
  conv2d_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 2000);
  _STAI_NETWORK_EVENT_NODE_START_CB(1, 1, { serving_default_input0_output.data->data});
  forward_conv2d_sssa8_ch_nl_pool(&conv2d_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(1, 1, { conv2d_0_output.data->data});
}
void forward_lite_concat_5(_stai_network_context* net_ctx)
{
  conv2d_3_output_array.data = AI_PTR(net_ctx->_activations[0] + 9408);
  conv2d_3_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 9408);
  conv2d_4_output_array.data = AI_PTR(net_ctx->_activations[0] + 6272);
  conv2d_4_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 6272);
  concat_5_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  concat_5_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(5, 2, { conv2d_3_output.data->data,conv2d_4_output.data->data});
  forward_concat(&concat_5_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(5, 1, { concat_5_output.data->data});
}
void forward_lite_concat_9(_stai_network_context* net_ctx)
{
  conv2d_7_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  conv2d_7_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  conv2d_8_output_array.data = AI_PTR(net_ctx->_activations[0] + 9408);
  conv2d_8_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 9408);
  concat_9_output_array.data = AI_PTR(net_ctx->_activations[0] + 3136);
  concat_9_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 3136);
  _STAI_NETWORK_EVENT_NODE_START_CB(9, 2, { conv2d_7_output.data->data,conv2d_8_output.data->data});
  forward_concat(&concat_9_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(9, 1, { concat_9_output.data->data});
}
void forward_lite_concat_14(_stai_network_context* net_ctx)
{
  conv2d_12_output_array.data = AI_PTR(net_ctx->_activations[0] + 2640);
  conv2d_12_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 2640);
  conv2d_13_output_array.data = AI_PTR(net_ctx->_activations[0] + 8784);
  conv2d_13_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 8784);
  concat_14_output_array.data = AI_PTR(net_ctx->_activations[0] + 4208);
  concat_14_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 4208);
  _STAI_NETWORK_EVENT_NODE_START_CB(14, 2, { conv2d_12_output.data->data,conv2d_13_output.data->data});
  forward_concat(&concat_14_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(14, 1, { concat_14_output.data->data});
}
void forward_lite_concat_18(_stai_network_context* net_ctx)
{
  conv2d_16_output_array.data = AI_PTR(net_ctx->_activations[0] + 1200);
  conv2d_16_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1200);
  conv2d_17_output_array.data = AI_PTR(net_ctx->_activations[0] + 8640);
  conv2d_17_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 8640);
  concat_18_output_array.data = AI_PTR(net_ctx->_activations[0] + 2768);
  concat_18_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 2768);
  _STAI_NETWORK_EVENT_NODE_START_CB(18, 2, { conv2d_16_output.data->data,conv2d_17_output.data->data});
  forward_concat(&concat_18_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(18, 1, { concat_18_output.data->data});
}
void forward_lite_pool_20(_stai_network_context* net_ctx)
{
  conv2d_19_output_array.data = AI_PTR(net_ctx->_activations[0] + 356);
  conv2d_19_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 356);
  pool_20_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  pool_20_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(20, 1, { conv2d_19_output.data->data});
  forward_ap_integer_INT8(&pool_20_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(20, 1, { pool_20_output.data->data});
}

/*****************************************************************************/



static const ai_u16 conv2d_2_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_2_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_2_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_2_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_2_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_2_t_out_0_shape_ch_const_u16 = 8;
static const ai_i8 conv2d_2_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_2_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_2_t_in_0_fmt_scale_const_f32 = 0.006514836568385363f;
static const ai_float conv2d_2_t_out_0_fmt_scale_const_f32 = 0.030764872208237648f;
static const ai_float conv2d_2_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.006009957287460566f, 0.0048488653264939785f, 0.0064236377365887165f, 0.004981383681297302f, 0.007117386441677809f, 0.004087396431714296f, 0.005866778548806906f, 0.0029909948352724314f);
static const ai_layer_format_type conv2d_2_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_4_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_4_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_4_t_in_0_shape_ch_const_u16 = 8;
static const ai_u16 conv2d_4_t_out_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_4_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_4_t_weight_0_shape_h_const_u16 = 3;
static const ai_u16 conv2d_4_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_4_l_stride_0_const_u16 = 1;
static const ai_i32 conv2d_4_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_4_l_pad_H_0_const_s32 = 1;
static const ai_i8 conv2d_4_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_4_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_4_t_in_0_fmt_scale_const_f32 = 0.030764872208237648f;
static const ai_float conv2d_4_t_out_0_fmt_scale_const_f32 = 0.05573930963873863f;
static const ai_float conv2d_4_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0017517533851787448f, 0.006165361497551203f, 0.006830205209553242f, 0.004253053106367588f, 0.0042145405896008015f, 0.0029222150333225727f, 0.005358804948627949f, 0.006311207544058561f, 0.003092315746471286f, 0.005586893297731876f, 0.003643403295427561f, 0.0035902983509004116f, 0.003394323866814375f, 0.0043577528558671474f, 0.0033050330821424723f, 0.003646374912932515f);
static const ai_layer_format_type conv2d_4_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_4_t_out_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_4_t_out_0_shape_h_const_u16 = 14;

static const ai_u16 conv2d_3_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_3_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_3_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_3_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_3_t_in_0_shape_ch_const_u16 = 8;
static const ai_u16 conv2d_3_t_out_0_shape_ch_const_u16 = 16;
static const ai_i8 conv2d_3_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_3_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_3_t_in_0_fmt_scale_const_f32 = 0.030764872208237648f;
static const ai_float conv2d_3_t_out_0_fmt_scale_const_f32 = 0.05573930963873863f;
static const ai_float conv2d_3_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0037112445570528507f, 0.004576786886900663f, 0.003201864194124937f, 0.0040643829852342606f, 0.0038583893328905106f, 0.004552155267447233f, 0.0032182508148252964f, 0.0035193918738514185f, 0.003974678460508585f, 0.005177670624107122f, 0.004611937794834375f, 0.0038198649417608976f, 0.003978102933615446f, 0.004450968001037836f, 0.007014195900410414f, 0.004661254119127989f);
static const ai_layer_format_type conv2d_3_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;


static const ai_u16 conv2d_6_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_6_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_6_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_6_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_6_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_6_t_out_0_shape_ch_const_u16 = 8;
static const ai_i8 conv2d_6_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_6_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_6_t_in_0_fmt_scale_const_f32 = 0.05573930963873863f;
static const ai_float conv2d_6_t_out_0_fmt_scale_const_f32 = 0.04966450110077858f;
static const ai_float conv2d_6_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.004857872612774372f, 0.005762494169175625f, 0.005888147745281458f, 0.005649925675243139f, 0.006816063541918993f, 0.0067381467670202255f, 0.005930265877395868f, 0.00580610241740942f);
static const ai_layer_format_type conv2d_6_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_8_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_8_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_8_t_in_0_shape_ch_const_u16 = 8;
static const ai_u16 conv2d_8_t_out_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_8_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_8_t_weight_0_shape_h_const_u16 = 3;
static const ai_u16 conv2d_8_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_8_l_stride_0_const_u16 = 1;
static const ai_i32 conv2d_8_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_8_l_pad_H_0_const_s32 = 1;
static const ai_i8 conv2d_8_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_8_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_8_t_in_0_fmt_scale_const_f32 = 0.04966450110077858f;
static const ai_float conv2d_8_t_out_0_fmt_scale_const_f32 = 0.11481409519910812f;
static const ai_float conv2d_8_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.004184543155133724f, 0.007346422411501408f, 0.005119443871080875f, 0.004299368243664503f, 0.0036596686113625765f, 0.0026418257039040327f, 0.011030502617359161f, 0.004209436941891909f, 0.0024022171273827553f, 0.001960519002750516f, 0.003339831717312336f, 0.0031485510990023613f, 0.00343298795633018f, 0.0021590653341263533f, 0.003662077011540532f, 0.003306340891867876f);
static const ai_layer_format_type conv2d_8_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_8_t_out_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_8_t_out_0_shape_h_const_u16 = 14;

static const ai_u16 conv2d_7_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_7_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_7_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_7_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_7_t_in_0_shape_ch_const_u16 = 8;
static const ai_u16 conv2d_7_t_out_0_shape_ch_const_u16 = 16;
static const ai_i8 conv2d_7_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_7_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_7_t_in_0_fmt_scale_const_f32 = 0.04966450110077858f;
static const ai_float conv2d_7_t_out_0_fmt_scale_const_f32 = 0.11481409519910812f;
static const ai_float conv2d_7_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.004380494821816683f, 0.004208667203783989f, 0.003941367380321026f, 0.0035446370020508766f, 0.003294139401987195f, 0.006085270084440708f, 0.0037485051434487104f, 0.003571580396965146f, 0.0059180911630392075f, 0.005817997734993696f, 0.004057410638779402f, 0.003801557468250394f, 0.004762883763760328f, 0.0051994528621435165f, 0.006787337828427553f, 0.003432396799325943f);
static const ai_layer_format_type conv2d_7_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;


static const ai_u16 pool_10_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 pool_10_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 pool_10_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 pool_10_l_pool_size_1_const_u16 = 2;
static const ai_u16 pool_10_l_pool_size_0_const_u16 = 2;
static const ai_u16 pool_10_l_legacy_pool_pad_1_const_u16 = 0;
static const ai_u16 pool_10_l_legacy_pool_pad_0_const_u16 = 0;
static const ai_u16 pool_10_l_pool_stride_1_const_u16 = 2;
static const ai_u16 pool_10_l_pool_stride_0_const_u16 = 2;
static const ai_u16 pool_10_t_out_0_shape_w_const_u16 = 7;
static const ai_u16 pool_10_t_out_0_shape_h_const_u16 = 7;
static const ai_i8 pool_10_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 pool_10_t_out_0_fmt_zero_const_s8 = -128;

static const ai_u16 conv2d_11_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_11_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_11_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_11_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_11_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_11_t_out_0_shape_ch_const_u16 = 16;
static const ai_i8 conv2d_11_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_11_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_11_t_in_0_fmt_scale_const_f32 = 0.11481409519910812f;
static const ai_float conv2d_11_t_out_0_fmt_scale_const_f32 = 0.14174887537956238f;
static const ai_float conv2d_11_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.005290650762617588f, 0.005980667658150196f, 0.007174445316195488f, 0.004254277795553207f, 0.0049180383794009686f, 0.003450903808698058f, 0.007562402170151472f, 0.00329749658703804f, 0.005025525577366352f, 0.007900217548012733f, 0.004691625013947487f, 0.0047651235945522785f, 0.005856940522789955f, 0.004471592139452696f, 0.002632827265188098f, 0.0054289717227220535f);
static const ai_layer_format_type conv2d_11_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_13_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_13_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_13_pad_before_t_in_0_shape_h_const_u32 = 7;

static const ai_u16 conv2d_13_t_in_0_shape_w_const_u16 = 9;
static const ai_u16 conv2d_13_t_in_0_shape_h_const_u16 = 9;
static const ai_u16 conv2d_13_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_13_t_out_0_shape_ch_const_u16 = 32;
static const ai_i8 conv2d_13_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_13_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_13_t_in_0_fmt_scale_const_f32 = 0.14174887537956238f;
static const ai_float conv2d_13_t_out_0_fmt_scale_const_f32 = 0.3459133505821228f;
static const ai_float conv2d_13_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0060373349115252495f, 0.006977534387260675f, 0.004751121159642935f, 0.0031681007239967585f, 0.0036105632316321135f, 0.007118845824152231f, 0.001219273079186678f, 0.005022076424211264f, 0.0049072676338255405f, 0.0038110974710434675f, 0.004151586443185806f, 0.007081456482410431f, 0.0038874575402587652f, 0.005179067142307758f, 0.005759841296821833f, 0.0031119375489652157f, 0.003931874874979258f, 0.003357051406055689f, 0.002370776841416955f, 0.004926181398332119f, 0.004296136554330587f, 0.00432651536539197f, 0.004002169240266085f, 0.0041931988671422005f, 0.0038890980649739504f, 0.0031060201581567526f, 0.005441906861960888f, 0.0054847425781190395f, 0.0013436764711514115f, 0.004781966097652912f, 0.003629811340942979f, 0.004542325157672167f);
static const ai_layer_format_type conv2d_13_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_13_t_out_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_13_t_out_0_shape_h_const_u16 = 7;

static const ai_u16 conv2d_12_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_12_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_12_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_12_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_12_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_12_t_out_0_shape_ch_const_u16 = 32;
static const ai_i8 conv2d_12_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_12_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_12_t_in_0_fmt_scale_const_f32 = 0.14174887537956238f;
static const ai_float conv2d_12_t_out_0_fmt_scale_const_f32 = 0.3459133505821228f;
static const ai_float conv2d_12_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.003994259983301163f, 0.003453149925917387f, 0.007384726777672768f, 0.003021371318027377f, 0.004953670781105757f, 0.0031887295190244913f, 0.004911625292152166f, 0.0027095808181911707f, 0.0037062838673591614f, 0.0025841672904789448f, 0.0026144676376134157f, 0.0025062989443540573f, 0.0042260452173650265f, 0.0027914405800402164f, 0.003017246723175049f, 0.0034949774853885174f, 0.0038064445834606886f, 0.0030064573511481285f, 0.005707141011953354f, 0.0029729928355664015f, 0.0030159088782966137f, 0.00427228631451726f, 0.0026529186870902777f, 0.004142744466662407f, 0.002723642159253359f, 0.004247912205755711f, 0.004385484382510185f, 0.0030915190000087023f, 0.003244571853429079f, 0.0026350219268351793f, 0.003195839235559106f, 0.004630392417311668f);
static const ai_layer_format_type conv2d_12_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;


static const ai_u16 conv2d_15_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_15_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_15_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_15_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_15_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_15_t_out_0_shape_ch_const_u16 = 16;
static const ai_i8 conv2d_15_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_15_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_15_t_in_0_fmt_scale_const_f32 = 0.3459133505821228f;
static const ai_float conv2d_15_t_out_0_fmt_scale_const_f32 = 0.41464847326278687f;
static const ai_float conv2d_15_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.004626646637916565f, 0.006140351295471191f, 0.008861283771693707f, 0.006435605697333813f, 0.0038479245267808437f, 0.003000241704285145f, 0.0051542711444199085f, 0.00473244022578001f, 0.004313579294830561f, 0.0043044667690992355f, 0.006478800904005766f, 0.006419289857149124f, 0.003951000981032848f, 0.0040946961380541325f, 0.007165161892771721f, 0.004002119414508343f);
static const ai_layer_format_type conv2d_15_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_17_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_17_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_17_pad_before_t_in_0_shape_h_const_u32 = 7;

static const ai_u16 conv2d_17_t_in_0_shape_w_const_u16 = 9;
static const ai_u16 conv2d_17_t_in_0_shape_h_const_u16 = 9;
static const ai_u16 conv2d_17_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_17_t_out_0_shape_ch_const_u16 = 32;
static const ai_i8 conv2d_17_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_17_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_17_t_in_0_fmt_scale_const_f32 = 0.41464847326278687f;
static const ai_float conv2d_17_t_out_0_fmt_scale_const_f32 = 0.5669575333595276f;
static const ai_float conv2d_17_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.003735768608748913f, 0.0036408875603228807f, 0.004931484814733267f, 0.010173824615776539f, 0.004046970512717962f, 0.004837776068598032f, 0.003489213762804866f, 0.004496278241276741f, 0.004477513954043388f, 0.004987092688679695f, 0.004928083159029484f, 0.004111138172447681f, 0.0032654774840921164f, 0.006378957070410252f, 0.004986762534826994f, 0.005079040303826332f, 0.0042442684061825275f, 0.0038919493090361357f, 0.007889613509178162f, 0.0038794383872300386f, 0.002159904222935438f, 0.0024209213443100452f, 0.0054964302107691765f, 0.006421132478863001f, 0.004921463783830404f, 0.006098717916756868f, 0.0027107722125947475f, 0.00786551833152771f, 0.006262474227696657f, 0.004655750468373299f, 0.005850601941347122f, 0.0039017368108034134f);
static const ai_layer_format_type conv2d_17_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_17_t_out_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_17_t_out_0_shape_h_const_u16 = 7;

static const ai_u16 conv2d_16_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_16_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_16_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_16_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_16_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_16_t_out_0_shape_ch_const_u16 = 32;
static const ai_i8 conv2d_16_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_16_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_16_t_in_0_fmt_scale_const_f32 = 0.41464847326278687f;
static const ai_float conv2d_16_t_out_0_fmt_scale_const_f32 = 0.5669575333595276f;
static const ai_float conv2d_16_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.005026788916438818f, 0.0029915955383330584f, 0.007042114157229662f, 0.004032675176858902f, 0.0034472865518182516f, 0.0064193508587777615f, 0.003250394482165575f, 0.0039189886301755905f, 0.003014928661286831f, 0.004951948300004005f, 0.0030881657730787992f, 0.0037963648792356253f, 0.005358605179935694f, 0.005837650969624519f, 0.004930904135107994f, 0.005311427637934685f, 0.0029384708032011986f, 0.003322441829368472f, 0.0027214146684855223f, 0.0028309521730989218f, 0.006182235199958086f, 0.007369711995124817f, 0.006019704043865204f, 0.003214749274775386f, 0.00312228687107563f, 0.0052063120529055595f, 0.00345275248400867f, 0.0046226163394749165f, 0.002783275442197919f, 0.005943987984210253f, 0.002771420404314995f, 0.005972647573798895f);
static const ai_layer_format_type conv2d_16_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;


static const ai_u16 conv2d_19_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_19_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_19_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_19_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_19_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_19_t_out_0_shape_ch_const_u16 = 10;
static const ai_i8 conv2d_19_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_19_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_19_t_in_0_fmt_scale_const_f32 = 0.5669575333595276f;
static const ai_float conv2d_19_t_out_0_fmt_scale_const_f32 = 0.6213035583496094f;
static const ai_float conv2d_19_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.006125673186033964f, 0.008505970239639282f, 0.004433146677911282f, 0.00599766056984663f, 0.004097284283488989f, 0.004786340519785881f, 0.005471273325383663f, 0.005804939195513725f, 0.005620005540549755f, 0.006128127686679363f);
static const ai_layer_format_type conv2d_19_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;


static const ai_u32 nl_21_t_in_0_shape_ch_prod_const_u32 = 10;
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
    
  forward_lite_conv2d_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END conv2d_0 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_2 */
  {
      const ai_i8* conv2d_2_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 2000);
    const ai_i8* conv2d_2_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[2] + 0);
    const ai_i32* conv2d_2_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[3] + 0);
    ai_i8* conv2d_2_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1888);
    ai_i16* conv2d_2_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 8272);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(2, 1, {(stai_ptr) conv2d_2_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_2_t_in_0_ptr_const_s8, conv2d_2_t_in_0_shape_w_const_u16, conv2d_2_t_in_0_shape_h_const_u16, conv2d_2_l_stride_1_const_u16, conv2d_2_l_stride_0_const_u16, conv2d_2_t_in_0_shape_ch_const_u16, conv2d_2_t_weight_0_ptr_const_s8, conv2d_2_t_out_0_shape_ch_const_u16, conv2d_2_t_weight_1_ptr_const_s32, conv2d_2_t_in_0_fmt_zero_const_s8, conv2d_2_t_out_0_fmt_zero_const_s8, conv2d_2_t_in_0_fmt_scale_const_f32, conv2d_2_t_out_0_fmt_scale_const_f32, conv2d_2_t_weight_0_fmt_scale_const_f32, conv2d_2_l_out_ch_format_const_layer_format_type, conv2d_2_t_out_0_ptr_s8, 1, 208, conv2d_2_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(2, 1, {(stai_ptr) conv2d_2_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_2 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_4 */
  {
      const ai_i8* conv2d_4_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1888);
    const ai_i8* conv2d_4_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[4] + 0);
    const ai_i32* conv2d_4_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[5] + 0);
    ai_i8* conv2d_4_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 6272);
    ai_i16* conv2d_4_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 3456);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(4, 1, {(stai_ptr) conv2d_4_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_sssa8_ch(conv2d_4_t_in_0_ptr_const_s8, conv2d_4_t_in_0_shape_w_const_u16, conv2d_4_t_in_0_shape_h_const_u16, conv2d_4_t_in_0_shape_ch_const_u16, conv2d_4_t_weight_0_ptr_const_s8, conv2d_4_t_out_0_shape_ch_const_u16, conv2d_4_t_weight_0_shape_w_const_u16, conv2d_4_t_weight_0_shape_h_const_u16, conv2d_4_l_stride_1_const_u16, conv2d_4_l_stride_0_const_u16, conv2d_4_l_pad_W_0_const_s32, conv2d_4_l_pad_H_0_const_s32, conv2d_4_t_weight_1_ptr_const_s32, conv2d_4_t_in_0_fmt_zero_const_s8, conv2d_4_t_out_0_fmt_zero_const_s8, conv2d_4_t_in_0_fmt_scale_const_f32, conv2d_4_t_out_0_fmt_scale_const_f32, conv2d_4_t_weight_0_fmt_scale_const_f32, conv2d_4_l_out_ch_format_const_layer_format_type, conv2d_4_t_out_0_ptr_s8, conv2d_4_t_out_0_shape_w_const_u16, conv2d_4_t_out_0_shape_h_const_u16, 1, 2816, conv2d_4_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(4, 1, {(stai_ptr) conv2d_4_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_4 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_3 */
  {
      const ai_i8* conv2d_3_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1888);
    const ai_i8* conv2d_3_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[6] + 0);
    const ai_i32* conv2d_3_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[7] + 0);
    ai_i8* conv2d_3_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 9408);
    ai_i16* conv2d_3_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 1696);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(3, 1, {(stai_ptr) conv2d_3_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_3_t_in_0_ptr_const_s8, conv2d_3_t_in_0_shape_w_const_u16, conv2d_3_t_in_0_shape_h_const_u16, conv2d_3_l_stride_1_const_u16, conv2d_3_l_stride_0_const_u16, conv2d_3_t_in_0_shape_ch_const_u16, conv2d_3_t_weight_0_ptr_const_s8, conv2d_3_t_out_0_shape_ch_const_u16, conv2d_3_t_weight_1_ptr_const_s32, conv2d_3_t_in_0_fmt_zero_const_s8, conv2d_3_t_out_0_fmt_zero_const_s8, conv2d_3_t_in_0_fmt_scale_const_f32, conv2d_3_t_out_0_fmt_scale_const_f32, conv2d_3_t_weight_0_fmt_scale_const_f32, conv2d_3_l_out_ch_format_const_layer_format_type, conv2d_3_t_out_0_ptr_s8, 1, 192, conv2d_3_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(3, 1, {(stai_ptr) conv2d_3_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_3 */
  /* LITE_KERNEL_SECTION BEGIN concat_5 */
  {
    
  forward_lite_concat_5(net_ctx);
  }
  /* LITE_KERNEL_SECTION END concat_5 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_6 */
  {
      const ai_i8* conv2d_6_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_6_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[8] + 0);
    const ai_i32* conv2d_6_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[9] + 0);
    ai_i8* conv2d_6_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 6480);
    ai_i16* conv2d_6_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 6272);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(6, 1, {(stai_ptr) conv2d_6_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_6_t_in_0_ptr_const_s8, conv2d_6_t_in_0_shape_w_const_u16, conv2d_6_t_in_0_shape_h_const_u16, conv2d_6_l_stride_1_const_u16, conv2d_6_l_stride_0_const_u16, conv2d_6_t_in_0_shape_ch_const_u16, conv2d_6_t_weight_0_ptr_const_s8, conv2d_6_t_out_0_shape_ch_const_u16, conv2d_6_t_weight_1_ptr_const_s32, conv2d_6_t_in_0_fmt_zero_const_s8, conv2d_6_t_out_0_fmt_zero_const_s8, conv2d_6_t_in_0_fmt_scale_const_f32, conv2d_6_t_out_0_fmt_scale_const_f32, conv2d_6_t_weight_0_fmt_scale_const_f32, conv2d_6_l_out_ch_format_const_layer_format_type, conv2d_6_t_out_0_ptr_s8, 1, 208, conv2d_6_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(6, 1, {(stai_ptr) conv2d_6_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_6 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_8 */
  {
      const ai_i8* conv2d_8_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 6480);
    const ai_i8* conv2d_8_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[10] + 0);
    const ai_i32* conv2d_8_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[11] + 0);
    ai_i8* conv2d_8_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 9408);
    ai_i16* conv2d_8_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(8, 1, {(stai_ptr) conv2d_8_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_sssa8_ch(conv2d_8_t_in_0_ptr_const_s8, conv2d_8_t_in_0_shape_w_const_u16, conv2d_8_t_in_0_shape_h_const_u16, conv2d_8_t_in_0_shape_ch_const_u16, conv2d_8_t_weight_0_ptr_const_s8, conv2d_8_t_out_0_shape_ch_const_u16, conv2d_8_t_weight_0_shape_w_const_u16, conv2d_8_t_weight_0_shape_h_const_u16, conv2d_8_l_stride_1_const_u16, conv2d_8_l_stride_0_const_u16, conv2d_8_l_pad_W_0_const_s32, conv2d_8_l_pad_H_0_const_s32, conv2d_8_t_weight_1_ptr_const_s32, conv2d_8_t_in_0_fmt_zero_const_s8, conv2d_8_t_out_0_fmt_zero_const_s8, conv2d_8_t_in_0_fmt_scale_const_f32, conv2d_8_t_out_0_fmt_scale_const_f32, conv2d_8_t_weight_0_fmt_scale_const_f32, conv2d_8_l_out_ch_format_const_layer_format_type, conv2d_8_t_out_0_ptr_s8, conv2d_8_t_out_0_shape_w_const_u16, conv2d_8_t_out_0_shape_h_const_u16, 1, 2816, conv2d_8_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(8, 1, {(stai_ptr) conv2d_8_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_8 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_7 */
  {
      const ai_i8* conv2d_7_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 6480);
    const ai_i8* conv2d_7_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[12] + 0);
    const ai_i32* conv2d_7_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[13] + 0);
    ai_i8* conv2d_7_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_7_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 6288);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(7, 1, {(stai_ptr) conv2d_7_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_7_t_in_0_ptr_const_s8, conv2d_7_t_in_0_shape_w_const_u16, conv2d_7_t_in_0_shape_h_const_u16, conv2d_7_l_stride_1_const_u16, conv2d_7_l_stride_0_const_u16, conv2d_7_t_in_0_shape_ch_const_u16, conv2d_7_t_weight_0_ptr_const_s8, conv2d_7_t_out_0_shape_ch_const_u16, conv2d_7_t_weight_1_ptr_const_s32, conv2d_7_t_in_0_fmt_zero_const_s8, conv2d_7_t_out_0_fmt_zero_const_s8, conv2d_7_t_in_0_fmt_scale_const_f32, conv2d_7_t_out_0_fmt_scale_const_f32, conv2d_7_t_weight_0_fmt_scale_const_f32, conv2d_7_l_out_ch_format_const_layer_format_type, conv2d_7_t_out_0_ptr_s8, 1, 192, conv2d_7_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(7, 1, {(stai_ptr) conv2d_7_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_7 */
  /* LITE_KERNEL_SECTION BEGIN concat_9 */
  {
    
  forward_lite_concat_9(net_ctx);
  }
  /* LITE_KERNEL_SECTION END concat_9 */
  /* LITE_KERNEL_SECTION BEGIN pool_10 */
  {
      const ai_i8* pool_10_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 3136);
    ai_i8* pool_10_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(10, 1, {(stai_ptr) pool_10_t_in_0_ptr_const_s8});
    
  forward_lite_maxpool_is8os8_scalepos(pool_10_t_in_0_ptr_const_s8, pool_10_t_out_0_ptr_s8, pool_10_t_in_0_shape_w_const_u16, pool_10_t_in_0_shape_h_const_u16, pool_10_t_in_0_shape_ch_const_u16, pool_10_l_pool_size_1_const_u16, pool_10_l_pool_size_0_const_u16, pool_10_l_legacy_pool_pad_1_const_u16, pool_10_l_legacy_pool_pad_0_const_u16, pool_10_l_pool_stride_1_const_u16, pool_10_l_pool_stride_0_const_u16, pool_10_t_out_0_shape_w_const_u16, pool_10_t_out_0_shape_h_const_u16, 1.0f, pool_10_t_in_0_fmt_zero_const_s8, pool_10_t_out_0_fmt_zero_const_s8);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(10, 1, {(stai_ptr) pool_10_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END pool_10 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_11 */
  {
      const ai_i8* conv2d_11_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_11_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[14] + 0);
    const ai_i32* conv2d_11_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[15] + 0);
    ai_i8* conv2d_11_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1856);
    ai_i16* conv2d_11_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 1568);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(11, 1, {(stai_ptr) conv2d_11_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_11_t_in_0_ptr_const_s8, conv2d_11_t_in_0_shape_w_const_u16, conv2d_11_t_in_0_shape_h_const_u16, conv2d_11_l_stride_1_const_u16, conv2d_11_l_stride_0_const_u16, conv2d_11_t_in_0_shape_ch_const_u16, conv2d_11_t_weight_0_ptr_const_s8, conv2d_11_t_out_0_shape_ch_const_u16, conv2d_11_t_weight_1_ptr_const_s32, conv2d_11_t_in_0_fmt_zero_const_s8, conv2d_11_t_out_0_fmt_zero_const_s8, conv2d_11_t_in_0_fmt_scale_const_f32, conv2d_11_t_out_0_fmt_scale_const_f32, conv2d_11_t_weight_0_fmt_scale_const_f32, conv2d_11_l_out_ch_format_const_layer_format_type, conv2d_11_t_out_0_ptr_s8, 1, 288, conv2d_11_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(11, 1, {(stai_ptr) conv2d_11_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_11 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_13_pad_before */
  {
      const ai_ptr conv2d_13_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 1856);
    ai_ptr conv2d_13_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(13, 1, {(stai_ptr) conv2d_13_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_13_pad_before_t_in_0_ptr_const_ptr, conv2d_13_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_13_pad_before_v_pad_constant_value_const_s8), conv2d_13_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_13_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(112), (ai_i32)(144), (ai_i32)(144), (ai_i32)(16), (ai_i32)(16));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(13, 1, {(stai_ptr) conv2d_13_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_13_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_13 */
  {
      const ai_i8* conv2d_13_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_13_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[16] + 0);
    const ai_i32* conv2d_13_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[17] + 0);
    ai_i8* conv2d_13_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 8784);
    ai_i16* conv2d_13_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 2640);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(13, 1, {(stai_ptr) conv2d_13_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_13_t_in_0_ptr_const_s8, conv2d_13_t_in_0_shape_w_const_u16, conv2d_13_t_in_0_shape_h_const_u16, conv2d_13_t_in_0_shape_ch_const_u16, conv2d_13_t_weight_0_ptr_const_s8, conv2d_13_t_out_0_shape_ch_const_u16, conv2d_13_t_weight_1_ptr_const_s32, conv2d_13_t_in_0_fmt_zero_const_s8, conv2d_13_t_out_0_fmt_zero_const_s8, conv2d_13_t_in_0_fmt_scale_const_f32, conv2d_13_t_out_0_fmt_scale_const_f32, conv2d_13_t_weight_0_fmt_scale_const_f32, conv2d_13_l_out_ch_format_const_layer_format_type, conv2d_13_t_out_0_ptr_s8, conv2d_13_t_out_0_shape_w_const_u16, conv2d_13_t_out_0_shape_h_const_u16, 1, 6144, conv2d_13_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(13, 1, {(stai_ptr) conv2d_13_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_13 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_12 */
  {
      const ai_i8* conv2d_12_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1856);
    const ai_i8* conv2d_12_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[18] + 0);
    const ai_i32* conv2d_12_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[19] + 0);
    ai_i8* conv2d_12_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 2640);
    ai_i16* conv2d_12_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(12, 1, {(stai_ptr) conv2d_12_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_12_t_in_0_ptr_const_s8, conv2d_12_t_in_0_shape_w_const_u16, conv2d_12_t_in_0_shape_h_const_u16, conv2d_12_l_stride_1_const_u16, conv2d_12_l_stride_0_const_u16, conv2d_12_t_in_0_shape_ch_const_u16, conv2d_12_t_weight_0_ptr_const_s8, conv2d_12_t_out_0_shape_ch_const_u16, conv2d_12_t_weight_1_ptr_const_s32, conv2d_12_t_in_0_fmt_zero_const_s8, conv2d_12_t_out_0_fmt_zero_const_s8, conv2d_12_t_in_0_fmt_scale_const_f32, conv2d_12_t_out_0_fmt_scale_const_f32, conv2d_12_t_weight_0_fmt_scale_const_f32, conv2d_12_l_out_ch_format_const_layer_format_type, conv2d_12_t_out_0_ptr_s8, 1, 384, conv2d_12_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(12, 1, {(stai_ptr) conv2d_12_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_12 */
  /* LITE_KERNEL_SECTION BEGIN concat_14 */
  {
    
  forward_lite_concat_14(net_ctx);
  }
  /* LITE_KERNEL_SECTION END concat_14 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_15 */
  {
      const ai_i8* conv2d_15_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 4208);
    const ai_i8* conv2d_15_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[20] + 0);
    const ai_i32* conv2d_15_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[21] + 0);
    ai_i8* conv2d_15_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 416);
    ai_i16* conv2d_15_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(15, 1, {(stai_ptr) conv2d_15_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_15_t_in_0_ptr_const_s8, conv2d_15_t_in_0_shape_w_const_u16, conv2d_15_t_in_0_shape_h_const_u16, conv2d_15_l_stride_1_const_u16, conv2d_15_l_stride_0_const_u16, conv2d_15_t_in_0_shape_ch_const_u16, conv2d_15_t_weight_0_ptr_const_s8, conv2d_15_t_out_0_shape_ch_const_u16, conv2d_15_t_weight_1_ptr_const_s32, conv2d_15_t_in_0_fmt_zero_const_s8, conv2d_15_t_out_0_fmt_zero_const_s8, conv2d_15_t_in_0_fmt_scale_const_f32, conv2d_15_t_out_0_fmt_scale_const_f32, conv2d_15_t_weight_0_fmt_scale_const_f32, conv2d_15_l_out_ch_format_const_layer_format_type, conv2d_15_t_out_0_ptr_s8, 1, 416, conv2d_15_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(15, 1, {(stai_ptr) conv2d_15_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_15 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_17_pad_before */
  {
      const ai_ptr conv2d_17_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 416);
    ai_ptr conv2d_17_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 1200);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(17, 1, {(stai_ptr) conv2d_17_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_17_pad_before_t_in_0_ptr_const_ptr, conv2d_17_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_17_pad_before_v_pad_constant_value_const_s8), conv2d_17_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_17_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(112), (ai_i32)(144), (ai_i32)(144), (ai_i32)(16), (ai_i32)(16));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(17, 1, {(stai_ptr) conv2d_17_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_17_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_17 */
  {
      const ai_i8* conv2d_17_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1200);
    const ai_i8* conv2d_17_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[22] + 0);
    const ai_i32* conv2d_17_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[23] + 0);
    ai_i8* conv2d_17_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 8640);
    ai_i16* conv2d_17_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 2496);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(17, 1, {(stai_ptr) conv2d_17_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_17_t_in_0_ptr_const_s8, conv2d_17_t_in_0_shape_w_const_u16, conv2d_17_t_in_0_shape_h_const_u16, conv2d_17_t_in_0_shape_ch_const_u16, conv2d_17_t_weight_0_ptr_const_s8, conv2d_17_t_out_0_shape_ch_const_u16, conv2d_17_t_weight_1_ptr_const_s32, conv2d_17_t_in_0_fmt_zero_const_s8, conv2d_17_t_out_0_fmt_zero_const_s8, conv2d_17_t_in_0_fmt_scale_const_f32, conv2d_17_t_out_0_fmt_scale_const_f32, conv2d_17_t_weight_0_fmt_scale_const_f32, conv2d_17_l_out_ch_format_const_layer_format_type, conv2d_17_t_out_0_ptr_s8, conv2d_17_t_out_0_shape_w_const_u16, conv2d_17_t_out_0_shape_h_const_u16, 1, 6144, conv2d_17_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(17, 1, {(stai_ptr) conv2d_17_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_17 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_16 */
  {
      const ai_i8* conv2d_16_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 416);
    const ai_i8* conv2d_16_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[24] + 0);
    const ai_i32* conv2d_16_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[25] + 0);
    ai_i8* conv2d_16_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1200);
    ai_i16* conv2d_16_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(16, 1, {(stai_ptr) conv2d_16_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_16_t_in_0_ptr_const_s8, conv2d_16_t_in_0_shape_w_const_u16, conv2d_16_t_in_0_shape_h_const_u16, conv2d_16_l_stride_1_const_u16, conv2d_16_l_stride_0_const_u16, conv2d_16_t_in_0_shape_ch_const_u16, conv2d_16_t_weight_0_ptr_const_s8, conv2d_16_t_out_0_shape_ch_const_u16, conv2d_16_t_weight_1_ptr_const_s32, conv2d_16_t_in_0_fmt_zero_const_s8, conv2d_16_t_out_0_fmt_zero_const_s8, conv2d_16_t_in_0_fmt_scale_const_f32, conv2d_16_t_out_0_fmt_scale_const_f32, conv2d_16_t_weight_0_fmt_scale_const_f32, conv2d_16_l_out_ch_format_const_layer_format_type, conv2d_16_t_out_0_ptr_s8, 1, 384, conv2d_16_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(16, 1, {(stai_ptr) conv2d_16_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_16 */
  /* LITE_KERNEL_SECTION BEGIN concat_18 */
  {
    
  forward_lite_concat_18(net_ctx);
  }
  /* LITE_KERNEL_SECTION END concat_18 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_19 */
  {
      const ai_i8* conv2d_19_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 2768);
    const ai_i8* conv2d_19_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[26] + 0);
    const ai_i32* conv2d_19_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[27] + 0);
    ai_i8* conv2d_19_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 356);
    ai_i16* conv2d_19_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(19, 1, {(stai_ptr) conv2d_19_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_19_t_in_0_ptr_const_s8, conv2d_19_t_in_0_shape_w_const_u16, conv2d_19_t_in_0_shape_h_const_u16, conv2d_19_l_stride_1_const_u16, conv2d_19_l_stride_0_const_u16, conv2d_19_t_in_0_shape_ch_const_u16, conv2d_19_t_weight_0_ptr_const_s8, conv2d_19_t_out_0_shape_ch_const_u16, conv2d_19_t_weight_1_ptr_const_s32, conv2d_19_t_in_0_fmt_zero_const_s8, conv2d_19_t_out_0_fmt_zero_const_s8, conv2d_19_t_in_0_fmt_scale_const_f32, conv2d_19_t_out_0_fmt_scale_const_f32, conv2d_19_t_weight_0_fmt_scale_const_f32, conv2d_19_l_out_ch_format_const_layer_format_type, conv2d_19_t_out_0_ptr_s8, 1, 356, conv2d_19_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(19, 1, {(stai_ptr) conv2d_19_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_19 */
  /* LITE_KERNEL_SECTION BEGIN pool_20 */
  {
    
  forward_lite_pool_20(net_ctx);
  }
  /* LITE_KERNEL_SECTION END pool_20 */
  /* LITE_KERNEL_SECTION BEGIN nl_21 */
  {
      ai_i8* nl_21_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_outputs[0] + 0);
    const ai_i8* nl_21_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i32* nl_21_t_scratch_0_ptr_s32 = (ai_i32*)(net_ctx->_activations[0] + 12);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(21, 1, {(stai_ptr) nl_21_t_in_0_ptr_const_s8});
    
  forward_lite_nl_softmax_is8os8(nl_21_t_out_0_ptr_s8, nl_21_t_in_0_ptr_const_s8, nl_21_t_in_0_shape_ch_prod_const_u32, 1, 10, 1498981504, 24, -124, nl_21_t_scratch_0_ptr_s32);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(21, 1, {(stai_ptr) nl_21_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END nl_21 */
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
  net_ctx->_inputs[0] = activations[0] + 10064;

  net_ctx->_outputs[0] = activations[0] + 508;
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

