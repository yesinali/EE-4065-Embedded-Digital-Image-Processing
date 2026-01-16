/*
 * ESP32-CAM Optimized TFLite Inference for st_yolo_lc_v1 Object Detection
 *
 * Model: st_yolo_lc_v1 (96x96, 5 classes)
 * Classes: eight(8), five(5), nine(9), seven(7), six(6)
 * Quantization: INT8
 *
 * LIBRARY: "tflite-micro-esp-examples" or "TFLite Micro ESP32"
 * Install from Arduino Library Manager
 */

#include "esp_camera.h"
#include "model_data.h"
#include <Arduino.h>

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include <math.h>

// --- CAMERA PINS (AI-THINKER ESP32-CAM) ---
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

// --- MODEL SETTINGS ---
#define MODEL_INPUT_WIDTH 96
#define MODEL_INPUT_HEIGHT 96
#define MODEL_INPUT_CHANNELS 3

// st_yolo_lc_v1: stride=16, grid=96/16=6
#define GRID_SIZE 6
#define NUM_ANCHORS 5
#define NUM_CLASSES 5

// YOLO output: grid * grid * num_anchors * (5 + num_classes)
// For each anchor: [x, y, w, h, objectness, class0, class1, class2, class3,
// class4]
#define VALUES_PER_ANCHOR (5 + NUM_CLASSES)

// Tensor Arena size - Model ~345KB, arena ~500KB is sufficient
const int kTensorArenaSize = 500 * 1024;
uint8_t *tensor_arena = nullptr;

// Class Names (according to data.yaml order: eight, five, nine, seven, six)
const char *class_names[] = {"8", "5", "9", "7", "6"};

// TFLite Global Variables
tflite::MicroInterpreter *interpreter = nullptr;
TfLiteTensor *input_tensor = nullptr;
TfLiteTensor *output_tensor = nullptr;

// Quantization parametreleri
float input_scale = 1.0f;
int32_t input_zero_point = 0;
float output_scale = 1.0f;
int32_t output_zero_point = 0;

// Detection threshold values
const float CONF_THRESHOLD = 0.25f;
const float NMS_THRESHOLD = 0.45f;

// Function Prototypes
bool initCamera();
void setupInference();
void runInference();
void preprocessImage(camera_fb_t *fb);
float sigmoid(float x);
float dequantize(int8_t value, float scale, int32_t zero_point);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("ESP32-CAM DIGIT DETECTION (Optimized)");
  Serial.println("Model: st_yolo_lc_v1 (96x96, 5 classes)");
  Serial.println("Classes: 5, 6, 7, 8, 9");
  Serial.println("========================================\n");

  // PSRAM check
  if (psramInit()) {
    Serial.printf("✅ PSRAM Active! Size: %d bytes (%.2f MB)\n",
                  ESP.getPsramSize(), ESP.getPsramSize() / 1024.0 / 1024.0);
  } else {
    Serial.println("⚠️ WARNING: PSRAM not found! Model might not work.");
  }

  // Init camera
  if (!initCamera()) {
    Serial.println("❌ Camera init failed! System halting...");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("✅ Camera ready");

  // Allocate memory for Tensor Arena (use PSRAM)
  tensor_arena = (uint8_t *)ps_malloc(kTensorArenaSize);
  if (!tensor_arena) {
    Serial.println("❌ Failed to allocate memory for Tensor Arena!");
    while (1) {
      delay(1000);
    }
  }
  Serial.printf("✅ Tensor arena allocated: %d bytes\n", kTensorArenaSize);

  // TFLite setup
  setupInference();

  Serial.println("\n🚀 System ready! You can show a digit...\n");
}

void loop() {
  // Capture image from camera
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("❌ Camera error: Failed to capture image!");
    delay(1000);
    return;
  }

  // Process image and run model
  preprocessImage(fb);
  runInference();

  // Release frame buffer
  esp_camera_fb_return(fb);

  // Wait a bit (prevent overheating)
  delay(500);
}

