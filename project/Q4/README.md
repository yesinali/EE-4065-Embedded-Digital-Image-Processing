# El Yazısı Rakam Tanıma - ESP32-CAM

5 CNN modeli ile el yazısı rakam tanıma sistemi (SqueezeNet, EfficientNet, ResNet, MobileNet, ShuffleNet).

## 📦 Gerekli Kütüphaneler

### ESP32 Board Package
| Paket | Versiyon | Kurulum |
|-------|----------|---------|
| ESP32 by Espressif | **v2.0.11** veya üzeri | Board Manager |

**Board Manager URL:**
```
https://dl.espressif.com/dl/package_esp32_index.json
```

### Arduino Kütüphaneleri
| Kütüphane | Versiyon | Geliştirici |
|-----------|----------|-------------|
| TensorFlowLite_ESP32 | **v1.0.0** | TANAKA Masayuki |

**GitHub (Manuel kurulum):**
- https://github.com/tanakamasayuki/Arduino_TensorFlowLite_ESP32

---

## ⚙️ Arduino IDE Ayarları

| Ayar | Değer |
|------|-------|
| Board | AI Thinker ESP32-CAM |
| CPU Frequency | 240MHz (WiFi/BT) |
| Flash Mode | QIO |
| Flash Size | 4MB (32Mb) |
| Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
| PSRAM | Enabled |
| Upload Speed | 921600 |

---

## 📁 Proje Yapısı

```
hw6-ag/
├── digit_recognition_training.ipynb    # Colab eğitim notebook'u
├── README.md
└── ESP32_CAM_Digit_Recognition/        # Arduino projesi
    ├── ESP32_CAM_Digit_Recognition.ino # Ana sketch
    └── model_data.h                    # Model verisi (Colab'dan)
```

---

## 🚀 Kullanım

### Adım 1: Model Eğitimi (Colab)
1. `digit_recognition_training.ipynb` dosyasını Google Colab'a yükleyin
2. Runtime → Change runtime type → GPU seçin
3. Tüm hücreleri çalıştırın
4. `digit_recognition_headers.zip` dosyasını indirin

### Adım 2: Model Hazırlığı
1. ZIP dosyasını açın
2. İstediğiniz modeli seçin (örn: `squeezenet_model.h`)
3. Dosyayı `ESP32_CAM_Digit_Recognition/` klasörüne kopyalayın
4. `model_data.h` dosyasını düzenleyin:

```cpp
// Bu satırı ekleyin:
#include "squeezenet_model.h"

// Bu satırları değiştirin:
const unsigned int g_model_len = squeezenet_model_len;
alignas(8) const unsigned char* g_model = squeezenet_model;
```

### Adım 3: Yükleme
1. Arduino IDE'de `ESP32_CAM_Digit_Recognition.ino` açın
2. Board ayarlarını yapın (yukarıdaki tabloya bakın)
3. Derle ve yükle
4. Serial Monitor açın (115200 baud)

### Adım 4: Test
- Kamerayı bir rakama doğru tutun
- Her 5 saniyede otomatik tanıma yapar
- Serial Monitor'dan Enter'a basarak manuel tetikleyin

---

## 🧠 Model Karşılaştırması

| Model | Boyut | Özellik |
|-------|-------|---------|
| SqueezeNet | ~80-150 KB | Fire modülleri |
| EfficientNet | ~100-200 KB | Compound scaling |
| ResNet | ~80-180 KB | Residual bağlantılar |
| MobileNet | ~60-120 KB | Depthwise separable conv |
| ShuffleNet | ~50-100 KB | Channel shuffle |

---

## 🔧 Sorun Giderme

| Sorun | Çözüm |
|-------|-------|
| Derleme hatası | ESP32 board v2.0.11+ olduğundan emin olun |
| Kamera hatası | Pin konfigürasyonunu kontrol edin |
| Bellek hatası | PSRAM'in aktif olduğundan emin olun |
| Model hatası | TFLite model formatını kontrol edin |
