import sys
import os
import struct
import random
import time
import numpy as np
import cv2  # OpenCV Required
from stm_ai_runner import AiRunner

# --- SETTINGS ---
COM_PORT = 'COM3'      
BAUD_RATE = 115200

# Dataset Paths
DATASET_DIR = "MNIST-dataset"
TEST_IMG_NAME = "t10k-images.idx3-ubyte"
TEST_LABEL_NAME = "t10k-labels.idx1-ubyte"

TEST_IMG_PATH = os.path.join(DATASET_DIR, TEST_IMG_NAME)
TEST_LABEL_PATH = os.path.join(DATASET_DIR, TEST_LABEL_NAME)

# --- 1. LOAD MNIST ---
def load_mnist_images(path):
    print(f"Reading image file: {path}")
    try:
        with open(path, "rb") as f:
            magic, num, rows, cols = struct.unpack(">IIII", f.read(16))
            images = np.frombuffer(f.read(), dtype=np.uint8).reshape(num, rows, cols)
        return images
    except FileNotFoundError:
        print("❌ ERROR: Image file not found!")
        return None

def load_mnist_labels(path):
    print(f"Reading label file: {path}")
    try:
        with open(path, "rb") as f:
            magic, num = struct.unpack(">II", f.read(8))
            labels = np.frombuffer(f.read(), dtype=np.uint8)
        return labels
    except FileNotFoundError:
        print("❌ ERROR: Label file not found!")
        return None

# --- 2. FEATURE EXTRACTION (FLOAT32) ---
def extract_float_features(image):
    """
    1. Calculate moments (binaryImage=True as in training)
    2. Calculate Hu Moments
    3. Return as float32 WITHOUT ANY CONVERSION.
    """
    # cv2.moments(img, True) was used in training code (Listing11_6.py).
    # Therefore, we use binaryImage=True here as well.
    moments = cv2.moments(image, binaryImage=True) 
    
    # Hu Moments (7 features)
    hu_moments = cv2.HuMoments(moments).reshape(7)
    
    # --- CRITICAL POINT ---
    # No scale, zero_point, or int8 conversion anymore.
    # Send directly as float32 in shape (1, 7).
    return hu_moments.astype(np.float32).reshape(1, 7)

# --- 3. TEST LOOP ---
def run_batch_test(runner, model_name, images, labels, count=20):
    print(f"\n>>> MNIST FLOAT TEST ({count} Random Images) <<<")
    
    total_samples = len(images)
    indices = random.sample(range(total_samples), count)
    correct_count = 0
    
    print(f"{'IDX':<6} | {'REAL':<6} | {'PRED':<6} | {'RESULT'}")
    print("-" * 50)

    for idx in indices:
        img = images[idx]
        real_label = labels[idx]
        
        # Extract features (Float)
        input_data = extract_float_features(img)
        
        try:
            # Direct driver invocation
            outputs, _ = runner._drv.invoke_sample([input_data], name=model_name)
        except Exception as e:
            print(f"Communication Error: {e}")
            break

        if outputs:
            # Output will be Float probabilities (or Logits).
            # Get the highest one (argmax).
            prediction = np.argmax(outputs[0])
            
            is_correct = (prediction == real_label)
            icon = "✅" if is_correct else "❌"
            
            print(f"{idx:<6} | {real_label:<6} | {prediction:<6} | {icon}")
            
            if is_correct:
                correct_count += 1
            
            time.sleep(0.01)

    print("-" * 50)
    acc = (correct_count / count) * 100
    print(f"RESULT: {correct_count}/{count} correct.")
    print(f"ACCURACY RATE: {acc:.1f}%")

def main():
    print("--- ST AI Runner: MNIST Float Mode ---")
    
    # 1. Load Files
    images = load_mnist_images(TEST_IMG_PATH)
    labels = load_mnist_labels(TEST_LABEL_PATH)
    
    if images is None or labels is None:
        return
    print(f"   Dataset loaded ({len(images)} images).")

    # 2. Connect
    runner = AiRunner(debug=False)
    print(f"2. Connecting to port {COM_PORT}...")
    
    try:
        runner.connect('serial', port=COM_PORT, baudrate=BAUD_RATE)
    except: pass

    if not runner._drv:
        print("CRITICAL ERROR: No driver. Unplug and replug the USB cable.")
        return

    # 3. Find Model
    try:
        names = runner._drv.discover()
    except Exception:
        print("ERROR: Board did not respond. Please press the RESET button.")
        return

    if not names:
        print("Model not found.")
        return
        
    model_name = names[0]
    print(f"   Model Found: {model_name}")
    print(f"   Mode: FLOAT32 (Quantization Disabled)")

    # 4. Start Test
    run_batch_test(runner, model_name, images, labels, count=20)

    runner.disconnect()
    print("\nConnection closed.")

if __name__ == '__main__':
    main()