void setupInference() {
  // Load model
  const tflite::Model *model = tflite::GetModel(model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.printf("❌ Model schema error! Expected: %d, Got: %lu\n",
                  TFLITE_SCHEMA_VERSION, model->version());
    return;
  }
  Serial.println("✅ Model loaded");

  // Op Resolver - Add necessary operators
  // Necessary operators for st_yolo_lc_v1
  static tflite::MicroMutableOpResolver<20> resolver;

  // Basic operators
  resolver.AddConv2D();
  resolver.AddDepthwiseConv2D();
  resolver.AddMaxPool2D();
  resolver.AddReshape();
  resolver.AddFullyConnected();
  resolver.AddSoftmax();
  resolver.AddLogistic();
  resolver.AddQuantize();
  resolver.AddDequantize();
  resolver.AddMul();
  resolver.AddAdd();
  resolver.AddMean();
  resolver.AddPad();
  resolver.AddConcatenation();
  resolver.AddRelu();
  resolver.AddRelu6();
  resolver.AddLeakyRelu();
  resolver.AddTranspose();
  resolver.AddResizeBilinear();
  resolver.AddSplit();

  // Create Interpreter (New API - no error_reporter)
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  // Allocate tensor memory
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    Serial.println("❌ AllocateTensors failed!");
    return;
  }
  Serial.printf("✅ Tensor allocation done. Arena used: %zu bytes\n",
                interpreter->arena_used_bytes());

  // Get input and output tensors
  input_tensor = interpreter->input(0);
  output_tensor = interpreter->output(0);

  // Input tensor info
  Serial.println("\n📊 Input Tensor Info:");
  Serial.printf("   - Type: %s\n", input_tensor->type == kTfLiteInt8 ? "INT8"
                                   : input_tensor->type == kTfLiteUInt8
                                       ? "UINT8"
                                       : "FLOAT32");
  Serial.printf("   - Size: [%d, %d, %d, %d]\n", input_tensor->dims->data[0],
                input_tensor->dims->data[1], input_tensor->dims->data[2],
                input_tensor->dims->data[3]);

  // Get quantization parameters
  if (input_tensor->type == kTfLiteInt8 || input_tensor->type == kTfLiteUInt8) {
    input_scale = input_tensor->params.scale;
    input_zero_point = input_tensor->params.zero_point;
    Serial.printf("   - Scale: %.6f, Zero Point: %d\n", input_scale,
                  input_zero_point);
  }

  // Output tensor info
  Serial.println("\n📊 Output Tensor Info:");
  Serial.printf("   - Type: %s\n", output_tensor->type == kTfLiteInt8 ? "INT8"
                                   : output_tensor->type == kTfLiteUInt8
                                       ? "UINT8"
                                       : "FLOAT32");
  Serial.printf("   - Size: [%d, %d, %d, %d]\n", output_tensor->dims->data[0],
                output_tensor->dims->data[1], output_tensor->dims->data[2],
                output_tensor->dims->data[3]);

  if (output_tensor->type == kTfLiteInt8 ||
      output_tensor->type == kTfLiteUInt8) {
    output_scale = output_tensor->params.scale;
    output_zero_point = output_tensor->params.zero_point;
    Serial.printf("   - Scale: %.6f, Zero Point: %d\n", output_scale,
                  output_zero_point);
  }

  Serial.println("\n✅ Model ready!");
}

void preprocessImage(camera_fb_t *fb) {
  // Center-crop + resize QVGA (320x240) image to 96x96

  // Source image dimensions
  int src_width = fb->width;
  int src_height = fb->height;

  // Target dimensions
  int dst_width = MODEL_INPUT_WIDTH;
  int dst_height = MODEL_INPUT_HEIGHT;

  // Start points for center crop
  // Get min(width, height) to crop a square region
  int crop_size = min(src_width, src_height);
  int start_x = (src_width - crop_size) / 2;
  int start_y = (src_height - crop_size) / 2;

  // For each pixel
  for (int y = 0; y < dst_height; y++) {
    for (int x = 0; x < dst_width; x++) {
      // Calculate source coordinates (nearest neighbor interpolation)
      int src_x = start_x + (x * crop_size / dst_width);
      int src_y = start_y + (y * crop_size / dst_height);

      // Each pixel is 2 bytes in RGB565 format
      int src_idx = (src_y * src_width + src_x) * 2;

      // Convert RGB565 to RGB888
      uint8_t hb = fb->buf[src_idx];
      uint8_t lb = fb->buf[src_idx + 1];
      uint16_t rgb565 = (hb << 8) | lb;

      // Extract color components
      // RGB565: RRRRR GGGGGG BBBBB
      uint8_t r = ((rgb565 >> 11) & 0x1F);
      uint8_t g = ((rgb565 >> 5) & 0x3F);
      uint8_t b = (rgb565 & 0x1F);

      // Expand from 5/6-bit to 8-bit
      r = (r << 3) | (r >> 2);
      g = (g << 2) | (g >> 4);
      b = (b << 3) | (b >> 2);

      // Target tensor index
      int dst_idx = (y * dst_width + x) * MODEL_INPUT_CHANNELS;

      // Write value based on tensor type
      if (input_tensor->type == kTfLiteInt8) {
        // INT8: quantize pixel value
        input_tensor->data.int8[dst_idx] = (int8_t)(r - 128);
        input_tensor->data.int8[dst_idx + 1] = (int8_t)(g - 128);
        input_tensor->data.int8[dst_idx + 2] = (int8_t)(b - 128);
      } else if (input_tensor->type == kTfLiteUInt8) {
        // UINT8: direct 0-255 values
        input_tensor->data.uint8[dst_idx] = r;
        input_tensor->data.uint8[dst_idx + 1] = g;
        input_tensor->data.uint8[dst_idx + 2] = b;
      } else {
        // FLOAT32: normalize between 0-1
        input_tensor->data.f[dst_idx] = r / 255.0f;
        input_tensor->data.f[dst_idx + 1] = g / 255.0f;
        input_tensor->data.f[dst_idx + 2] = b / 255.0f;
      }
    }
  }
}

