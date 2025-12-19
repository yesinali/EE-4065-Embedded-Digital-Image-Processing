/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
#include <stdlib.h>

/* Private defines -----------------------------------------------------------*/
#define IMG_WIDTH  128
#define IMG_HEIGHT 128
#define IMG_SIZE   (IMG_WIDTH * IMG_HEIGHT)

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;
uint8_t image_buf[IMG_SIZE];      // Q1 ve Q3 için 16KB
uint8_t color_buf[IMG_SIZE * 3];  // Q2 için 48KB
uint8_t temp_buf[IMG_SIZE];       // Morfoloji için geçici buffer
uint8_t header[2];                // [Mode, Unused]

/* Function Prototypes -------------------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
uint8_t compute_otsu(uint8_t *data, uint32_t size);
void morph_dilation(uint8_t *src, uint8_t *dst);
void morph_erosion(uint8_t *src, uint8_t *dst);

int main(void) {
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  while (1) {
    /* PC'den mod bilgisini bekle */
    if (HAL_UART_Receive(&huart2, header, 2, HAL_MAX_DELAY) == HAL_OK) {
        uint8_t mode = header[0];

        if (mode == 1) { // Q1: Grayscale Otsu
            if (HAL_UART_Receive(&huart2, image_buf, IMG_SIZE, HAL_MAX_DELAY) == HAL_OK) {
                uint8_t thr = compute_otsu(image_buf, IMG_SIZE);
                for(int i=0; i<IMG_SIZE; i++) image_buf[i] = (image_buf[i] > thr) ? 255 : 0;
                HAL_UART_Transmit(&huart2, image_buf, IMG_SIZE, HAL_MAX_DELAY);
            }
        }
        else if (mode == 2) { // Q2: Color Otsu (Channel-wise)
            if (HAL_UART_Receive(&huart2, color_buf, IMG_SIZE * 3, HAL_MAX_DELAY) == HAL_OK) {
                for (int c = 0; c < 3; c++) {
                    uint8_t *channel = &color_buf[c * IMG_SIZE];
                    uint8_t thr = compute_otsu(channel, IMG_SIZE);
                    for (int i = 0; i < IMG_SIZE; i++) channel[i] = (channel[i] > thr) ? 255 : 0;
                }
                HAL_UART_Transmit(&huart2, color_buf, IMG_SIZE * 3, HAL_MAX_DELAY);
            }
        }
        else if (mode >= 3 && mode <= 4) { // Q3: Morphological (Dilation/Erosion)
            if (HAL_UART_Receive(&huart2, image_buf, IMG_SIZE, HAL_MAX_DELAY) == HAL_OK) {
                if (mode == 3) morph_dilation(image_buf, temp_buf);
                else if (mode == 4) morph_erosion(image_buf, temp_buf);
                HAL_UART_Transmit(&huart2, temp_buf, IMG_SIZE, HAL_MAX_DELAY);
            }
        }
    }
  }
}

/* Otsu Method Implementation */
uint8_t compute_otsu(uint8_t *data, uint32_t size) {
    uint32_t hist[256] = {0};
    for (uint32_t i = 0; i < size; i++) hist[data[i]]++;
    float sum = 0, sumB = 0, varMax = 0;
    for (int i = 0; i < 256; i++) sum += (float)i * hist[i];
    int wB = 0, wF = 0; uint8_t threshold = 0;
    for (int i = 0; i < 256; i++) {
        wB += hist[i]; if (wB == 0) continue;
        wF = size - wB; if (wF == 0) break;
        sumB += (float)(i * hist[i]);
        float mB = sumB / (float)wB, mF = (sum - sumB) / (float)wF;
        float varBetween = (float)wB * (float)wF * (mB - mF) * (mB - mF);
        if (varBetween > varMax) { varMax = varBetween; threshold = (uint8_t)i; }
    }
    return threshold;
}

/* Dilation: Nesneyi genişletir */
void morph_dilation(uint8_t *src, uint8_t *dst) {
    for (int y = 1; y < IMG_HEIGHT-1; y++) {
        for (int x = 1; x < IMG_WIDTH-1; x++) {
            uint8_t res = 0;
            for (int ky = -1; ky <= 1; ky++)
                for (int kx = -1; kx <= 1; kx++)
                    if (src[(y+ky)*IMG_WIDTH + (x+kx)] == 255) res = 255;
            dst[y*IMG_WIDTH + x] = res;
        }
    }
}

/* Erosion: Nesneyi inceltir */
void morph_erosion(uint8_t *src, uint8_t *dst) {
    for (int y = 1; y < IMG_HEIGHT-1; y++) {
        for (int x = 1; x < IMG_WIDTH-1; x++) {
            uint8_t res = 255;
            for (int ky = -1; ky <= 1; ky++)
                for (int kx = -1; kx <= 1; kx++)
                    if (src[(y+ky)*IMG_WIDTH + (x+kx)] == 0) res = 0;
            dst[y*IMG_WIDTH + x] = res;
        }
    }
}

/* Hardware Configuration Functions */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

static void MX_USART2_UART_Init(void) {
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  HAL_UART_Init(&huart2);
}

static void MX_GPIO_Init(void) { __HAL_RCC_GPIOA_CLK_ENABLE(); }
