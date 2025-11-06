# EE4065 – Embedded Digital Image Processing

This repository contains the source files and project setup for **EE4065 – Embedded Digital Image Processing**, implemented on **STM32** using **STM32CubeIDE**.

## Overview

The project demonstrates:
- Converting a grayscale image into a header file and displaying pixel values in MCU memory.  
- Applying several intensity transformations directly on the microcontroller.  
- Observing and verifying results through the **Memory Window** in STM32CubeIDE.

#### Giriş Görüntüsü
[Original Image] (original_image.jpg)

---

## Q1 – Grayscale Image Creation and Display (40 points)

**Task summary:**
1. Create a grayscale image of suitable size on your PC (e.g., 128×128 pixels, 8-bit grayscale).  
2. Convert the image to a C header file (array of pixel values).  
3. Add this header file to your STM32 project.  
4. Display selected image entries by observing memory locations in STM32CubeIDE.

**Files involved:**
- `image.h` – contains the grayscale image data and dimension definitions.  
- `main.c` – includes code to read and display sample pixel values from memory.  

**Observation:**  
Use the **Memory Window** under STM32CubeIDE to check the image data stored in memory.

---

## Q2 – Intensity Transformations (60 points)

Apply the following transformations to the same image and verify the results using the memory viewer.

### a) Negative Image
Compute the negative of each pixel value:
\[
s = 255 - r
\]

### b) Thresholding
Set pixel values above a certain threshold \( T \) to 255 and others to 0.

### c) Gamma Correction
Apply gamma correction for:
- \( \gamma = 3 \)
- \( \gamma = \frac{1}{3} \)

\[
s = 255 \times \left(\frac{r}{255}\right)^\gamma
\]

### d) Piecewise Linear Transformation
Use a piecewise linear function based on the same threshold \( T \) from part (b).  
Define two linear sections with different slopes to stretch or compress contrast in specific ranges.

---

## Verification

Use **STM32CubeIDE → Debug → Memory Window** to:
- View the original image array and the transformed versions.
- Compare memory values for different transformation outputs.

---

## Project Structure

Core/
├── Inc/
│ ├── image.h
│ └── main.h
├── Src/
│ ├── main.c
│ └── transformations.c

---

## Notes

- Use 8-bit unsigned integers (`uint8_t`) for pixel data.  
- Keep arrays `volatile` so values remain visible in the Memory Window.  
- Choose a small image to fit into MCU RAM (e.g., 128×128).  
- The math library (`math.h`) may be required for gamma correction.  
- Results and analysis should be added below.

---

## Results and Discussion

*(You can add screenshots, memory observations, or brief explanations here later.)*

---

**Author:**  
EE4065 Embedded Digital Image Processing – Project Report