void runInference() {
  if (!interpreter || !input_tensor || !output_tensor) {
    Serial.println("❌ Interpreter not ready yet!");
    return;
  }

  unsigned long start = millis();

  // Run model
  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    Serial.println("❌ Invoke failed!");
    return;
  }

  unsigned long duration = millis() - start;

  // Analyze output
  // st_yolo_lc_v1 output: [1, 6, 6, 50] -> 6x6 grid, 5 anchor, 10 values/anchor

  float best_confidence = 0.0f;
  int best_class = -1;
  int best_grid_x = -1, best_grid_y = -1, best_anchor = -1;

  // Output data
  int8_t *output_int8 = nullptr;
  float *output_float = nullptr;
  bool is_quantized = (output_tensor->type == kTfLiteInt8);

  if (is_quantized) {
    output_int8 = output_tensor->data.int8;
  } else {
    output_float = output_tensor->data.f;
  }

  // Scan all grid cells and anchors
  for (int gy = 0; gy < GRID_SIZE; gy++) {
    for (int gx = 0; gx < GRID_SIZE; gx++) {
      for (int a = 0; a < NUM_ANCHORS; a++) {
        // Calculate output tensor index
        // Format: [batch, grid_y, grid_x, anchor * values_per_anchor]
        int base_idx = (gy * GRID_SIZE * NUM_ANCHORS * VALUES_PER_ANCHOR) +
                       (gx * NUM_ANCHORS * VALUES_PER_ANCHOR) +
                       (a * VALUES_PER_ANCHOR);

        // Objectness value (index 4)
        float objectness;
        if (is_quantized) {
          objectness = sigmoid(dequantize(output_int8[base_idx + 4],
                                          output_scale, output_zero_point));
        } else {
          objectness = sigmoid(output_float[base_idx + 4]);
        }

        // Threshold check
        if (objectness < CONF_THRESHOLD)
          continue;

        // Check class scores
        float max_class_score = 0.0f;
        int max_class_idx = -1;

        for (int c = 0; c < NUM_CLASSES; c++) {
          float class_score;
          if (is_quantized) {
            class_score = sigmoid(dequantize(output_int8[base_idx + 5 + c],
                                             output_scale, output_zero_point));
          } else {
            class_score = sigmoid(output_float[base_idx + 5 + c]);
          }

          if (class_score > max_class_score) {
            max_class_score = class_score;
            max_class_idx = c;
          }
        }

        // Final confidence = objectness * class_score
        float final_conf = objectness * max_class_score;

        if (final_conf > best_confidence) {
          best_confidence = final_conf;
          best_class = max_class_idx;
          best_grid_x = gx;
          best_grid_y = gy;
          best_anchor = a;
        }
      }
    }
  }

  // Print result
  Serial.printf("⚡ Inference: %lu ms | ", duration);

  if (best_confidence > CONF_THRESHOLD && best_class >= 0 &&
      best_class < NUM_CLASSES) {
    Serial.printf("🎯 DETECTED: [ %s ] (Conf: %.1f%%)\n",
                  class_names[best_class], best_confidence * 100.0f);
    Serial.printf("   Grid: (%d, %d), Anchor: %d\n", best_grid_x, best_grid_y,
                  best_anchor);
  } else {
    Serial.println("❌ No digit detected");
  }
}

bool initCamera() {
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size = FRAMESIZE_QVGA; // 320x240
  config.jpeg_quality = 10;
  config.fb_count = 1;

  // Use high-quality frame buffer if PSRAM is available
  if (psramFound()) {
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("❌ Camera init error: 0x%x\n", err);
    return false;
  }

  // Camera sensor settings
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, 0);                 // -2 ile 2 arası
    s->set_contrast(s, 0);                   // -2 ile 2 arası
    s->set_saturation(s, 0);                 // -2 ile 2 arası
    s->set_whitebal(s, 1);                   // AWB on
    s->set_awb_gain(s, 1);                   // AWB gain on
    s->set_wb_mode(s, 0);                    // Auto WB
    s->set_exposure_ctrl(s, 1);              // AEC on
    s->set_aec2(s, 0);                       // AEC DSP off
    s->set_gain_ctrl(s, 1);                  // AGC on
    s->set_agc_gain(s, 0);                   // AGC gain 0
    s->set_gainceiling(s, (gainceiling_t)6); // Gain ceiling
    s->set_bpc(s, 0);                        // BPC off
    s->set_wpc(s, 1);                        // WPC on
    s->set_raw_gma(s, 1);                    // Raw GMA on
    s->set_lenc(s, 1);                       // Lens correction on
    s->set_hmirror(s, 0);                    // No horizontal mirror
    s->set_vflip(s, 0);                      // No vertical flip
    s->set_dcw(s, 1);                        // DCW on
  }

  return true;
}

// Sigmoid activation function
inline float sigmoid(float x) { return 1.0f / (1.0f + expf(-x)); }

// INT8 dequantization
inline float dequantize(int8_t value, float scale, int32_t zero_point) {
  return (static_cast<float>(value) - static_cast<float>(zero_point)) * scale;
}
