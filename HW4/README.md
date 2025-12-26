# EE4065 – Embedded Digital Image Processing

This repository contains the source code and implementation report for **EE4065 – Embedded Digital Image Processing Homework 4**, focused on handwritten digit recognition using the **MNIST dataset**. The project implements two distinct machine learning models—a single-neuron binary classifier and a multilayer neural network—using **Hu Moments** as the primary feature set.

---

## Overview

The project demonstrates:
* Loading and parsing the **MNIST dataset** from binary `.ubyte` files.
* Feature extraction of **7 Hu Moments** to characterize digit shapes.
* Implementing a **Single Neuron Classifier** for binary "zero" vs. "not zero" recognition.
* Implementing a **Multilayer Neural Network (MLP)** for full 10-class (0–9) digit recognition.
* Performance evaluation using **Confusion Matrices** to analyze classification errors.

---

## Q1 – Single Neuron Classifier (Section 10.9)

**Task Summary:**
1. Extract 7 Hu Moments for each image in the MNIST dataset.
2. [cite_start]Define binary classes: **"zero"** (Class 0) and **"not zero"** (Class 1)[cite: 8].
3. Implement a single-layer model with 1 neuron and a **Sigmoid** activation function.
4. Train using **Binary Cross-entropy** loss and the **Adam optimizer**.

**Implementation & Methodology:**
* **Preprocessing:** Hu Moments are normalized by subtracting the mean and dividing by the standard deviation of the training set.
* **Imbalance Handling:** A class weight of `{0: 8, 1: 1}` was applied to prioritize the "zero" class due to the smaller sample size compared to "not zeros".

**Observations from Results (1.png):**
* **Successes:** The model correctly identified 946 "zero" digits.
* **Errors:** There were 1521 False Positives where "not zero" digits were incorrectly classified as "zero".
* **Conclusion:** A single neuron can capture basic geometric differences but lacks the depth to isolate the "zero" shape from all other nine digits perfectly using only 7 features.

---

## Q2 – Multilayer Neural Network (Section 11.8)

**Task Summary:**
1. Use 7 Hu Moments as inputs for a full 10-class (0–9) recognition task.
2. Implement a three-layer neural network architecture:
    * **Hidden Layer 1:** 100 neurons with **ReLU** activation.
    * **Hidden Layer 2:** 100 neurons with **ReLU** activation.
    * **Output Layer:** 10 neurons with **Softmax** activation representing classes 0–9.
3. Train using **Sparse Categorical Cross-entropy** loss.

**Observations from Results (2.png):**
* **Accuracy:** The MLP provides much higher classification detail. Digit "1" is the most successfully recognized class with 1096 correct predictions.
* **Confusion:** Significant overlap exists between similar shapes; for example, 262 instances of "True 2" were misclassified as "Predicted 4".
* **Conclusion:** Deeper layers allow for better class separation, but 7 Hu Moments remain a bottleneck for distinguishing between complex overlaps in handwriting.

---

## Comparison and Analysis

| Feature | Single Neuron (Q1) | Neural Network (Q2) |
| :--- | :--- | :--- |
| **Classification Type** | Binary ("0" vs "Not-0") | Multi-class (0-9) |
| **Activation Functions** | Sigmoid | ReLU & Softmax |
| **Loss Function** | Binary Cross-entropy | Sparse Categorical Cross-entropy |
| **Input Features** | 7 Hu Moments | 7 Hu Moments |

---

## Project Structure

```text
.
├── mnist.py               # Utility for loading MNIST binary data
├── Listing10_19UPDATED.py # Script for the single-neuron binary classifier
├── Listing11_6.py         # Script for the multi-class MLP
├── MNIST-dataset/         # Local folder containing raw idx-ubyte files
└── outputs/
    ├── 1.png              # Confusion Matrix: Single Neuron Classifier
    └── 2.png              # Confusion Matrix: Neural Network