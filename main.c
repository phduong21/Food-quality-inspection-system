/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "stdio.h"
#include "DHT.h"
#include "math.h"
#include "string.h"
#include "i2c-lcd.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

uint32_t value[2];       // to store the adc value

volatile float result_NH3;
volatile float result_H2S;
volatile float Vo_NH3;
float Temperature, Humidity;
float adc_new_NH3, Rs_NH3, R02_NH3;
float Rs_final_NH3 = 0;
float ppm_NH3;

DHT_DataTypedef DHT22_Data;

float adc_new_H2S, Rs_H2S, R02_H2S, Vo_H2S;
float Rs_final_H2S = 0;
float ppm_H2S;

// float R0 = 17392;
//float R0 = 7828;
//float R1 = 8000;
//float R0 = 5000;
//float R1 = 8411;
float R1 = 9200;
float R0 = 7000;

char ppmValue1[20], ppmValue2[20], ppmValue3[20], ppmValue4[20];


volatile float adc_NH3, adc_H2S;

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

float calculate_NH3(float Temperature, float Humidity){

   float Rh33_NH3,Rh85_NH3,cal_NH3;

   Rh33_NH3=0.000367*(Temperature*Temperature) - 0.0267241*Temperature + 1.3912447;
   Rh85_NH3=0.000309*(Temperature*Temperature) - 0.023062*Temperature  + 1.255687;

   cal_NH3=(Rh33_NH3-Rh85_NH3)*(85-Humidity)/(85-33)+Rh85_NH3;

   return cal_NH3;
}

float calculate_H2S(float Temperature, float Humidity){

   float Rh33_H2S,Rh85_H2S,cal_H2S;

   Rh33_H2S=0.000361*(Temperature*Temperature) - 0.0264119*Temperature  + 1.3839120;
   Rh85_H2S=0.000306*(Temperature*Temperature) - 0.0231550*Temperature  + 1.2549732;

   cal_H2S=(Rh33_H2S-Rh85_H2S)*(85-Humidity)/(85-33)+Rh85_H2S;

   return cal_H2S;
}

void read_TempAndHumidity(){

    DHT_GetData(&DHT22_Data);
    Temperature = (DHT22_Data.Temperature)/10;
    Humidity    = (DHT22_Data.Humidity)/10;

    result_NH3 = calculate_NH3(Temperature,Humidity);
    result_H2S = calculate_H2S(Temperature,Humidity);

    HAL_Delay(1000);
//    HAL_ADC_Start_DMA(&hadc1, value, 2);      // Start the ADC in DMA mode
}

void avg_adc_NH3(){
	for(int i=0;i<100;i++){
		adc_NH3 += value[0];
	}
	adc_NH3 = adc_NH3/100.0;
}

void avg_adc_H2S(){
	for(int j=0;j<100;j++){
		adc_H2S += value[1];
	}
	adc_H2S = adc_H2S/100.0;
}

float standard_Calibration_NH3(){
   float RL_NH3=1000;
	avg_adc_NH3();

   Vo_NH3 =(float)(3.3*adc_NH3/4095.0);           // output voltage
   Rs_NH3 = ((5-Vo_NH3)*RL_NH3)/Vo_NH3;            // do Rs
   //Rs_NH3 = 5000/Vo_NH3 - 1000;

   R02_NH3=Rs_NH3/calculate_NH3(Temperature,Humidity);           // noi suy R0 theo nhiet do do am do dc
   Rs_final_NH3=R02_NH3*calculate_NH3(20,65);                        // tinh Rs tai dieu kien xac lap
   return Rs_final_NH3;                                   // standard 20 degree, 65% (Rs)
}

float standard_Calibration_H2S(){
   float RL_H2S=1000;
   avg_adc_H2S();
   //Vo_H2S =(float)(3.0*value[1]*1000.0/4095);           // output voltage
   Vo_H2S =(float)(3.3*adc_H2S/4095);
   Rs_H2S = ((5-Vo_H2S)*RL_H2S)/Vo_H2S;            // do Rs
   //Rs_H2S = 5000/Vo_H2S - 1000;

   R02_H2S=Rs_H2S/calculate_H2S(Temperature,Humidity);           // noi suy R0 theo nhiet do do am do dc
   Rs_final_H2S=R02_H2S*calculate_H2S(20,65);                        // tinh Rs tai dieu kien xac lap
   return Rs_final_H2S;                                   // standard 20 degree, 65% (Rs)
}


float final_Result_NH3(float x, float y){

   float a = (0.82995-log10(x/y))/0.41497;
   return pow(10,a);
}

float final_Result_H2S(float x, float y){

   float b = (0.41526-log10(x/y))/0.26911;
   return pow(10,b);
}




