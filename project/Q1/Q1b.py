import serial
import numpy as np
import cv2
import time

# --- SETTINGS ---
PORT = 'COM9'      # Check your port
BAUD_RATE = 921600 # NOW VERY FAST
WIDTH = 320
HEIGHT = 240
TOTAL_PIXELS = WIDTH * HEIGHT

def main():
    print(f"Establishing connection: {PORT} @ {BAUD_RATE}...")
    
    try:
        # Timeout set to None = Waits forever (Does not give up before data arrives)
        ser = serial.Serial(PORT, BAUD_RATE, timeout=None, dsrdtr=False, rtscts=False)
        ser.dtr = False 
        ser.rts = False
        time.sleep(2) 
        ser.reset_input_buffer()
        
    except Exception as e:
        print(f"ERROR: {e}")
        return

    print("--- READY ---")
    print("Now press the RST button on ESP32, the image will start streaming.")

    while True:
        # 1. Send Request
        ser.write(b'1')
        
        # 2. WAIT HERE until exactly 76800 bytes are read
        # Since timeout=None, it won't proceed with incomplete data, it waits.
        data = ser.read(TOTAL_PIXELS)
        
        # 3. Process if data arrived
        if len(data) == TOTAL_PIXELS:
            img_array = np.frombuffer(data, dtype=np.uint8)
            img_matrix = img_array.reshape((HEIGHT, WIDTH))
            
            cv2.imshow("ESP32 Fast Image", img_matrix)
            
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
        else:
            # Very unlikely to reach here but check anyway
            print(f"Unexpected error. Bytes received: {len(data)}")

    ser.close()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()