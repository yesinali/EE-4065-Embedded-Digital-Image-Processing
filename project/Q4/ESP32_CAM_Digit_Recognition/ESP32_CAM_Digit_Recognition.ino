/**
 * ESP32-CAM Digit Recognition
 * El Yazısı Rakam Tanıma - 5 CNN Modeli
 *
 * Gerekli Kütüphaneler ve Versiyonlar:
 * =====================================
 * 1. ESP32 Board Package: v2.0.11 veya üzeri
 *    - Arduino IDE -> Preferences -> Additional Board URLs:
 *      https://dl.espressif.com/dl/package_esp32_index.json
 *    - Tools -> Board -> Boards Manager -> "esp32" ara -> v2.0.11 yükle
 *
 * 2. TensorFlow Lite Micro for ESP32 (Arduino Library Manager):
 *    - Sketch -> Include Library -> Manage Libraries
 *    - "TensorFlowLite_ESP32" ara -> v1.0.0 yükle (TANAKA Masayuki)
 *    VEYA GitHub'dan manuel:
 *    - https://github.com/tanakamasayuki/Arduino_TensorFlowLite_ESP32
 *
 * Arduino IDE Ayarları:
 * =====================
 * - Board: "AI Thinker ESP32-CAM"
 * - CPU Frequency: 240MHz
 * - Flash Mode: QIO
 * - Flash Size: 4MB (32Mb)
 * - Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"
 * - PSRAM: "Enabled"
 * - Upload Speed: 921600
 */

#include <TensorFlowLite_ESP32.h>
#include "esp_camera.h"
#include "esp_heap_caps.h"
#include "img_converters.h" // JPEG dönüşümü için
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <WebServer.h>
#include <WiFi.h>

// Model verisi - Colab'dan üretilen header
#include "model_data.h"

// ============================================
// AI-THINKER ESP32-CAM Pin Tanımları
// ============================================
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
#define LED_GPIO_NUM 4
#define FLASH_PWM_CHANNEL 2 // Kanal 0 kamera tarafından kullanılıyor
#define FLASH_BRIGHTNESS 0  // 0-255 arası parlaklık (düşük tutalım)

// ============================================
// Model Sabitleri
// ============================================
#define INPUT_WIDTH 28
#define INPUT_HEIGHT 28
#define INPUT_CHANNELS 1
#define INPUT_SIZE (INPUT_WIDTH * INPUT_HEIGHT * INPUT_CHANNELS)
#define NUM_CLASSES 10
#define TENSOR_ARENA_SIZE (100 * 1024) // 100KB

// ============================================
// Global Değişkenler
// ============================================
uint8_t *tensor_arena = nullptr;
const tflite::Model *model = nullptr;
tflite::MicroInterpreter *interpreter = nullptr;
TfLiteTensor *input_tensor = nullptr;
TfLiteTensor *output_tensor = nullptr;

int8_t inference_buffer[INPUT_SIZE];

