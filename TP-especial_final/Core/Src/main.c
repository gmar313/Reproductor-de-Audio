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
#include "cmsis_os.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "fonts.h"
#include "ssd1306.h"
#include "fft.h"
#include "FreeRTOS.h"
#include "biquad.h"
#include "math.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define BUFFERSIZE 128

#define BARRA_ANCHO 4
#define BARRA_MAX_ALTURA 64
#define CAIDA_PICO 1

#define SIZE_FFT 64
#define FFT_COL 32

#define velocidad_fall 4

typedef enum {
	NING,
	UP,
	DWN
}botones_e;

typedef enum{
	INICIO,
	CAN,
	REP,
	SELEC

}mensajes_l;

mensajes_l men=INICIO;
botones_e boton=NING;

char archivos[5][11];

uint16_t idx;

uint8_t bufferDn[BUFFERSIZE],bufferUp[BUFFERSIZE];

uint8_t *puntero_escritura = bufferUp;
uint8_t *puntero_lectura = bufferDn;

cmpx Y[SIZE_FFT]; //buffer para hacer la fft

uint8_t barras[FFT_COL], barras_peak[FFT_COL],y[FFT_COL];
uint8_t x[FFT_COL]={4,8,12,16,20,24,28,32,36,40,44,48,52,46,60,64,
		68,72,76,80,84,88,92,96,100,104,108,112,116,120,124,128};

uint8_t flag_ec,ready, clse,sel; // flags generales

uint32_t vadc;

float ganancia_low = 1.4;
float ganancia_high = 1.4;


/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define SD_SPI_HANDLE hspi2

#define FCLK_SLOW() { MODIFY_REG(SD_SPI_HANDLE.Instance->CR1, SPI_BAUDRATEPRESCALER_256, SPI_BAUDRATEPRESCALER_32); }
/* Set SCLK = slow, approx 280 KBits/s*/

#define FCLK_FAST() { MODIFY_REG(SD_SPI_HANDLE.Instance->CR1, SPI_BAUDRATEPRESCALER_256, SPI_BAUDRATEPRESCALER_2); }
/* Set SCLK = fast, approx 4.5 MBits/s */



