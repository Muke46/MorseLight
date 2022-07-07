
#define th 2500 //threshold between off and on
#define tunit 290 //unità di tempo (in millisecondi)

#include "main.h"

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;

UART_HandleTypeDef huart2;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC_Init(void);
uint32_t getInput(void);
char decode(char symb, int inx);

int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_ADC_Init();
  /* USER CODE BEGIN 2 */
  char str[] = "Started!\r\n";
  HAL_UART_Transmit(&huart2, str, strlen(str), 0xFFFF);

  //timers
  uint32_t intime1; //tracks the duration of a symbol
  uint32_t endtime1;
  uint32_t intime2; //tracks the duration of a pause

  int state=0; //state machine

  char symb=0; //stores a binary representation of the input
  int inx=0; //index to write symb
  char word[20]; //stores the decoded word
  int w_inx=0; //index to write word

  char msg[30]; //generic string for prints

  while (1)
  {
	  switch(state){
	  case 0: //IDLE State
		  while(getInput()>th); //wait for a character (stuck characters are not managed)
		  intime1 = HAL_GetTick(); //timer 1 start
		  state=1; //decode a symbol
		  break;

	  case 1: //Decoding symbol
		  while(getInput()<=th); //wait for a symbol to end
		  endtime1 = HAL_GetTick(); //timer 1 stop
		  intime2 = HAL_GetTick(); //timer 2 start

		  if((endtime1-intime1)<(2*tunit)){ //if the duration of the symbol is less than 2 units of time, it's a dot

			  //sprintf(msg, "Simbolo: . [Durata: %ld]\r\n", endtime1-intime1); //debug

			  sprintf(msg, ".", endtime1-intime1);  //prepare for serial print symbol
			  symb |= 1<<inx; //a dot is represented as a 1 in symb
		  }
		  else{ //if the duration of the symbol is less than 2 units of time, it's a line

			  //sprintf(msg, "Simbolo: - [Durata: %ld]\r\n", endtime1-intime1); //debug

			  sprintf(msg, "-", endtime1-intime1); //prepare for serial print symbol
		  }
		  HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 0xFFFF); //serial print symbol
		  inx++;

		  state=2; //Check if the symbols are finished or if there are others
		  break;

	  case 2: //Check if the symbols are finished or if there are others
		  while( (getInput()>th) && ( (HAL_GetTick()-intime2) < (2*tunit) ) ); //wait for another symbol or for character timeout
		  if (getInput()<=th){ //new symbol

			  //sprintf(msg, "Nuovo simbolo [inp:%ld] \r\n", getInput()); //debug
			  //HAL_UART_Transmit(&huart2, msg, strlen(msg), 0xFFFF);

			  intime1 = HAL_GetTick(); //reset timer 1

			  state=1; //decode a new symbol
			  break;
		  }
		  else { //character timeout

			  //sprintf(msg, "Character timeout [tim:%ld] \r\n", HAL_GetTick()-intime2); //debug
			  //HAL_UART_Transmit(&huart2, msg, strlen(msg), 0xFFFF);


			  char decoded = decode(symb, inx); //decode the character

			  sprintf(msg, "(%c) ",decoded);
			  HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 0xFFFF); //print the decoded character to serial

			  //reset symb and its index
			  symb=0;
			  inx=0;

			  //add the decoded character to the current word
			  word[w_inx]=decoded;
			  w_inx++;

			  state=3; //end of character -> check if there is a new one or if the word stops
			  break;
		  }
		  break;

	  case 3: //end of character -> check if there is a new one or if the word stops

		  while( (getInput()>th) && ((HAL_GetTick()-intime2)<(4*tunit)) ); //wait for another character or for word timeout

		  if (getInput()<=th){ //new character

			  //sprintf(msg, "New character \r\n"); //debug
			  //HAL_UART_Transmit(&huart2, msg, strlen(msg), 0xFFFF);

			  intime1=HAL_GetTick(); //reset timer 1
			  state=1; //Decode the new symbol
			  break;
		  }
		  else { //word timeout

			  //sprintf(msg, "Word timeout [tim:%ld] \r\n", HAL_GetTick()-intime2); //debug
			  //HAL_UART_Transmit(&huart2, msg, strlen(msg), 0xFFFF);

			  state=0; //word ended, go back to the IDLE state

			  word[w_inx]='\r'; //carriage return
			  word[w_inx+1]='\n'; //line feed
			  word[w_inx+2]='\0'; //ends the string
			  w_inx=0; //reset the word index

			  HAL_UART_Transmit(&huart2, (uint8_t*)word, strlen(word), 0xFFFF); //Print the decoded word to serial


			  //sprintf(msg, "\r\n", endtime1-intime1); //debug
			  //HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 0xFFFF);
			  break;
		  }
		  break;
	  }

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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI14;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI14CalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC_Init(void)
{

  /* USER CODE BEGIN ADC_Init 0 */

  /* USER CODE END ADC_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC_Init 1 */

  /* USER CODE END ADC_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc.Instance = ADC1;
  hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC_Init 2 */

  /* USER CODE END ADC_Init 2 */

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
  huart2.Init.BaudRate = 38400;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
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

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

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

}