const char *class_labels[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
String lastPredictionResult = "Sonuc Bekleniyor...";

// ============================================
// WiFi ve Web Sunucu Ayarları
// ============================================
const char *ssid = "godlessrose";
const char *password = "UsRiot_191";
WebServer server(80);

void handleRoot() {
  String html = "<html><head><meta charset='utf-8'><meta name='viewport' "
                "content='width=device-width, initial-scale=1'></head><body>";
  html += "<h1>ESP32-CAM Rakam Tanıma</h1>";
  html += "<h2>Tahmin: <span id='result' style='color:red; "
          "font-size:24px;'>...</span></h2>";
  html += "<img src='/capture' id='photo' style='width:320px; height:240px; "
          "border:1px solid #000;'>";
  html += "<br><br><button onclick='location.reload()'>Sayfayı Yenile</button>";
  html += "<p>Otomatik yenileme: <input type='checkbox' id='autoRefresh' "
          "checked></p>";
  html += "<script>";
  html += "setInterval(function(){";
  html += "  if(document.getElementById('autoRefresh').checked){";
  html += "    document.getElementById('photo').src='/capture?t='+new "
          "Date().getTime();";
  html += "  }";
  html += "  fetch('/status').then(res => res.text()).then(data => {";
  html += "    document.getElementById('result').innerText = data;";
  html += "  });";
  html += "}, 1000);"; // 1 saniyede bir dene
  html += "</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleStatus() { server.send(200, "text/plain", lastPredictionResult); }

void handleCapture() {
  // Kameradan görüntü al
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Kamera hatasi");
    return;
  }

  // Web arayüzünde modelin gördüğü gibi (NEGATİF + THRESHOLD) gözükmesi için
  // işle
  for (size_t i = 0; i < fb->len; i++) {
    uint8_t val = fb->buf[i];

    // Invert
    val = 255 - val;

    // Threshold / Noise Gate (Same as model input)
    if (val < 50) {
      val = 0;
    } else {
      // Contrast Stretch
      float fval = (float)val;
      fval = (fval - 50.0f) * (255.0f / (255.0f - 50.0f));
      if (fval > 255.0f)
        fval = 255.0f;
      val = (uint8_t)fval;
    }

    fb->buf[i] = val;
  }

  // Model GRISKALA (GRAYSCALE) çalışıyor, tarayıcı için JPEG'e çevirmemiz gerek
  // img_converters.h -> fmt2jpg
  uint8_t *jpg_buf = NULL;
  size_t jpg_len = 0;
  bool converted = fmt2jpg(fb->buf, fb->len, fb->width, fb->height, fb->format,
                           31, &jpg_buf, &jpg_len);

  // Frame buffer'ı serbest bırak
  esp_camera_fb_return(fb);

  if (!converted) {
    server.send(500, "text/plain", "JPEG donusum hatasi");
    return;
  }

  // Tarayıcıya gönder
  server.sendHeader("Content-Disposition", "inline; filename=capture.jpg");
  server.send_P(200, "image/jpeg", (const char *)jpg_buf, jpg_len);

  // JPEG buffer'ını temizle
  free(jpg_buf);
}

// ============================================
// Kamera Başlatma
// ============================================
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_QVGA; // 320x240
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // PSRAM kontrolü
  if (psramFound()) {
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
    Serial.println("PSRAM bulundu, 2 frame buffer kullaniliyor");
  } else {
    config.frame_size = FRAMESIZE_QQVGA; // 160x120
    config.fb_location = CAMERA_FB_IN_DRAM;
    Serial.println("PSRAM yok, kucuk frame boyutu kullaniliyor");
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Kamera baslatilamadi! Hata: 0x%x\n", err);
    return false;
  }

  // Kamera ayarları
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, 0);
    s->set_contrast(s, 1);
    s->set_saturation(s, 0);
  }

  Serial.println("Kamera basariyla baslatildi");
  return true;
}

// ============================================
// TensorFlow Lite Başlatma
// ============================================
bool initTFLite() {
  Serial.println("TensorFlow Lite baslatiliyor...");

  // Tensor arena bellekte ayır (PSRAM tercih)
  if (psramFound()) {
    tensor_arena =
        (uint8_t *)heap_caps_malloc(TENSOR_ARENA_SIZE, MALLOC_CAP_SPIRAM);
    Serial.println("Tensor arena PSRAM'da olusturuldu");
  } else {
    tensor_arena = (uint8_t *)malloc(TENSOR_ARENA_SIZE);
    Serial.println("Tensor arena DRAM'da olusturuldu");
  }

  if (tensor_arena == nullptr) {
    Serial.println("Tensor arena olusturulamadi!");
    return false;
  }

  // Model yükle
  model = tflite::GetModel(g_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.printf("Model versiyonu %d != desteklenen %d\n", model->version(),
                  TFLITE_SCHEMA_VERSION);
    return false;
  }
  Serial.printf("Model yuklendi, boyut: %d byte\n", g_model_len);

  // Op resolver - kullanılan operasyonlar
  static tflite::MicroMutableOpResolver<15> resolver;
  resolver.AddConv2D();
  resolver.AddDepthwiseConv2D();
  resolver.AddMaxPool2D();
  resolver.AddAveragePool2D();
  resolver.AddReshape();
  resolver.AddFullyConnected();
  resolver.AddSoftmax();
  resolver.AddRelu();
  resolver.AddRelu6();
  resolver.AddAdd();
  resolver.AddMul();
  resolver.AddMean();
  resolver.AddConcatenation();
  resolver.AddQuantize();
  resolver.AddDequantize();

  // Error reporter oluştur
  static tflite::MicroErrorReporter micro_error_reporter;
  tflite::ErrorReporter *error_reporter = &micro_error_reporter;

  // Interpreter oluştur
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, TENSOR_ARENA_SIZE, error_reporter);
  interpreter = &static_interpreter;

  // Tensor'ları ayır
  if (interpreter->AllocateTensors() != kTfLiteOk) {
    Serial.println("Tensor tahsisi basarisiz!");
    return false;
  }

  input_tensor = interpreter->input(0);
  output_tensor = interpreter->output(0);

  Serial.printf("Input tensor: [%d, %d, %d, %d]\n", input_tensor->dims->data[0],
                input_tensor->dims->data[1], input_tensor->dims->data[2],
                input_tensor->dims->data[3]);

  size_t used = interpreter->arena_used_bytes();
  Serial.printf("Tensor arena kullanimi: %d byte (%.1f KB)\n", used,
                used / 1024.0);

  Serial.println("TensorFlow Lite basariyla baslatildi");
  return true;
}

