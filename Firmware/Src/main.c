/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include <stdlib.h>
uint8_t RX_Buff_dma[8];
uint8_t RX_Data_dma;
uint8_t TX_Buff[25] = "Thuy Kieu xinh dep";
uint8_t rxIndex;
uint16_t Tx_Flag = 0;
int motor_state = 0;
int mode_select;
int last_motor_state = 0;
float tkp = 0;
float tki = 0;
float tkd = 0;
float a;
float b;
float g;
float delta;
typedef struct {
	uint8_t rawData[21];
	struct {
		char startStop[1];
		char setpoint[5];
		char kp[5];
		char ki[5];
		char kd[5];
	} data;
} rawData_t;
typedef struct {
	struct {
		float kp;
		float ki;
		float kd;
		float setpoint;
	} PID;
} data_t;

uint8_t rcv_flag = 0;

rawData_t rawData;
data_t data;
#define RX_BUFF_SIZE 22
uint8_t rx_buff[RX_BUFF_SIZE];
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart3_tx;
DMA_HandleTypeDef hdma_usart3_rx;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
volatile short encoder_cnt = 0, encoder_cnt_pre = 0;
volatile short rate = 0, pulse;
//volatile short a = 0, b = 0;
uint16_t var, pwm, pwm_last = 0;
float speed; //tốc độ (rpm)
//const uint8_t * buffer[8];
char buffer[8]; //  ?ịnh nghĩa bộ đệm đủ lớn để lưu trữ chuỗi số
char buffer_pos[8];
float speed_ref;
float setpoint = 0;
float kp;
float ki;
float kd;
float e_current, E1 = 0, E2 = 0;
float a, b, g;
char buffer_speed_ref[6];
int64_t position = 0;
int16_t position_ref;
int16_t position_deg = 0;
uint32_t last_counter = 0;
int16_t velocity = 0;

//uint32_t timer_counter;
uint16_t PID(float T);
uint16_t PID_POS(float T, float e_pos);
//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
//	timer_counter = __HAL_TIM_GET_COUNTER(&htim2);
//}

uint32_t counter = 0;

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
	counter = __HAL_TIM_GET_COUNTER(htim);
}
void floatToStr(float value, char *buffer, int bufferSize) {
	if (value >= 100)
		snprintf(buffer, bufferSize, "%.2f", value);

	else if (value >= 10) //10.1 -> 99.99
		snprintf(buffer, bufferSize, "%.3f", value);

	else
		snprintf(buffer, bufferSize, "%.4f", value);

}
float stringToFloat(char *str) {
	return atof(str);
}