/* USER CODE BEGIN 4 */
uint32_t getInput(void){
	HAL_ADC_Start(&hadc);
	HAL_ADC_PollForConversion(&hadc, HAL_MAX_DELAY);
	return HAL_ADC_GetValue(&hadc);
}

char decode(char symb, int inx){
	if(symb==0b00001 && inx==2) return 'A';       //.-
	else if (symb==0b01110 && inx==4) return 'B'; //-...
	else if (symb==0b01010 && inx==4) return 'C'; //-.-.
	else if (symb==0b00110 && inx==3) return 'D'; //-..
	else if (symb==0b00001 && inx==1) return 'E'; //.
	else if (symb==0b01011 && inx==4) return 'F'; //..-.
	else if (symb==0b00100 && inx==3) return 'G'; //--.
	else if (symb==0b01111 && inx==4) return 'H'; //....
	else if (symb==0b00011 && inx==2) return 'I'; //..
	else if (symb==0b00001 && inx==4) return 'J'; //.---
	else if (symb==0b00010 && inx==3) return 'K'; //-.-
	else if (symb==0b01101 && inx==4) return 'L'; //.-..
	else if (symb==0b00000 && inx==2) return 'M'; //--
	else if (symb==0b00010 && inx==2) return 'N'; //-.
	else if (symb==0b00000 && inx==3) return 'O'; //---
	else if (symb==0b01001 && inx==4) return 'P'; //.--.
	else if (symb==0b00100 && inx==4) return 'Q'; //--.-
	else if (symb==0b00101 && inx==3) return 'R'; //.-.
	else if (symb==0b00111 && inx==3) return 'S'; //...
	else if (symb==0b00000 && inx==1) return 'T'; //-
	else if (symb==0b00011 && inx==3) return 'U'; //..-
	else if (symb==0b00111 && inx==4) return 'V'; //...-
	else if (symb==0b00001 && inx==3) return 'W'; //.--
	else if (symb==0b00110 && inx==4) return 'X'; //-..-
	else if (symb==0b00010 && inx==4) return 'Y'; //-.--
	else if (symb==0b01100 && inx==4) return 'Z'; //--..
	else if (symb==0b00000 && inx==5) return '0'; //-----
	else if (symb==0b00001 && inx==5) return '1'; //.----
	else if (symb==0b00011 && inx==5) return '2'; //..---
	else if (symb==0b00111 && inx==5) return '3'; //...--
	else if (symb==0b01111 && inx==5) return '4'; //....-
	else if (symb==0b11111 && inx==5) return '5'; //.....
	else if (symb==0b11110 && inx==5) return '6'; //-....
	else if (symb==0b11100 && inx==5) return '7'; //--...
	else if (symb==0b11000 && inx==5) return '8'; //---..
	else if (symb==0b10000 && inx==5) return '9'; //----.
	else return '?';

}
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