// ============================================
// Görüntü Yeniden Boyutlandırma (Bilinear)
// ============================================
void resizeImage(const uint8_t *src, int srcW, int srcH, int8_t *dst, int dstW,
                 int dstH, float scale, int zero_point) {
  // 1. Center Crop Calculation
  // We want to crop a square region from the center of the source image
  // to preserve aspect ratio and focus on the digit.
  int cropSize = min(srcW, srcH);
  int startX = (srcW - cropSize) / 2;
  int startY = (srcH - cropSize) / 2;

  // 2. Resize Logic
  float x_ratio = (float)(cropSize - 1) / (dstW - 1);
  float y_ratio = (float)(cropSize - 1) / (dstH - 1);

  for (int y = 0; y < dstH; y++) {
    for (int x = 0; x < dstW; x++) {
      float srcX = startX + x * x_ratio;
      float srcY = startY + y * y_ratio;

      int x1 = (int)srcX;
      int y1 = (int)srcY;
      int x2 = min(x1 + 1, srcW - 1);
      int y2 = min(y1 + 1, srcH - 1);

      float xFrac = srcX - x1;
      float yFrac = srcY - y1;

      // Bilinear interpolation
      // Note: accessing src with full width stride
      float val = (1 - xFrac) * (1 - yFrac) * src[y1 * srcW + x1] +
                  xFrac * (1 - yFrac) * src[y1 * srcW + x2] +
                  (1 - xFrac) * yFrac * src[y2 * srcW + x1] +
                  xFrac * yFrac * src[y2 * srcW + x2];

      // 3. Inversion (White/Light Background -> Black Background)
      val = 255.0f - val;

      // 4. Noise Gate / Thresholding
      // If the background isn't perfectly white (inverted -> black), clamp it
      // to 0 Adjust this threshold (e.g. 50) based on lighting conditions
      if (val < 50) {
        val = 0;
      } else {
        // Expand dynamic range for the digit
        // Map [50, 255] -> [0, 255]
        val = (val - 50) * (255.0f / (255.0f - 50.0f));
        if (val > 255.0f)
          val = 255.0f;
      }

      // 5. Normalization & Quantization
      float normalized = val / 255.0f;
      int8_t quantized = (int8_t)(normalized / scale + zero_point);
      dst[y * dstW + x] = quantized;
    }
  }
}

// ============================================
// Görüntü Ön İşleme
// ============================================
bool preprocessImage(camera_fb_t *fb) {
  if (fb == nullptr || fb->format != PIXFORMAT_GRAYSCALE) {
    return false;
  }

  float input_scale = input_tensor->params.scale;
  int input_zero_point = input_tensor->params.zero_point;

  resizeImage(fb->buf, fb->width, fb->height, inference_buffer, INPUT_WIDTH,
              INPUT_HEIGHT, input_scale, input_zero_point);

  memcpy(input_tensor->data.int8, inference_buffer, INPUT_SIZE);
  return true;
}

// ============================================
// Inference Çalıştır
// ============================================
int runInference() {
  unsigned long start = millis();

  if (interpreter->Invoke() != kTfLiteOk) {
    Serial.println("Inference basarisiz!");
    return -1;
  }

  unsigned long duration = millis() - start;
  Serial.printf("Inference suresi: %lu ms\n", duration);

  // Sonuçları al
  int8_t *output = output_tensor->data.int8;
  float output_scale = output_tensor->params.scale;
  int output_zero_point = output_tensor->params.zero_point;

  int predicted_class = 0;
  float max_prob = -1000.0f;

  Serial.println("\nSinif olasiliklari:");
  for (int i = 0; i < NUM_CLASSES; i++) {
    float prob = (output[i] - output_zero_point) * output_scale;
    Serial.printf("  %s: %.4f\n", class_labels[i], prob);

    if (prob > max_prob) {
      max_prob = prob;
      predicted_class = i;
    }
  }

  return predicted_class;
}