/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 64 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for Lectura_botones */
osThreadId_t Lectura_botonesHandle;
const osThreadAttr_t Lectura_botones_attributes = {
  .name = "Lectura_botones",
  .stack_size = 74 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for GRAFICA */
osThreadId_t GRAFICAHandle;
const osThreadAttr_t GRAFICA_attributes = {
  .name = "GRAFICA",
  .stack_size = 240 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
};
/* Definitions for SD */
osThreadId_t SDHandle;
const osThreadAttr_t SD_attributes = {
  .name = "SD",
  .stack_size = 845 * 4,
  .priority = (osPriority_t) osPriorityNormal2,
};
/* Definitions for Sem_esc */
osSemaphoreId_t Sem_escHandle;
const osSemaphoreAttr_t Sem_esc_attributes = {
  .name = "Sem_esc"
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI2_Init(void);
void StartDefaultTask(void *argument);
void leerboton(void *argument);
void pantalla(void *argument);
void lectura_sd(void *argument);

/* USER CODE BEGIN PFP */
void ADC1_in_NoHAL(void) ;
void callback_in(int tag) {
	switch (tag) {
	case TAG_IDLE: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET); break;
	case TAG_Pantalla: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET); break;
	case TAG_Boton: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET); break;
	case TAG_SD: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); break;
	}
}
void callback_out(int tag) {
	switch (tag) {
	case TAG_IDLE: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); break;
	case TAG_Pantalla: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET); break;
	case TAG_Boton: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET); break;
	case TAG_SD: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); break;
	}
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_FATFS_Init();
  MX_I2C1_Init();
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */

  SSD1306_Init();

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of Sem_esc */
  Sem_escHandle = osSemaphoreNew(1, 1, &Sem_esc_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */

  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of Lectura_botones */
  Lectura_botonesHandle = osThreadNew(leerboton, NULL, &Lectura_botones_attributes);

  /* creation of GRAFICA */
  GRAFICAHandle = osThreadNew(pantalla, NULL, &GRAFICA_attributes);

  /* creation of SD */
  SDHandle = osThreadNew(lectura_sd, NULL, &SD_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */

  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_Base_Start_IT(&htim2);


  ADC1_in_NoHAL();
//  HAL_ADC_Start(&hadc1);

  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
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
  hi2c1.Init.ClockSpeed = 400000;
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
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 71;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 124;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
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
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 255;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|hook_IDLE_Pin|hook_pantalla_Pin|hook_boton_Pin
                          |hook_sd_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PC14 PC15 */
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 hook_IDLE_Pin hook_pantalla_Pin hook_boton_Pin
                           hook_sd_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_0|hook_IDLE_Pin|hook_pantalla_Pin|hook_boton_Pin
                          |hook_sd_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : SD_CS_Pin */
  GPIO_InitStruct.Pin = SD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SD_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BTNDWN_Pin BTNUP_Pin */
  GPIO_InitStruct.Pin = BTNDWN_Pin|BTNUP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void ADC1_in_NoHAL(void) {
    // 1. Habilitar reloj GPIOA y ADC1
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    // 2. PA5 como entrada analógica (canal 5)
    //mode00 entrada ; CNF=00 analogico
    GPIOA->CRL &= ~(GPIO_CRL_MODE5 | GPIO_CRL_CNF5);

    // 3. Habilitar ADC
    ADC1->CR2 |= ADC_CR2_ADON;
    for (volatile int i = 0; i < 1000; i++);
    ADC1->CR2 |= ADC_CR2_ADON;

    // 4. Calibración
    ADC1->CR2 |= ADC_CR2_CAL;
    while (ADC1->CR2 & ADC_CR2_CAL);

    // 5. Sample time canal 5
    ADC1->SMPR2 |= ADC_SMPR2_SMP5_1 | ADC_SMPR2_SMP5_0; //configurado con 71.5 ciclos

    // 6.indica que se lee del canal 5 y una sola conversion por vez
    ADC1->SQR1 = 0;
    ADC1->SQR3 = 5;

    // 7. Activar modo continuo
    ADC1->CR2 |= ADC_CR2_CONT;

    // 8. Iniciar conversión continua
    ADC1->CR2 |= ADC_CR2_ADON;
}


void calcular_barras_fft(void) {
	uint8_t modulo[FFT_COL];

	for(uint8_t k = 1; k < FFT_COL+1; k++) {
		modulo[k-1]= sqrtf(Y[k].real * Y[k].real + Y[k].imag * Y[k].imag);


	}

	for(uint8_t i=0;i<FFT_COL;i++){

		barras[i] = log_table[modulo[i]]; // genera valor entre 0 y 63 de forma logaritmica
		y[i] = BARRA_MAX_ALTURA-barras[i];// valor de altura de la barra a graficar

		if (barras[i] >= barras_peak[i]) {
			barras_peak[i] = barras[i];  // nuevo pico
		} else {
			// "fall down" lento: baja 1 unidad cada vez que graficas
			//			if (barras_peak[i] > 0) barras_peak[i]--;

			barras_peak[i] -= (barras_peak[i] > 0) ? velocidad_fall : 0;

		}
	}
}



/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_leerboton */
/**
* @brief Function implementing the Lectura_botones thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_leerboton */
void leerboton(void *argument)
{
  /* USER CODE BEGIN leerboton */
	vTaskSetApplicationTaskTag( NULL, (void*) TAG_Boton);
  /* Infinite loop */

	uint8_t lastState1 = 1, lastState2 = 1;
	  for(;;)
	  {
		  UBaseType_t Stack_res_boton=uxTaskGetStackHighWaterMark(NULL);

		  //lee el ADC
		  if(men==REP){
			  vadc=ADC1->DR;

			  if(flag_ec){
				  ganancia_high= 0.2 + (float)(vadc/ 4049.0f)*(1.4f-0.2) ;
			  }else{
				  ganancia_low= 0.2 + (float)(vadc/ 4049.0f)*(1.4f-0.2);
			  }
		  }


	      uint8_t currentState1 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_12);
	      uint8_t currentState2 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_11);

	      // Botón 1 liberado
	      if (lastState1 == 0 && currentState1 == 1) {
	    	  boton= UP;
	      }

	      // Botón 2 liberado
	      if (lastState2 == 0 && currentState2 == 1) {
	    	  boton= DWN;
	      }

	      lastState1 = currentState1;
	      lastState2 = currentState2;

	      osDelay(90);
	  }
  /* USER CODE END leerboton */
}

