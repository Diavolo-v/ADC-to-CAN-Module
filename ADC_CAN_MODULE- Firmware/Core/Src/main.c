/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "adc.h"
#include "fdcan.h"
#include "gpio.h"


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//Vin = Vadc * (R1+R2)/R2
uint16_t converReadingsToDVoltage(uint32_t value){
	float digitalValue = 0;
	digitalValue = (value/4095.0f)* 3.3f * 5.7f;
	digitalValue *= 1000;
	return (uint16_t)digitalValue;

}

uint8_t getHighByte(uint16_t value){
	uint8_t byteHigh = 0;
	byteHigh = (value >> 8);
	return byteHigh;
}
uint8_t getLowByte(uint16_t value){
	uint8_t byteLow = 0;
	byteLow = (value & 0xFF);
	return byteLow;
}
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

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_FDCAN1_Init();
  /* USER CODE BEGIN 2 */
  FDCAN_FilterTypeDef filter = {0};
  filter.IdType = FDCAN_STANDARD_ID;
  filter.FilterIndex = 0;
  filter.FilterType = FDCAN_FILTER_MASK;
  filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  filter.FilterID1 = 0x000;
  filter.FilterID2 = 0x000;

  if(HAL_FDCAN_ConfigFilter(&hfdcan1, &filter) != HAL_OK){
	  Error_Handler();
  }
  HAL_StatusTypeDef status;
  status = HAL_FDCAN_Start(&hfdcan1);

  if(status != HAL_OK){
	  Error_Handler();
  }
  	uint32_t adc_raw1 = 0;
    uint32_t adc_raw2 = 0;
    int errorCounter = 0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  HAL_StatusTypeDef reading1 = HAL_ERROR;
	  HAL_StatusTypeDef reading2 = HAL_ERROR;
	  bool measurementsCorrect = false;
	  if(HAL_ADC_Start(&hadc1) == HAL_OK){
		  reading1 = HAL_ADC_PollForConversion(&hadc1, 10) ;
		  if(reading1 == HAL_OK){
			  adc_raw1 = HAL_ADC_GetValue(&hadc1);
		  }
		  reading2 = HAL_ADC_PollForConversion(&hadc1, 10);
		  if(reading2 == HAL_OK){
			  adc_raw2 = HAL_ADC_GetValue(&hadc1);
		  }
		  HAL_ADC_Stop(&hadc1);
		  if(reading1 == HAL_OK && reading2 == HAL_OK){
			  measurementsCorrect = true;
		  }
	  }

	  if(measurementsCorrect){
		  uint16_t adc_digital1 = converReadingsToDVoltage(adc_raw1);
		  uint16_t adc_digital2 = converReadingsToDVoltage(adc_raw2);


		  uint8_t byte_low1 = getLowByte(adc_digital1);
		  uint8_t byte_high1 = getHighByte(adc_digital1);

		  uint8_t byte_low2 = getLowByte(adc_digital2);
		  uint8_t byte_high2 = getHighByte(adc_digital2);

		  uint8_t txData[4];
		  	  txData[0] = byte_high1;
		  	  txData[1] = byte_low1;
		  	  txData[2] = byte_high2;
		  	  txData[3] = byte_low2;

		  	  FDCAN_TxHeaderTypeDef frame;
		  	  frame.Identifier = 0x123;
		  	  frame.IdType = FDCAN_STANDARD_ID;
		  	  frame.DataLength = FDCAN_DLC_BYTES_4;
		  	  frame.TxFrameType = FDCAN_DATA_FRAME;
		  	  frame.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
		  	  frame.BitRateSwitch = FDCAN_BRS_OFF;
		  	  frame.FDFormat = FDCAN_CLASSIC_CAN;
		  	  frame.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
		  	  frame.MessageMarker = 0;

		  	  if(HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1,&frame, txData) != HAL_OK){
		  		  //Error sending frame to CAN.
		  		  errorCounter++;

		  	  }
	  }


	  HAL_Delay(10);



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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
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
#ifdef USE_FULL_ASSERT
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
