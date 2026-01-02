import sys
import os
import random
import numpy as np
import scipy.signal as sig
from scipy.io import wavfile
import cmsisdsp as dsp
import cmsisdsp.mfcc as mfcc
from cmsisdsp.datatype import F32
from stm_ai_runner import AiRunner

# --- SETTINGS ---
COM_PORT = 'COM3'      
BAUD_RATE = 115200

# CHANGE THIS TO YOUR OWN FOLDER PATH
RECORDINGS_DIR = "recordings"

# --- PARAMETERS ---
FFT_SIZE = 1024
SAMPLE_RATE = 8000
NUM_MEL_FILTERS = 20
NUM_DCT_OUTPUTS = 13

# --- 1. FUNCTION: FEATURE EXTRACTION (CMSIS-DSP) ---
def extract_features_cmsis(wav_path):
    """
    Reads the audio file and converts it to the (1, 26) format expected by the Model.
    """
    try:
        # MFCC Init
        freq_min = 20
        freq_high = SAMPLE_RATE / 2
        window = sig.get_window("hamming", FFT_SIZE)

        filtLen, filtPos, packedFilters = mfcc.melFilterMatrix(
            F32, freq_min, freq_high, NUM_MEL_FILTERS, SAMPLE_RATE, FFT_SIZE
        )
        dctMatrixFilters = mfcc.dctMatrix(F32, NUM_DCT_OUTPUTS, NUM_MEL_FILTERS)

        mfccf32 = dsp.arm_mfcc_instance_f32()
        status = dsp.arm_mfcc_init_f32(
            mfccf32, FFT_SIZE, NUM_MEL_FILTERS, NUM_DCT_OUTPUTS,
            dctMatrixFilters, filtPos, filtLen, packedFilters, window
        )

        if status != 0: return None

        # Read File
        fs, sample = wavfile.read(wav_path)
        
        # Data Preparation
        sample = sample.astype(np.float32)
        target_len = 2 * FFT_SIZE
        
        # Cropping or Padding
        if len(sample) >= target_len:
            sample = sample[:target_len]
        else:
            sample = np.pad(sample, (0, target_len - len(sample)), "constant", constant_values=0)

        # Normalization
        max_val = np.max(np.abs(sample))
        if max_val > 0: sample = sample / max_val

        # MFCC Calculation
        tmp = np.zeros(FFT_SIZE + 2)
        part1 = dsp.arm_mfcc_f32(mfccf32, sample[:FFT_SIZE], tmp)
        part2 = dsp.arm_mfcc_f32(mfccf32, sample[FFT_SIZE:], tmp)
        
        mfcc_feature = np.concatenate((part1, part2))
        return mfcc_feature.reshape(1, -1).astype(np.float32)

    except Exception:
        return None

# --- 2. FUNCTION: BATCH TEST LOGIC ---
def run_batch_test(runner, model_name, folder_path, count=10):
    """
    Selects random 'count' files from the specified folder and tests them.
    """
    print(f"\n>>> BATCH TEST STARTING ({count} Files) <<<")
    
    if not os.path.exists(folder_path):
        print(f"ERROR: '{folder_path}' folder not found!")
        return

    # Get only .wav files
    all_files = [f for f in os.listdir(folder_path) if f.endswith('.wav')]
    
    if not all_files:
        print("No .wav files in the folder.")
        return

    # Make a random selection (If few files, take all)
    selected_files = random.sample(all_files, min(count, len(all_files)))
    
    correct_count = 0
    total_tested = 0

    print(f"{'FILENAME':<30} | {'REAL':<6} | {'PRED':<6} | {'RESULT'}")
    print("-" * 65)

    for filename in selected_files:
        # 1. Get real label from filename (Ex: 3_jackson_0.wav -> 3)
        try:
            real_label = int(filename.split('_')[0])
        except:
            continue # Skip if filename doesn't match format

        # 2. Extract features
        full_path = os.path.join(folder_path, filename)
        input_data = extract_features_cmsis(full_path)
        
        if input_data is None: continue

        # 3. Send to Board
        outputs, _ = runner._drv.invoke_sample([input_data], name=model_name)
        
        if outputs:
            # 4. Find Prediction
            prediction = np.argmax(outputs[0])
            
            # 5. Compare
            is_correct = (prediction == real_label)
            icon = "✅" if is_correct else "❌"
            
            print(f"{filename:<30} | {real_label:<6} | {prediction:<6} | {icon}")
            
            if is_correct:
                correct_count += 1
            total_tested += 1
            
    print("-" * 65)
    if total_tested > 0:
        acc = (correct_count / total_tested) * 100
        print(f"RESULT: {correct_count} out of {total_tested} files are correct.")
        print(f"ACCURACY RATE: {acc:.2f}%")
    else:
        print("No files could be tested.")

# --- 3. MAIN FUNCTION ---
def main():
    print("--- ST AI Runner: FSDD Batch Tester ---")
    runner = AiRunner(debug=False)

    # Connect
    print(f"1. Connecting to port {COM_PORT}...")
    try:
        runner.connect('serial', port=COM_PORT, baudrate=BAUD_RATE)
    except: pass

    if not runner._drv:
        print("CRITICAL ERROR: No driver. Check USB connection.")
        return

    # Find Model
    names = runner._drv.discover()
    if not names:
        print("ERROR: Model not found.")
        return
    model_name = names[0]
    print(f"   Model found: {model_name}")
    
    # BATCH TEST
    run_batch_test(runner, model_name, RECORDINGS_DIR, count=20)

    # Close
    runner.disconnect()
    print("\nConnection closed.")

if __name__ == '__main__':
    main()