/* USER CODE BEGIN Header_pantalla */
/**
* @brief Function implementing the GRAFICA thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_pantalla */
void pantalla(void *argument)
{
  /* USER CODE BEGIN pantalla */

	vTaskSetApplicationTaskTag( NULL, (void*) TAG_Pantalla);
	uint8_t ver=10,k;

  /* Infinite loop */
  for(;;)
  {
	  UBaseType_t Stack_res_Pan=uxTaskGetStackHighWaterMark(NULL);
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_SET);
	  switch (men) {

	  case CAN:
		  SSD1306_Fill(SSD1306_COLOR_BLACK);
		  for(uint8_t i=0;i<5;i++){
			  SSD1306_GotoXY(5, 0+(ver*i));
			  SSD1306_Puts(archivos[i], &Font_7x10, 1);
		  }

		  SSD1306_DrawFilledRectangle(107, (sel*10)+3, 4, 4, SSD1306_COLOR_WHITE);
		  SSD1306_UpdateScreen();
		  men=SELEC;
		  break;

	  case REP:
		  //graficar las barras
		  FFT(&Y, SIZE_FFT);
		  calcular_barras_fft();
		  SSD1306_Fill(SSD1306_COLOR_BLACK);
		  for(k = 0; k < FFT_COL; k++) {
			  if (barras_peak[k] <= barras[k]) {
				  SSD1306_DrawFilledRectangle(x[k], 64 - barras_peak[k], BARRA_ANCHO - 1,barras_peak[k], SSD1306_COLOR_WHITE);
			  }else{
				  SSD1306_DrawFilledRectangle(x[k], y[k], BARRA_ANCHO - 1, barras[k], SSD1306_COLOR_WHITE);
			  }
		  }
		  //marca para seleccion de banda
		  if(flag_ec){
			  SSD1306_DrawFilledRectangle(96,0,32,2, SSD1306_COLOR_WHITE);
		  }else{
			  SSD1306_DrawFilledRectangle(0,0,32,2, SSD1306_COLOR_WHITE);
		  }

		  SSD1306_UpdateScreen();

		  if(boton==UP){
			  //hacer que se frene con este boton
			  men=CAN;
			  clse=1;
			  boton=NING;
			  osSemaphoreRelease(Sem_escHandle); //libero el semaforo por si esta bloqueando la tarea
		  }else if(boton==DWN){
			  //elegir la banda de frecuencias
			  flag_ec^=1;
			  boton=NING;
		  }
		  break;

	  case SELEC:
		  switch (boton) {
			  case UP:
				  //selecciona cancion
				  men=REP;
				  boton=NING;
				  osSemaphoreRelease(Sem_escHandle);
				  break;

			  case DWN:;
			  //baja en las canciones
			  sel++;
			  if(sel>4){
				  sel=0;
			  }
			  men=CAN;
			  boton=NING;
			  break;

			  default:
				  break;
		  }
		  break;

	 default:
		  break;
	  }
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_RESET);
	  osDelay(150);
  }
  /* USER CODE END pantalla */
}

/* USER CODE BEGIN Header_lectura_sd */
/**
* @brief Function implementing the SD thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_lectura_sd */
void lectura_sd(void *argument)
{
  /* USER CODE BEGIN lectura_sd */
	vTaskSetApplicationTaskTag( NULL, (void*) TAG_SD);

	//variables for FatFs

		FATFS FatFs; //Fatfs handle
		FIL fil; //File handle
		FRESULT  fres; //Result after operations

		DIR dir;
		FILINFO fno;

		UINT br; //numero de bytes leidos
		uint8_t primera=0,valor;
		uint16_t b;

		int16_t  salida_low,salida_high, entrada;


		fres = f_mount(&FatFs, "", 1); //1=mount now
		if (fres != FR_OK) {
			while(1);
		}

		fres = f_opendir(&dir, "/");  // Abrir directorio raíz

		if (fres == FR_OK) {
			while (1) {
				fres = f_readdir(&dir, &fno);  // Leer siguiente archivo
				if (fres != FR_OK || fno.fname[0] == 0) break;  // Fin del directorio

				if (strstr(fno.fname, ".RAW") != NULL) {

					strcpy(archivos[idx++], fno.fname);

				}
			}
			f_closedir(&dir);
			idx=0;
			men=CAN;
		}else{
			while(1);
		}

  /* Infinite loop */
  for(;;)
  {
	  UBaseType_t Stack_res_SD=uxTaskGetStackHighWaterMark(NULL); //valor en bytes
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_SET);


	  //se cierra el archivo cuando se apreta el boton stop o cuando termina la cancion
	  if(clse || br==0){
		  f_close(&fil);
		  clse=0;
		  ready=0;
		  primera=0;
		  br=1;
		  men=CAN;
	  }

	  if(men==REP){

		  if(primera<1){
			  fres = f_open(&fil, archivos[sel], FA_READ); //abrimos archivo dependiendo el boton que se aprete
			  //verificamos que funcione
			  if (fres != FR_OK) {
				  while(1);
			  }
			  primera++;
		  }

		  fres = f_read(&fil, puntero_escritura,  BUFFERSIZE ,&br);

		  if (fres != FR_OK) {
			  while(1);
		  }

		  for(b = 0; b<BUFFERSIZE; b++){
			  entrada=(int16_t )puntero_escritura[b]- 127;

			  //aplico los filtros de equalizacion
			  salida_low=(int16_t)(BiQuad(entrada, &filtro_LP)* ganancia_low);
			  salida_high=(int16_t)(BiQuad(entrada, &filtro_HP)* ganancia_high);

			  // Volver a uint8
			  valor=(uint8_t)(salida_low + salida_high) + 127;

			  if (valor < 0) valor = 0;
			  if (valor > 255) valor = 255;

			  puntero_escritura[b]=valor;

			  if(b<SIZE_FFT){
				  Y[b].real=(float)valor;
				  Y[b].imag=0;
			  }
		  }

		  ready=1;

		  if(primera<2){
			  uint8_t *temp = puntero_lectura;
			  puntero_lectura = puntero_escritura;
			  puntero_escritura = temp;
			  primera++;
		  }

	  }

	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_RESET);

	  osSemaphoreAcquire(Sem_escHandle, osWaitForever);

  }
  /* USER CODE END lectura_sd */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */
	if (htim->Instance == TIM2) {

		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);

		if(men==REP){
			if(ready){

				TIM3->CCR1 = puntero_lectura[idx++];

				if (idx >= BUFFERSIZE) {
					idx = 0;
					uint8_t *temp = puntero_lectura;
					puntero_lectura = puntero_escritura;
					puntero_escritura = temp;
					osSemaphoreRelease(Sem_escHandle);// suelto el semaforo, cuando hay que cargar un nuevo buffer
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
				}
			}

		}else {
			TIM3->CCR1 = 127; // Protección si no llegó el buffer a tiempo
		}

	}
  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */
  /* USER CODE END Callback 1 */
}

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
