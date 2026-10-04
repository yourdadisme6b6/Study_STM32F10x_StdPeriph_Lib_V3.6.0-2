/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/main.c 
  * @author  MCD Application Team
  * @version V3.6.0
  * @date    20-September-2021
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2011 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
//#include "stm32_eval.h"
#include <stdio.h>

#ifdef USE_STM32100B_EVAL
 #include "stm32100b_eval_lcd.h"
#elif defined USE_STM3210B_EVAL
 #include "stm3210b_eval_lcd.h"
#elif defined USE_STM3210E_EVAL
 #include "stm3210e_eval_lcd.h" 
#elif defined USE_STM3210C_EVAL
 #include "stm3210c_eval_lcd.h"
#elif defined USE_STM32100E_EVAL
 #include "stm32100e_eval_lcd.h"
#endif

/** @addtogroup STM32F10x_StdPeriph_Template
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#ifdef USE_STM32100B_EVAL
  #define MESSAGE1   "STM32 MD Value Line " 
  #define MESSAGE2   " Device running on  " 
  #define MESSAGE3   "  STM32100B-EVAL    " 
#elif defined (USE_STM3210B_EVAL)
  #define MESSAGE1   "STM32 Medium Density" 
  #define MESSAGE2   " Device running on  " 
  #define MESSAGE3   "   STM3210B-EVAL    " 
#elif defined (STM32F10X_XL) && defined (USE_STM3210E_EVAL)
  #define MESSAGE1   "  STM32 XL Density  " 
  #define MESSAGE2   " Device running on  " 
  #define MESSAGE3   "   STM3210E-EVAL    "
#elif defined (USE_STM3210E_EVAL)
  #define MESSAGE1   " STM32 High Density " 
  #define MESSAGE2   " Device running on  " 
  #define MESSAGE3   "   STM3210E-EVAL    " 
#elif defined (USE_STM3210C_EVAL)
  #define MESSAGE1   " STM32 Connectivity " 
  #define MESSAGE2   " Line Device running" 
  #define MESSAGE3   " on STM3210C-EVAL   "
#elif defined (USE_STM32100E_EVAL)
  #define MESSAGE1   "STM32 HD Value Line " 
  #define MESSAGE2   " Device running on  " 
  #define MESSAGE3   "  STM32100E-EVAL    "   
#endif

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
// USART_InitTypeDef USART_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_InitStructure;
/* Private function prototypes -----------------------------------------------*/
void NVIC_Configuration(void);
void Serial_Init(void);
void Serial_SendByte(uint8_t Byte);
void Serial_SendString(char *String);
#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/* Private functions ---------------------------------------------------------*/
void Delay_ms(uint32_t ms)
{
    uint32_t i, j;
    for(i = 0; i < ms; i++)
    {
        for(j = 0; j < 8000; j++) // ??8000????,???????????
        {
            __NOP(); // ???,?????????
        }
    }
}
// 初始化串口通信
void Serial_Init(void)
{
    // 开启时钟，USART1是APB2的外设，其他的串口都是APB1的外设
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    // 开启GPIO时钟，后续读写数据
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    // 初始化GPIO引脚
    // 目前只需要数据发送，所以只初始化TX即可
    // 将PA2配置为复用推挽输出，供USART1的TX使用
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP; // 复用推挽输出
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2;      // PA2做TX
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    // 初始化USART
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate            = 9600;                           // 波特率
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 不使用流控
    USART_InitStructure.USART_Mode                = USART_Mode_Tx;                  // 只发送，TX
    USART_InitStructure.USART_Parity              = USART_Parity_No;                // 不需要校验位
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;               // 1位停止位
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;            // 8位字长，不需要奇偶校验位
    USART_Init(USART2, &USART_InitStructure);
    // 开启USART
    USART_Cmd(USART2, ENABLE);
}
// 串口发送一个字节的数据
void Serial_SendByte(uint8_t Byte)
{
    USART_SendData(USART2, Byte);
    // 还需要等待一下，等TDR的数据转移到移位寄存器
    // 要不然数据还在TDR进行等待，我们再写入数据，就会产生数据覆盖
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET); // 等待发送寄存器空标志位置1
    // 标志位置SET（高电平）后不需要手动清零
    // 下一次再SendData，标志位自动清零
}
// 发送一个字符串
void Serial_SendString(char *String)
{
    uint8_t i = 0; // 循环遍历
    for (i = 0; String[i] != '\0'; i++) 
	{
		Serial_SendByte(String[i]);
    }
}
/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{
  /*!< At this stage the microcontroller clock setting is already configured, 
       this is done through SystemInit() function which is called from startup
       file (startup_stm32f10x_xx.s) before to branch to application main.
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32f10x.c file
     */     

  /* Initialize LEDs, Key Button, LCD and COM port(USART) available on
     STM3210X-EVAL board ******************************************************/
//  STM_EVAL_LEDInit(LED1);
//  STM_EVAL_LEDInit(LED2);
//  STM_EVAL_LEDInit(LED3);
//  STM_EVAL_LEDInit(LED4);

  /* USARTx configured as follow:
        - BaudRate = 115200 baud  
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive and transmit enabled
  */
//  USART_InitStructure.USART_BaudRate = 115200;
//  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
//  USART_InitStructure.USART_StopBits = USART_StopBits_1;
//  USART_InitStructure.USART_Parity = USART_Parity_No;
//  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
//  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

//  STM_EVAL_COMInit(COM1, &USART_InitStructure);

  /* Initialize the LCD */
#ifdef USE_STM32100B_EVAL
  STM32100B_LCD_Init();
#elif defined (USE_STM3210B_EVAL)
  STM3210B_LCD_Init();
#elif defined (USE_STM3210E_EVAL)
  STM3210E_LCD_Init();
#elif defined (USE_STM3210C_EVAL)
  STM3210C_LCD_Init();
#elif defined (USE_STM32100E_EVAL)
  STM32100E_LCD_Init();  
#endif

  /* Display message on STM3210X-EVAL LCD *************************************/
  /* Clear the LCD */ 
//  LCD_Clear(LCD_COLOR_WHITE);

//  /* Set the LCD Back Color */
//  LCD_SetBackColor(LCD_COLOR_BLUE);
//  /* Set the LCD Text Color */
//  LCD_SetTextColor(LCD_COLOR_WHITE);
//  LCD_DisplayStringLine(LCD_LINE_0, (uint8_t *)MESSAGE1);
//  LCD_DisplayStringLine(LCD_LINE_1, (uint8_t *)MESSAGE2);
//  LCD_DisplayStringLine(LCD_LINE_2, (uint8_t *)MESSAGE3);

  /* Retarget the C library printf function to the USARTx, can be USART1 or USART2
     depending on the EVAL board you are using ********************************/
//  printf("\n\r %s", MESSAGE1);
//  printf(" %s", MESSAGE2);
//  printf(" %s\n\r", MESSAGE3);

  /* Turn on leds available on STM3210X-EVAL **********************************/
//  STM_EVAL_LEDOn(LED1);
//  STM_EVAL_LEDOn(LED2);
//  STM_EVAL_LEDOn(LED3);
//  STM_EVAL_LEDOn(LED4);

  /* Add your application code here
     */
	/* GPIOA clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);  
		GPIOA->BRR = 0x00000003;
	/* TIM3 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	
	  /* TIM3 configuration:  mode ------------------------
     The external signal is connected to TIM3 CH2 pin (PA.01), 
  ------------------------------------------------------------ */
	  /* Set the configuration */
			
  TIM_InitStructure.TIM_Period = 10000-1;
  TIM_InitStructure.TIM_Prescaler = 7200-1;
  TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_InitStructure.TIM_RepetitionCounter = 0x0000;
	TIM_TimeBaseInit(TIM3, &TIM_InitStructure);
		//清除定时器更新标志位
	//TIM_TimeBaseInit函数末尾，手动产生了更新事件
	//若不清除此标志位，则开启中断后，会立刻进入一次中断
	//如果不介意此问题，则不清除此标志位也可
	//预分频器是有一个缓冲寄存器的
	//我们写的值只有在更新事件时，才会真正起作用
	//所以为了让值立刻起作用
	//TIM_TimeBaseInit函数末尾手动产生了一个更新事件
	//但同时，它的副作用就是，更新事件和更新中断是同时发生的
	//更新中断会置更新中断标志位
	//一旦初始化完成，更新中断就会立刻进入
	TIM_ClearFlag(TIM3, TIM_FLAG_Update);
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);	
	NVIC_Configuration();
	TIM_Cmd(TIM3,ENABLE);	//启动定时器
	
	Serial_Init();
	/* Infinite loop */
	
  while (1)
  {
//		GPIOA->BSRR = 0x00000006;
//		Delay_ms(1000);
//		GPIOA->BRR  = 0x00000006;
//		Delay_ms(1000);

//		USART_SendData(USART2, 0x55);
//		Delay_ms(100);
//printf("CFGR = 0x%08lX\r\n", RCC->CFGR);
//Delay_ms(100);		
//printf("CR   = 0x%08lX\r\n", RCC->CR);
//		Delay_ms(100);
//printf("USART2->BRR = 0x%04X\r\n", USART2->BRR);
//		Delay_ms(100);
		RCC_ClocksTypeDef c;
RCC_GetClocksFreq(&c);
printf("SystemCoreClock=%u  RCC_GetClocksFreq.SYSCLK=%u  PCLK1=%u  CFGR=0x%08lX  BRR=0x%04X\r\n",
       (unsigned)SystemCoreClock, (unsigned)c.SYSCLK_Frequency, (unsigned)c.PCLK1_Frequency,
       (unsigned long)RCC->CFGR, (unsigned)USART2->BRR);
			 Delay_ms(50);
			 printf("115200");
			 Delay_ms(50);
  }
}
void NVIC_Configuration(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Enable the TIM3 global Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}
/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
//PUTCHAR_PROTOTYPE
//{
//  /* Place your implementation of fputc here */
//  /* e.g. write a character to the USART */
//  USART_SendData(EVAL_COM2, (uint8_t) ch);

//  /* Loop until the end of transmission */
//  while (USART_GetFlagStatus(EVAL_COM2, USART_FLAG_TC) == RESET)
//  {}

//  return ch;
//}
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART */
  USART_SendData(USART2, (uint8_t) ch);

  /* Loop until the end of transmission */
  while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET)
  {}

  return ch;
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */


