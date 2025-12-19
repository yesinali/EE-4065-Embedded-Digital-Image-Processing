import cv2
import serial
import numpy as np
import os
import time
from tensorflow.keras.datasets import mnist

# --- Konfigürasyon ---
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
os.chdir(BASE_DIR)
OUTPUT_DIR = os.path.join(BASE_DIR, 'homework_outputs')
if not os.path.exists(OUTPUT_DIR): os.makedirs(OUTPUT_DIR)

SERIAL_PORT = 'COM7' # Kendi portunu kontrol et!
BAUD_RATE = 115200
W, H = 128, 128

def stm32_transfer(mode, data):
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=10)
        ser.write(bytes([mode, 0])) # Header gönder
        time.sleep(0.1)
        ser.write(bytes(data.flatten().tolist()))
        beklenen = len(data.flatten())
        gelen = ser.read(beklenen)
        ser.close()
        return np.frombuffer(gelen, dtype=np.uint8) if len(gelen) == beklenen else None
    except Exception as e:
        print(f"Hata: {e}"); return None

# --- 1. Q1 & Q2 İÇİN ORİJİNAL GÖRÜNTÜ ---
img_path = os.path.join(BASE_DIR, 'input_image.png')
if not os.path.exists(img_path):
    print("HATA: input_image.png bulunamadı!"); exit()

img = cv2.imread(img_path)
img_resized = cv2.resize(img, (W, H))

# Q1: Grayscale Otsu (Orijinal Görüntü)
gray = cv2.cvtColor(img_resized, cv2.COLOR_BGR2GRAY)
q1_res = stm32_transfer(1, gray)
if q1_res is not None:
    cv2.imwrite(os.path.join(OUTPUT_DIR, "Q1_Original_Otsu.png"), q1_res.reshape((H, W)))
    print("Q1: Orijinal görüntü Otsu tamamlandı.")

# Q2: Color Otsu (Orijinal Görüntü)
rgb = cv2.cvtColor(img_resized, cv2.COLOR_BGR2RGB)
q2_res = stm32_transfer(2, rgb.transpose(2, 0, 1))
if q2_res is not None:
    color_img = q2_res.reshape((3, H, W)).transpose(1, 2, 0)
    cv2.imwrite(os.path.join(OUTPUT_DIR, "Q2_Original_Color_Otsu.png"), cv2.cvtColor(color_img, cv2.COLOR_RGB2BGR))
    print("Q2: Renkli görüntü Otsu tamamlandı.")

# --- 2. Q3 İÇİN MNIST GÖRÜNTÜSÜ ---
print("Q3 için MNIST verisi çekiliyor...")
(x_train, _), _ = mnist.load_data()
mnist_img = x_train[np.random.randint(0, 5000)]
mnist_128 = cv2.resize(mnist_img, (W, H), interpolation=cv2.INTER_NEAREST) #

# Önce MNIST'i binary yapmamız lazım (Morfoloji için 0-255 arası değerler gerekir)
mnist_binary_raw = stm32_transfer(1, mnist_128)
if mnist_binary_raw is not None:
    mnist_binary = mnist_binary_raw.reshape((H, W))
    cv2.imwrite(os.path.join(OUTPUT_DIR, "Q3_MNIST_Input_Binary.png"), mnist_binary)
    
    # Q3: Morphological (Dilation & Erosion)
    for mode, name in {3: "Dilation", 4: "Erosion"}.items():
        res = stm32_transfer(mode, mnist_binary)
        if res is not None:
            cv2.imwrite(os.path.join(OUTPUT_DIR, f"Q3_MNIST_{name}.png"), res.reshape((H, W)))
    print("Q3: MNIST morfolojik işlemler tamamlandı.")

print(f"\nTüm sonuçlar '{OUTPUT_DIR}' klasöründe toplandı.")