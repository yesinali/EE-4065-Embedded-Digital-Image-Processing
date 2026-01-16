/**
 * Model Data Header
 *
 * Bu dosya Colab notebook'tan üretilen model verisini içerir.
 *
 * KULLANIM:
 * =========
 * 1. digit_recognition_training.ipynb dosyasını Colab'da çalıştırın
 * 2. İndirilen header dosyasını (örn: squeezenet_model.h) bu klasöre kopyalayın
 * 3. Aşağıdaki placeholder'ı gerçek model verisiyle değiştirin
 *
 * MÜSAİT MODELLER:
 * ================
 * - squeezenet_model.h   (~80-150 KB)
 * - efficientnet_model.h (~100-200 KB)
 * - resnet_model.h       (~80-180 KB)
 * - mobilenet_model.h    (~60-120 KB)
 * - shufflenet_model.h   (~50-100 KB)
 */

#ifndef MODEL_DATA_H
#define MODEL_DATA_H

// ============================================
// ÖRNEK: Colab'dan üretilen header'ı include edin
// ============================================
// #include "squeezenet_model.h"
// #include "efficientnet_model.h"
// #include "resnet_model.h"
// #include "mobilenet_model.h"
// #include "shufflenet_model.h"

#include "shufflenet_model.h"

// ============================================
// PLACEHOLDER MODEL (Geçici - Değiştirin!)
// ============================================
// Not: Bu placeholder sadece derleme testi içindir.
// Gerçek inference için Colab'dan üretilen modeli kullanın.

/*
const unsigned int g_model_len = 4;

alignas(8) const unsigned char g_model[] = {0x00, 0x00, 0x00, 0x00};
*/

const unsigned int g_model_len = shufflenet_model_len;
const unsigned char *g_model = shufflenet_model;

// ============================================
// Colab'dan model kullanırken:
// ============================================
// Örnek: squeezenet_model.h dosyasını include ettiyseniz:
//
// #include "squeezenet_model.h"
// const unsigned int g_model_len = squeezenet_model_len;
// alignas(8) const unsigned char* g_model = squeezenet_model;
//
// VEYA doğrudan header'daki isimleri kullanın:
// model_data.h yerine squeezenet_model.h'ı include edin ve
// ana kodda g_model yerine squeezenet_model kullanın.

#endif // MODEL_DATA_H
