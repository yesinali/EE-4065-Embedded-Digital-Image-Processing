/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <image_to_process.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define NUM_GRAY_LEVELS 256
#define MEDIAN_KERNEL_SIZE 3
#define WINDOW_SIZE (MEDIAN_KERNEL_SIZE * MEDIAN_KERNEL_SIZE) // 9
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;
uint32_t histogram[NUM_GRAY_LEVELS];
float cdf[NUM_GRAY_LEVELS];
uint8_t equalized_image[IMAGE_SIZE];
uint8_t output_image_med[IMAGE_SIZE] = {0};
uint8_t output_image_hp[IMAGE_SIZE] = {0};
uint8_t output_image_lp[IMAGE_SIZE] = {0};

const float low_pass_kernel_3x3[9] = {
    1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f,
    1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f,
    1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f
};


const float high_pass_kernel_3x3[9] = {
    -1.0f, -1.0f, -1.0f,
    -1.0f,  8.0f, -1.0f,
    -1.0f, -1.0f, -1.0f
};


/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void apply_2d_convolution(const uint8_t *input_image, uint8_t *output_image,
                          int height, int width, const float *kernel, int kernel_size) {

    int center = kernel_size / 2;
    int k_size = kernel_size;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float sum = 0.0f;


            for (int j = 0; j < k_size; j++) {
                for (int i = 0; i < k_size; i++) {


                    int image_y = y + j - center;
                    int image_x = x + i - center;


                    if (image_y < 0) image_y = 0;
                    if (image_y >= height) image_y = height - 1;
                    if (image_x < 0) image_x = 0;
                    if (image_x >= width) image_x = width - 1;


                    int input_index = image_y * width + image_x;
                    int kernel_index = j * k_size + i;


                    sum += (float)input_image[input_index] * kernel[kernel_index];
                }
            }


            int result = (int)(sum + 0.5f);

            if (result < 0) result = 0;
            if (result > 255) result = 255;

            output_image[y * width + x] = (uint8_t)result;
        }
    }
}
void sort_window(uint8_t *window, int size) {
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - 1 - i; j++) {
            if (window[j] > window[j + 1]) {
                // Swap işlemi (yer değiştirme)
                uint8_t temp = window[j];
                window[j] = window[j + 1];
                window[j + 1] = temp;
            }
        }
    }
}
void apply_median_filtering(const uint8_t *input_image, uint8_t *output_image,
                            int height, int width, int kernel_size) {

    int center = kernel_size / 2;
    uint8_t window[WINDOW_SIZE];

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int window_idx = 0;


            for (int j = 0; j < kernel_size; j++) {
                for (int i = 0; i < kernel_size; i++) {

                    int image_y = y + j - center;
                    int image_x = x + i - center;


                    if (image_y < 0) image_y = 0;
                    if (image_y >= height) image_y = height - 1;
                    if (image_x < 0) image_x = 0;
                    if (image_x >= width) image_x = width - 1;


                    window[window_idx++] = input_image[image_y * width + image_x];
                }
            }


            sort_window(window, WINDOW_SIZE);


            output_image[y * width + x] = window[WINDOW_SIZE / 2];
        }
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	for (int i = 0; i < 256; i++) {
	        histogram[i] = 0;
	    }
	for(int i=0; i<IMAGE_SIZE; i++){
		histogram[image[i]]++;
	}
	for(int i=0; i<NUM_GRAY_LEVELS; i++){
		cdf[i]=0;
	}
	cdf[0] = (float)histogram[0] / IMAGE_SIZE;
	for(int i=1; i<NUM_GRAY_LEVELS;i++){

	    cdf[i] = cdf[i-1] + ((float)histogram[i] / (float)IMAGE_SIZE);
	}

	int i = 0;
	while (i < NUM_GRAY_LEVELS && cdf[i] <= 0.0f) {
	    i++;
	}

	float cdf_min = 0.0f;
	if (i < NUM_GRAY_LEVELS) {

	    cdf_min = cdf[i];
	}

	float normalization_factor = 1.0f / (1.0f - cdf_min);

	for (int j = 0; j < IMAGE_SIZE; j++) {
	    uint8_t r_k = image[j];
	    float cdf_r_k = cdf[r_k];

	    float s_k_float;


	    if (cdf_r_k == cdf_min) {
	        s_k_float = 0.0f;
	    } else {

	        s_k_float = (cdf_r_k - cdf_min) * normalization_factor * 255.0f;
	    }


	    int result = (int)(s_k_float + 0.5f);
	    if (result > 255) result = 255;
	    if (result < 0) result = 0;


	    equalized_image[j] = (uint8_t)result;
	}

	uint32_t equalized_histogram[NUM_GRAY_LEVELS] = {0};
	for(int j=0; j<IMAGE_SIZE; j++){

	    equalized_histogram[equalized_image[j]]++;
	    apply_2d_convolution(equalized_image, output_image_lp,IMAGE_HEIGHT, IMAGE_WIDTH,low_pass_kernel_3x3, 3);
	    apply_2d_convolution(equalized_image, output_image_hp,IMAGE_HEIGHT, IMAGE_WIDTH,high_pass_kernel_3x3, 3);
	    apply_median_filtering(equalized_image, output_image_med, IMAGE_HEIGHT, IMAGE_WIDTH,3);
	}

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