void transmit_UART_NH3(){

   float nh3;
   char nh3array[15] = {0};

   nh3 = ppm_NH3;

   sprintf(nh3array, "%.2f\r\n", nh3);
   HAL_UART_Transmit(&huart2, (uint8_t*)nh3array,strlen(nh3array), 1000);

   HAL_UART_Transmit(&huart2, (uint8_t*)"#", 1, 1000);
//   HAL_Delay(5000);
}

void transmit_UART_H2S(){

   float h2s;
   char h2sarray[15] = {0};

   h2s = ppm_H2S;

   sprintf(h2sarray, "%.2f\r\n", h2s);
   HAL_UART_Transmit(&huart2, (uint8_t*)h2sarray,strlen(h2sarray), 1000);

   HAL_UART_Transmit(&huart2, (uint8_t*)"$", 1, 1000);

//   HAL_Delay(5000);
}

void transmit_UART_Temp(){
	float temp;
	char temparray[15] = {0};

	temp = Temperature;

	sprintf(temparray, "%.1f\r\n", temp);
	HAL_UART_Transmit(&huart2, (uint8_t*)temparray,strlen(temparray), 5000);

	HAL_UART_Transmit(&huart2, (uint8_t*)"!", 1, 1000);
}


void transmit_UART_Humid(){
	float hum;
	char humidarray[15] = {0};

	hum = Humidity;

	sprintf(humidarray, "%.1f\r\n", hum);
	HAL_UART_Transmit(&huart2, (uint8_t*)humidarray,strlen(humidarray), 1000);

	HAL_UART_Transmit(&huart2, (uint8_t*)"@", 1, 1000);

}

void display(){

    lcd_goto_XY(1,0);
    lcd_send_string("T: ");
    sprintf(ppmValue3, "%.0lf  ", Temperature);
    lcd_send_string(ppmValue3);

    lcd_goto_XY(1,6);
    lcd_send_string("NH3: ");
    sprintf(ppmValue1, "%.2f", ppm_NH3);
    lcd_send_string(ppmValue1);

    lcd_goto_XY(2,0);
    lcd_send_string("H: ");
    sprintf(ppmValue4, "%.0lf  ", Humidity);
    lcd_send_string(ppmValue4);

    lcd_goto_XY(2,6);
    lcd_send_string("H2S: ");
    sprintf(ppmValue2, "%.3f", ppm_H2S);
    lcd_send_string(ppmValue2);

}
//void delay(uint32_t delay){
//	__HAL_TIM_SET_COUNTER(&htim10, 0);
//	while (__HAL_TIM_GET_COUNTER(&htim10)< delay);
//}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  lcd_init();
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
//  HAL_TIM_Base_Start(&htim10);

  HAL_ADC_Start_DMA(&hadc1, value, 2);      // Start the ADC in DMA mode


//   HAL_UART_Transmit(&huart2, (uint8_t*)"H", 1, 1000);
//   HAL_UART_Transmit(&huart2, (uint8_t*)"\n", 1, 1000);
//   __NOP();
//
//   HAL_UART_Transmit(&huart2, (uint8_t*)"PHAM HOANG DUONG", 16, 1000);
//   HAL_UART_Transmit(&huart2, (uint8_t*)"\n", 1, 1000);
//   __NOP();
//
//   uint16_t intnumber = 1000;
//   char intarray[5] ={0};
//   sprintf(intarray, "%d", intnumber);
//   HAL_UART_Transmit(&huart2, (uint8_t*)intarray, 4 , 1000);
//   HAL_UART_Transmit(&huart2, (uint8_t*)"\n", 1, 1000);
//   __NOP();
//
//   float floatnumber = 15.3;
//   char floatarray[5] = {0};
//   sprintf(floatarray, "%.1f", floatnumber);
//   HAL_UART_Transmit(&huart2, (uint8_t*)floatarray, 4, 1000);
//   HAL_UART_Transmit(&huart2, (uint8_t*)"\n", 1, 1000);
//   __NOP();


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	 read_TempAndHumidity();

	 transmit_UART_Temp();
	 transmit_UART_Humid();

	 standard_Calibration_NH3();
	 ppm_NH3 = final_Result_NH3(standard_Calibration_NH3(), R0);

	 standard_Calibration_H2S();
	 ppm_H2S = final_Result_H2S(standard_Calibration_H2S(), R1);

	 if(ppm_NH3>300){
		ppm_NH3 = 300;
	 }

	 if(ppm_H2S>200){
		ppm_H2S = 200;
	 }

	 display();
	 transmit_UART_NH3();
//	 delay(5000000);   // 500 ms
//	 HAL_Delay(1000);
	 transmit_UART_H2S();

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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */
  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 2;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = 2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

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
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX;
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
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