// ============================================
// Görüntü Yakala ve Tanı
// ============================================
void captureAndRecognize() {
  Serial.println("\n========================================");
  Serial.println("Goruntu yakalaniyor...");

  // Flash LED (PWM ile düşük parlaklık)
  ledcWrite(FLASH_PWM_CHANNEL, FLASH_BRIGHTNESS);
  delay(100);

  camera_fb_t *fb = esp_camera_fb_get();

  ledcWrite(FLASH_PWM_CHANNEL, 0);

  if (fb == nullptr) {
    Serial.println("Goruntu yakalanamadi!");
    return;
  }

  Serial.printf("Goruntu: %dx%d, %d byte\n", fb->width, fb->height, fb->len);

  if (!preprocessImage(fb)) {
    esp_camera_fb_return(fb);
    return;
  }

  esp_camera_fb_return(fb);

  int prediction = runInference();

  if (prediction >= 0) {
    lastPredictionResult = String(class_labels[prediction]);
    Serial.println("\n========================================");
    Serial.printf(">>> TAHMIN EDILEN RAKAM: %s <<<\n",
                  class_labels[prediction]);
    Serial.println("========================================\n");
  }
}

// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n");
  Serial.println("========================================");
  Serial.println("  ESP32-CAM Rakam Tanima");
  Serial.println("  TensorFlow Lite Micro");
  Serial.println("========================================\n");

  // Bellek bilgisi
  Serial.printf("Toplam heap: %d byte\n", ESP.getHeapSize());
  Serial.printf("Bos heap: %d byte\n", ESP.getFreeHeap());
  if (psramFound()) {
    Serial.printf("PSRAM boyutu: %d byte\n", ESP.getPsramSize());
    Serial.printf("Bos PSRAM: %d byte\n", ESP.getFreePsram());
  }
  Serial.println();

  // LED pin
  // LED PWM ayarları
  ledcSetup(FLASH_PWM_CHANNEL, 5000, 8); // 5 kHz, 8-bit
  ledcAttachPin(LED_GPIO_NUM, FLASH_PWM_CHANNEL);
  ledcWrite(FLASH_PWM_CHANNEL, 0); // Başlangıçta kapalı

  // Kamera başlat
  if (!initCamera()) {
    Serial.println("Kamera hatasi! Dongu durduruluyor.");
    while (1)
      delay(1000);
  }

  // TFLite başlat
  if (!initTFLite()) {
    Serial.println("TFLite hatasi! Dongu durduruluyor.");
    while (1)
      delay(1000);
  }

  Serial.println("\n========================================");
  Serial.println("Hazir! Enter'a basin veya 5 sn bekleyin.");
  Serial.println("Kamerayi bir rakama dogru tutun.");
  Serial.println("========================================\n");

  // WiFi Başlatma
  Serial.print("WiFi baglaniyor: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // WiFi bağlantısını bekle (opsiyonel: sonsuz döngü yapmayalım ki internetsiz
  // de çalışsın)
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi baglandi!");
    Serial.print("IP Adresi: http://");
    Serial.println(WiFi.localIP());

    server.on("/", handleRoot);
    server.on("/capture", handleCapture);
    server.on("/status", handleStatus);
    server.begin();

    Serial.println("Web sunucu baslatildi.");
  } else {
    Serial.println("\nWiFi baglantisi basarisiz. Devam ediliyor...");
  }
}

// ============================================
// LOOP
// ============================================
void loop() {
  server.handleClient(); // Web istemcileri dinle

  static unsigned long lastCapture = 0;

  // Serial'dan tetikleme
  if (Serial.available() > 0) {
    while (Serial.available())
      Serial.read();
    captureAndRecognize();
    lastCapture = millis();
  }
  // Otomatik (5 saniyede bir)
  else if (millis() - lastCapture > 5000) {
    captureAndRecognize();
    lastCapture = millis();
  }

  delay(100);
}