void DMA_RxComplete_Callback() {
	uint16_t i;
	__HAL_DMA_CLEAR_FLAG(&hdma_usart3_rx, DMA_FLAG_TC3);
	/* Copy received data to rawData buffer */
	for (i = 0; i < RX_BUFF_SIZE; i++) {
		rawData.rawData[i] = rx_buff[i];
	}
	char char_value[5];

	motor_state = rx_buff[0] - 48;
	mode_select = rx_buff[21] - 48;
	for (int i = 1, j = 0; i <= 5; i++) {
		char_value[j++] = rx_buff[i];
	}
	if (mode_select == 0)
		speed_ref = atoff(char_value);
	else
		position_ref = atoff(char_value);
	for (int i = 6, j = 0; i <= 10; i++) {
		char_value[j++] = rx_buff[i];
	}
	kp = atoff(char_value);
	for (int i = 11, j = 0; i <= 15; i++) {
		char_value[j++] = rx_buff[i];
	}
	ki = atoff(char_value);
	for (int i = 16, j = 0; i <= 20; i++) {
		char_value[j++] = rx_buff[i];
	}
	kd = atoff(char_value);
//     speed_ref = (rx_buff[3] - 48)*100 + (rx_buff[4] - 48)*10 +(rx_buff[5]);
//      kp = rx_buff[10] - 48;
//      ki = rx_buff[15] - 48;
//      kd = (rx_buff[18] - 48) + (rx_buff);//ê có hàm string to float mà đâu
//      motor_state = rx_buff[0] - 48;// chưa hiểu hàm đó lắm, hàm đó t h�?i con chat ngu, để fix thử hàm đó xem
//      motor_state = rx_buff[0] - 48;
//      motor_state = rx_buff[0] - 48;

	/* Set flag to indicate that a frame has been received */
	rcv_flag = 1;

	/* Enable DMA1 to receive next frame */
	HAL_UART_Receive_DMA(&huart3, rx_buff, sizeof(rx_buff));
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
	/* USER CODE BEGIN 1 */
	float T = 50;
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
	MX_DMA_Init();
	MX_TIM2_Init();
	MX_TIM3_Init();
	MX_USART3_UART_Init();
	/* USER CODE BEGIN 2 */
	HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_1 | TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
	HAL_UART_Receive_DMA(&huart3, rx_buff, sizeof(rx_buff));
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		DMA_RxComplete_Callback();
		encoder_cnt = __HAL_TIM_GET_COUNTER(&htim2);
		rate = encoder_cnt - encoder_cnt_pre;
		encoder_cnt_pre = encoder_cnt;
		speed = rate / 1320.0 / T * 60 * 1000;
		uint32_t temp_counter = __HAL_TIM_GET_COUNTER(&htim2);
		if (temp_counter == last_counter)
			velocity = 0;
		else if (temp_counter > last_counter)
			if (__HAL_TIM_IS_TIM_COUNTING_DOWN(&htim2)) {
				velocity = -last_counter
						- (__HAL_TIM_GET_AUTORELOAD(&htim2) - temp_counter);

			} else {
				velocity = temp_counter - last_counter;
			}
		else {
			if (__HAL_TIM_IS_TIM_COUNTING_DOWN(&htim2)) {
				velocity = temp_counter - last_counter;
			} else {
				velocity = temp_counter
						+ (__HAL_TIM_GET_AUTORELOAD(&htim2) - last_counter);
			}
		}
		position += velocity;
		position_deg = position/1320.0*360;
		last_counter = temp_counter;

		if (mode_select == 0) {
			floatToStr(speed, buffer, 8);
			buffer[6] = '\r';
			buffer[7] = '\n';
			HAL_UART_Transmit_DMA(&huart3, buffer, sizeof(buffer));
			if (motor_state == 1) {
				//HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_8);
				if (last_motor_state == 0) {
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, 1); // thư�?ng lệnh này chỉ cho thay đổi 1 lần chứ ko cho chạy lien tuc
					last_motor_state = 1;
				}

				__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, PID(T));
				__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
			}
			if (motor_state == 0 && last_motor_state == 1) {

				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, 0);
				last_motor_state = 0;
				position = 0;
				pwm_last = 0;
				E1 = 0;
				E2 = 0;
			}
			HAL_Delay(T);
		} else {

			floatToStr(position_deg, buffer_pos, 8);
			buffer_pos[6] = '\r';
			buffer_pos[7] = '\n';
			HAL_UART_Transmit_DMA(&huart3, buffer_pos, sizeof(buffer_pos));
			if (motor_state == 1) {
				//HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_8);
				if (last_motor_state == 0) {
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, 1); // thư�?ng lệnh này chỉ cho thay đổi 1 lần chứ ko cho chạy lien tuc
					last_motor_state = 1;
				}
				//if ()
				float e_pos = position_ref - position_deg;
				if (e_pos >= 0) {
					__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1,
							PID_POS(T, e_pos));
					__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
				} else {
					__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
					__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2,
							PID_POS(T, -e_pos));
				}

			}
			if (motor_state == 0 && last_motor_state == 1) {

				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, 0);
				last_motor_state = 0;
				position = 0;
				pwm_last = 0;
				E1 = 0;
				E2 = 0;
			}
			HAL_Delay(T);

		}
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void) {

	/* USER CODE BEGIN TIM2_Init 0 */

	/* USER CODE END TIM2_Init 0 */

	TIM_Encoder_InitTypeDef sConfig = { 0 };
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };

	/* USER CODE BEGIN TIM2_Init 1 */
	//HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
	/* USER CODE END TIM2_Init 1 */
	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 0;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 65535;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
	sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
	sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
	sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
	sConfig.IC1Filter = 0;
	sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
	sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
	sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
	sConfig.IC2Filter = 0;
	if (HAL_TIM_Encoder_Init(&htim2, &sConfig) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM2_Init 2 */

	/* USER CODE END TIM2_Init 2 */

}

/**
 * @brief TIM3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM3_Init(void) {

	/* USER CODE BEGIN TIM3_Init 0 */

	/* USER CODE END TIM3_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };
	TIM_OC_InitTypeDef sConfigOC = { 0 };

	/* USER CODE BEGIN TIM3_Init 1 */

	/* USER CODE END TIM3_Init 1 */
	htim3.Instance = TIM3;
	htim3.Init.Prescaler = 79;
	htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim3.Init.Period = 9999;
	htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim3) != HAL_OK) {
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK) {
		Error_Handler();
	}
	if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 0;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1)
			!= HAL_OK) {
		Error_Handler();
	}
	if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM3_Init 2 */

	/* USER CODE END TIM3_Init 2 */
	HAL_TIM_MspPostInit(&htim3);

}

/**
 * @brief USART3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART3_UART_Init(void) {

	/* USER CODE BEGIN USART3_Init 0 */

	/* USER CODE END USART3_Init 0 */

	/* USER CODE BEGIN USART3_Init 1 */

	/* USER CODE END USART3_Init 1 */
	huart3.Instance = USART3;
	huart3.Init.BaudRate = 9600;
	huart3.Init.WordLength = UART_WORDLENGTH_8B;
	huart3.Init.StopBits = UART_STOPBITS_1;
	huart3.Init.Parity = UART_PARITY_NONE;
	huart3.Init.Mode = UART_MODE_TX_RX;
	huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart3.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart3) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART3_Init 2 */

	/* USER CODE END USART3_Init 2 */

}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void) {

	/* DMA controller clock enable */
	__HAL_RCC_DMA1_CLK_ENABLE();

	/* DMA interrupt init */
	/* DMA1_Channel2_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
	/* DMA1_Channel3_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* USER CODE BEGIN MX_GPIO_Init_1 */
	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);

	/*Configure GPIO pin : B1_Pin */
	GPIO_InitStruct.Pin = B1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : LD2_Pin */
	GPIO_InitStruct.Pin = LD2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : PB8 */
	GPIO_InitStruct.Pin = GPIO_PIN_8;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

	/* USER CODE BEGIN MX_GPIO_Init_2 */
	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
uint16_t PID(float T) {		//này ko ảnh hưởng

	e_current = speed_ref - speed;	//dât chưa có pid, no deo cap nhat dc dko,

//	a = 2 * kp + ki + 2 * kd;
//	b = ki - 4 * kd - 2 * kp;
//	g = 2 * kd;		// do này nè

//	a = 2*0.3*kp + ki*0.2*0.2 + 2*kd; // ct lỏ
//	b = ki*0.15 - 4*kd - 2*0.15*kp;
//	g = 2*kd;
	a = 2.0 * T / 1000.0 * kp + ki * T * T / 1000.0 / 1000 + 2.0 * kd;
	b = T / 1000.0 * T / 1000 * ki - 4.0 * kd - 2.0 * T / 1000.0 * kp;
	g = 2.0 * kd;
	delta = 2.0 * T / 1000.0;

//	a = 2 * T * kp + ki * T * T + 2 * kd;
//	b = T * T * ki - 4 * kd - 2 * T * kp;
//	g = 2 * kd;
//	a = 2 * T/1000 * kp + ki * T * T/1000/1000 + 2 * kd;
//	b = T/1000 * T/1000 * ki - 4 * kd - 2 * T/1000 * kp;
//	g = 2 * kd;
	pwm = (a * e_current + b * E1 + g * E2 + delta * pwm_last) / delta;
//	pwm = (a * e_current + b * E1 + g * E2 + 0.2 * pwm_last) / (2*0.1);//ct lỏ
	if (pwm - pwm_last < 50 && pwm - pwm_last > -50) {
		pwm_last = pwm;
		return pwm;
	}

	//pwm = (a*e_current + b*E1 + g*E2 + 2*pwm_last)/2;

	//pwm = pwm_last + kp*(e_current - E1) + ki*T/2/1000*(e_current - E1)+ kd/T/1000*(e_current - 2*E1 + E2);

	pwm_last = pwm;
	//pwm = pwm_last + kp*(e_current - E1) + ki*T/2*(e_current + E1) + kd/T*(e_current - 2*E1 + E2);
	E2 = E1;
	E1 = e_current;

	if (pwm > 10000)
		pwm = 10000;
	else if (pwm < 0)
		pwm = 0;
	return pwm;
}

uint16_t PID_POS(float T, float e_pos) {
	//e_current = position_ref - position;//dât chưa có pid, no deo cap nhat dc dko,
	e_current = e_pos;
	a = 2.0 * T / 1000.0 * kp + ki * T * T / 1000.0 / 1000 + 2.0 * kd;
	b = T / 1000.0 * T / 1000 * ki - 4.0 * kd - 2.0 * T / 1000.0 * kp;
	g = 2.0 * kd;
	delta = 2.0 * T / 1000.0;

	pwm = (a * e_current + b * E1 + g * E2 + delta * pwm_last) / delta;
	//	pwm = (a * e_current + b * E1 + g * E2 + 0.2 * pwm_last) / (2*0.1);//ct lỏ
	if (pwm - pwm_last < 25 && pwm - pwm_last > -25) {
		pwm_last = pwm;
		return pwm;
	}
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
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
