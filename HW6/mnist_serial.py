import sys
import os
import struct
import random
import time
import numpy as np
import cv2  # OpenCV Required
from stm_ai_runner import AiRunner

class Logger(object):
    def __init__(self, filename="mnist_console_output.txt"):
        self.terminal = sys.stdout
        self.log = open(filename, "w", encoding="utf-8")

    def write(self, message):
        self.terminal.write(message)
        self.log.write(message)

    def flush(self):
        self.terminal.flush()
        self.log.flush()

# --- SETTINGS ---
COM_PORT = 'COM8'      
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

# --- 2. DATA PREPARATION ---
def preprocess_image(image):
    """
    Prepare image for the CNN model.
    Model expects: (28, 28, 1)
    Training range: 0.0 to 1.0 (Float32) or Quantized Int8
    
    We will send Float32 (0.0 - 1.0) and let the ai_runner/STM32 handle
    input quantization if needed, OR send raw bytes if preferred.
    Given the training was / 255.0, we normalize here.
    """
    # Normalize to match quantized model expectation
    # Model info indicates: Int8, Zeropoint -128, Scale ~1/255
    # We map Uint8 [0, 255] to Int8 [-128, 127]
    
    # Add channel dimension: (28, 28) -> (28, 28, 1)
    img_reshaped = np.expand_dims(image, axis=-1)

    # Convert 0..255 to -128..127
    img_int8 = (img_reshaped.astype(np.int16) - 128).astype(np.int8)
    
    # Add batch dimension: (1, 28, 28, 1)
    return np.expand_dims(img_int8, axis=0)

# --- 3. TEST LOOP ---
def run_batch_test(runner, model_name, images, labels, count=20):
    print(f"\n>>> MNIST CNN TEST ({count} Random Images) <<<")
    
    total_samples = len(images)
    indices = random.sample(range(total_samples), count)
    correct_count = 0
    
    print(f"{'IDX':<6} | {'REAL':<6} | {'PRED':<6} | {'RESULT'}")
    print("-" * 50)

    for idx in indices:
        img = images[idx]
        real_label = labels[idx]
        
        # Prepare Input
        input_data = preprocess_image(img)
        
        try:
            # Direct driver invocation
            # ai_runner will handle serialization of the numpy array
            outputs, _ = runner._drv.invoke_sample([input_data], name=model_name)
        except Exception as e:
            print(f"Communication Error: {e}")
            break

        if outputs:
            # Output is typically softmax probabilities
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
    # Redirect stdout to both console and file
    sys.stdout = Logger()

    print("--- ST AI Runner: MNIST Int8 Mode ---")
    
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
    print(f"   Mode: CNN INPUT (28x28x1) [Int8]")

    # 4. Start Test
    run_batch_test(runner, model_name, images, labels, count=20)

    runner.disconnect()
    print("\nConnection closed.")

if __name__ == '__main__':
    main()