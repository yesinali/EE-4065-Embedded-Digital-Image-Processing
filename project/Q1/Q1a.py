import cv2
import numpy as np
import os

def main():
    # File name (Must be in the same folder as the code)
    filename = 'input.jpeg'
    
    # 1. Read the image as grayscale
    # '0' parameter ensures the image is read in black and white
    img = cv2.imread(filename, 0)
    
    # File reading check
    if img is None:
        print(f"ERROR: File '{filename}' not found!")
        print("Please ensure the image is in the same folder as the code file.")
        return

    # Target object size specified in the question
    OBJECT_SIZE = 1000

    # 2. Finding the threshold value (P-tile algorithm)
    # Flatten the image matrix into a one-dimensional array
    flat_pixels = img.flatten()
    
    # Sort pixel brightness from smallest to largest
    flat_pixels.sort()
    
    # Find the value that separates the brightest 1000 pixels
    # The end of the array is the brightest pixels. We take the 1000th pixel from the end.
    threshold_value = flat_pixels[-OBJECT_SIZE]
    
    print(f"Calculated Threshold Value: {threshold_value}")

    # 3. Apply the thresholding process
    # Values greater than threshold_value become 255 (White), smaller ones become 0 (Black)
    _, binary_img = cv2.threshold(img, threshold_value, 255, cv2.THRESH_BINARY)

    # 4. Show results
    cv2.imshow('Original Image', img)
    cv2.imshow('Result (Brightest 1000 Pixels)', binary_img)
    
    # Wait until a key is pressed
    cv2.waitKey(0)
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()