#include "macs_tunes.h"

#if MACS_USE_UART
	
#ifdef STM32F429xx 	
	#include "stm32f4xx_hal.h"
#endif	

#ifdef MDR1986VE9x
	#include "MDR32F9Qx_port.h"
	#include "MDR32F9Qx_rst_clk.h"
	#include "MDR32F9Qx_uart.h"
#endif	
 
#ifdef MDR1986VE1T
	#include "MDR1986VE1T.h"
	#include "MDR32F9Qx_port.h"
	#include "MDR32F9Qx_rst_clk.h"
	#include "MDR32F9Qx_uart.h"
#endif

#ifdef MDR_VE4U
extern "C" {
	#include "MDR32F9Qx_port.h"
	#include "MDR32F9Qx_rst_clk.h"
	#include "MDR32F9Qx_uart.h"
}
#endif

#ifdef MDR_VN034
	#include "hal_1967VN034R1.h"

	extern void UART_DeInit  (UART_TypeDef*);
	extern int  UART_Send    (UART_TypeDef*, char* BuffAddr, unsigned int amount);
	extern int  UART_Receive (UART_TypeDef*, char* BuffAddr, unsigned int amount);

	extern int  UART_ReceiveWait (UART_TypeDef*, char* BuffAddr, unsigned int amount);

	#define NO_UINT
#endif
	 

#include "macs_system.hpp"
#include "macs_uart_adapter.hpp"
#include "macs_semaphore.hpp"

namespace utils {
	 
/*******************************  STM32F4  ************************************/	
#ifdef STM32F429xx 	

static UART_HandleTypeDef s_hal_uarts[] = { { USART1 }, { UART5 } };

class AdUart
{
public:
	bool m_used;
	Semaphore m_semph;
	bool m_error;
	UART_HandleTypeDef & m_huart;
	static uint s_cnt;
public:
	AdUart() : m_huart(s_hal_uarts[s_cnt ++]) {
		m_used = false;
		m_error = false;		
	}
};
uint AdUart::s_cnt;
AdUart s_ad_uarts[countof(s_hal_uarts)];
	
inline UART_HandleTypeDef * GetUart(UartHandler uart) { return & s_ad_uarts[uart].m_huart; }
inline UartHandler 					GetUart(UART_HandleTypeDef * uart) { return uart - s_hal_uarts; }

extern "C" void USART1_IRQHandler(void)
{
	UART_HandleTypeDef * uart = & s_ad_uarts[0].m_huart;	
	_ASSERT(uart->Instance == USART1);
  HAL_UART_IRQHandler(uart);
}

extern "C" void UART5_IRQHandler(void)
{
	UART_HandleTypeDef * uart = & s_ad_uarts[1].m_huart;	
	_ASSERT(uart->Instance == UART5);
  HAL_UART_IRQHandler(uart);
}

void Uart_InitDrv() 
{}

UartHandler Uart_Open(short num, ulong speed_bps, ushort word_length, ushort stop_bits, ushort parity)
{
	if ( num == -1 ) 
		num = ! s_ad_uarts[0].m_used ? 0 : 1;	
	else
		_ASSERT(num < countof(s_hal_uarts));
		
	if ( s_ad_uarts[num].m_used ) 
		return INVALID_UART_HANDLER;	
	
	UART_HandleTypeDef * uart = & s_ad_uarts[num].m_huart;	

  __GPIOH_CLK_ENABLE();
  __GPIOA_CLK_ENABLE();
	 
  uart->Init.BaudRate = speed_bps;
	switch ( word_length ) {
	default : _ASSERT(false);
	case 8 : uart->Init.WordLength = UART_WORDLENGTH_8B;	break;
	case 9 : uart->Init.WordLength = UART_WORDLENGTH_9B;	break;
	}
	uart->Init.StopBits = (stop_bits == 1 ? UART_STOPBITS_1 : UART_STOPBITS_2);
	switch ( parity ) {
	default :	_ASSERT(false);
	case 0 : uart->Init.Parity = UART_PARITY_NONE;	break;
	case 1 : uart->Init.Parity = UART_PARITY_ODD;	break;
	case 2 : uart->Init.Parity = UART_PARITY_EVEN;	break;
	}
  uart->Init.Mode = UART_MODE_TX_RX;
  uart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
  uart->Init.OverSampling = UART_OVERSAMPLING_8;
  HAL_UART_Init(uart);
	
	s_ad_uarts[num].m_used = true;
	
	return num;
}

Result Uart_Close(UartHandler)
{
	// TODO: close
	return ResultOk;
}
	
extern "C" void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  if(huart->Instance==USART1)
  {
    __USART1_CLK_ENABLE();
  
    /**USART1 GPIO Configuration    
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
		System::SetIrqPriority(USART1_IRQn, System::MAX_SYSCALL_INTERRUPT_PRIORITY); 
  }
	else if(huart->Instance==UART5)
  {
    __UART5_CLK_ENABLE();
  
    /**UART5 GPIO Configuration    
    PC12     ------> UART5_TX
    PD2     ------> UART5_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART5;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);


    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART5;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(UART5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(UART5_IRQn);
		System::SetIrqPriority(UART5_IRQn, System::MAX_SYSCALL_INTERRUPT_PRIORITY); 
  }
}

extern "C" void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
  if(huart->Instance==USART1)
  {
    __USART1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  } 
	else if(huart->Instance==UART5) 
	{
    __UART5_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_12);
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2);
    HAL_NVIC_DisableIRQ(UART5_IRQn);
  }
}

////////////////////////////////////////////////////////////////

Result Uart_SendWait(UartHandler uart, const Buf & buf, ulong timeout_ms)
{
	UART_HandleTypeDef * phuart = GetUart(uart);
	HAL_StatusTypeDef retcode;
	
	do 
		retcode = HAL_UART_Transmit(phuart, (uint8_t *) buf.Data(), buf.Len(), timeout_ms);	
	while ( retcode == HAL_BUSY );

	if ( retcode == HAL_TIMEOUT )
		return ResultTimeout;
	
	return retcode == HAL_OK ? ResultOk : ResultErrorInvalidState;
}

Result Uart_SendIrq(UartHandler uart, const Buf & buf)
{
	UART_HandleTypeDef * phuart = GetUart(uart);
	HAL_StatusTypeDef retcode;
	do 
		retcode = HAL_UART_Transmit_IT(phuart, (uint8_t *) buf.Data(), buf.Len());
	while ( retcode == HAL_BUSY );

	return retcode == HAL_OK ? ResultOk : ResultErrorInvalidState;
}

Result Uart_RecvWait(UartHandler uart, Buf & buf, size_t len, ulong timeout_ms)
{
	buf.Alloc(len);
	UART_HandleTypeDef * phuart = GetUart(uart);
	HAL_StatusTypeDef retcode;
	do 
		retcode = HAL_UART_Receive(phuart, buf.Data(), len, timeout_ms);	
	while ( retcode == HAL_BUSY );
	
	if ( retcode == HAL_TIMEOUT )
		return ResultTimeout;
	if ( retcode != HAL_OK )
		return ResultErrorInvalidState;
	
	buf.AddLen(len);
	return ResultOk;
}  

Result Uart_RecvIrq(UartHandler uart, Buf & buf, size_t len)
{ 
	buf.Alloc(len);
	UART_HandleTypeDef * phuart = GetUart(uart);
	HAL_StatusTypeDef drv_retcode;
	do {
		drv_retcode = HAL_UART_Receive_IT(phuart, buf.Data(), len);
	} while ( drv_retcode == HAL_BUSY );
	RET_ERROR(drv_retcode == HAL_OK, ResultErrorInvalidState);
	
	buf.AddLen(len);
	return ResultOk;
}
 
Result Uart_RecvSemph(UartHandler uart, Buf & buf, size_t len, ulong timeout_ms)
{
	buf.Alloc(len);
	UART_HandleTypeDef * phuart = GetUart(uart);
	HAL_StatusTypeDef drv_retcode;
	
	do {
		do {
			drv_retcode = HAL_UART_Receive_IT(phuart, buf.Data(), len);
		} while ( drv_retcode == HAL_BUSY );	
		RET_ERROR(drv_retcode == HAL_OK, ResultErrorInvalidState);

		Result retcode = s_ad_uarts[uart].m_semph.Wait(timeout_ms);	
		if ( retcode == ResultTimeout )
			return ResultTimeout;
		RET_ERROR(retcode == ResultOk, retcode);	
	} while ( s_ad_uarts[uart].m_error );
	
	buf.AddLen(len);
	return ResultOk;
}
////////////////////////////////////////////////////////////////
extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef * huart)
{
	UartHandler uart = GetUart(huart);
	Uart_OnSend(uart);
}

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef * huart)
{
	UartHandler uart = GetUart(huart);
	//Uart_OnRecv(uart, huart->RxXferSize);
	s_ad_uarts[uart].m_error = false;
	s_ad_uarts[uart].m_semph.Signal();
}

extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef * huart)
{	
	UartHandler uart = GetUart(huart);
	s_ad_uarts[uart].m_error = true;	
	s_ad_uarts[uart].m_semph.Signal();
}
#endif	// #ifdef STM32F429xx 	


/*****************************  MDR1986VE9x  **********************************/

#ifdef MDR1986VE9x

static bool s_used_uarts[2] = { false, false };

#define UART_DEF(num) 			((num) == 0 ? MDR_UART2 : MDR_UART1)	// yes, use MDR_UART2 first
#define UART_NUM(uart)			((uart) == MDR_UART2 ? 0 : 1)
#define UART_RST_CLK(uart)	((uart) == MDR_UART1 ? RST_CLK_PCLK_UART1 : RST_CLK_PCLK_UART2)
#define UART_IRQn(uart)			((uart) == MDR_UART1 ? UART1_IRQn : UART2_IRQn)

void Uart_InitDrv() 
{
	SystemInit();  //GermanT: TODO: really need it?
}

UartHandler Uart_Open(short num, ulong speed_bps, ushort word_length, ushort stop_bits, ushort parity)
{
	RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTF,ENABLE);
	if ( num == -1 ) 
		num = ! s_used_uarts[0] ? 0 : 1;	
	else
		_ASSERT(num < countof(s_used_uarts));
		
	if ( s_used_uarts[num] ) 
		return INVALID_UART_HANDLER;	
	
	MDR_UART_TypeDef * uart = UART_DEF(num);	
	
	PORT_InitTypeDef PortInitStructure;
  PortInitStructure.PORT_Pin   = PORT_Pin_0 | PORT_Pin_1;
  PortInitStructure.PORT_MODE  = PORT_MODE_DIGITAL;
  PortInitStructure.PORT_FUNC  = PORT_FUNC_OVERRID;
  PortInitStructure.PORT_SPEED = PORT_SPEED_MAXFAST;
  PORT_Init(MDR_PORTF, & PortInitStructure);

  /* Enable the RTCHSE clock on UART */
  RST_CLK_PCLKcmd(UART_RST_CLK(uart), ENABLE);

  /* Set the UART HCLK division factor */
  UART_BRGInit(uart, UART_HCLKdiv1);

  UART_DeInit(uart);

  /* Disable interrupt on UART */
  NVIC_DisableIRQ(UART_IRQn(uart));
	
  /* UART configuration */
	UART_InitTypeDef UARTInitStructure;
	
	// У платы 1986VE91 требуется выставить 95% скорости от указанной в документации. Баг платы(возможно баг всех плат ve91).
	// Предыдущее объяснение этого бага: Необходимо для соответствия длине импульсов STM	
	#ifdef MDR1986VE91VE94
		#define BAUD_RATE_KOEF 95	
	#else	
		#define BAUD_RATE_KOEF 100	
	#endif	
	UARTInitStructure.UART_BaudRate = (speed_bps*BAUD_RATE_KOEF)/100;
	if ( parity ) 
		-- word_length;  
	switch ( word_length ) {
	case 5 : UARTInitStructure.UART_WordLength = UART_WordLength5b;	break;
	case 6 : UARTInitStructure.UART_WordLength = UART_WordLength6b;	break;
	case 7 : UARTInitStructure.UART_WordLength = UART_WordLength7b;	break;
	default : _ASSERT(false);
	case 8 : UARTInitStructure.UART_WordLength = UART_WordLength8b;	break;
	}
	UARTInitStructure.UART_StopBits = (stop_bits == 1 ? UART_StopBits1 : UART_StopBits2);
	switch ( parity ) {
	default :	_ASSERT(false);
	case 0 : UARTInitStructure.UART_Parity = UART_Parity_No;		break;
	case 1 : UARTInitStructure.UART_Parity = UART_Parity_Odd;		break;
	case 2 : UARTInitStructure.UART_Parity = UART_Parity_Even;	break;
	}
  UARTInitStructure.UART_FIFOMode                = UART_FIFO_ON;
  UARTInitStructure.UART_HardwareFlowControl     = UART_HardwareFlowControl_RXE | 
                                                   UART_HardwareFlowControl_TXE;
  UART_Init(uart, & UARTInitStructure);
	
  UART_Cmd(uart, ENABLE);
		
	s_used_uarts[UART_NUM(uart)] = true;
	
	return (UartHandler) uart;
}

Result Uart_Close(UartHandler handler)
{
	MDR_UART_TypeDef * uart = (MDR_UART_TypeDef *) handler;		
	_ASSERT(s_used_uarts[UART_NUM(uart)]);
  UART_Cmd(uart, DISABLE);
	s_used_uarts[UART_NUM(uart)] = false;
	return ResultOk;
}

Result Uart_SendWait(UartHandler handler, const Buf & buf, ulong timeout_ms)
{
	LazyBoy lb;	
	MDR_UART_TypeDef * uart = (MDR_UART_TypeDef *) handler;		
	const byte * data = buf.Data();
	size_t cnt = buf.Len();
	while ( cnt -- ) {
		UART_SendData(uart, * data ++);
		
		/* Wait for transmition end */
		while ( UART_GetFlagStatus(uart, UART_FLAG_TXFF) == SET ) 
			if ( lb.Spend() > timeout_ms ) 
				return ResultTimeout;
				
		while ( UART_GetFlagStatus(uart, UART_FLAG_BUSY) == SET ) 
			if ( lb.Spend() > timeout_ms ) 
				return ResultTimeout;
	}
	return ResultOk;
}

Result Uart_RecvWait(UartHandler handler, Buf & buf, size_t len, ulong timeout_ms)
{
	LazyBoy lb;	
	buf.Alloc(len);
	byte * data = buf.Data();
	size_t cnt = len;
	MDR_UART_TypeDef * uart = (MDR_UART_TypeDef *) handler;		
	while ( cnt -- ) {		
		for(;;) {
			/* Wait for any data in the receiver buffer */	
			while ( UART_GetFlagStatus(uart, UART_FLAG_RXFE) == SET )
				if ( lb.Spend() > timeout_ms ) 
					return ResultTimeout;

			uint16_t resp = UART_ReceiveData(uart);
			uint8_t flags = UART_Flags(resp);	
				
			if ( ! flags || (flags == (0x1 << (UART_Data_FE - 8))) ) {
				* data ++ = UART_Data(resp);
				break;
			}
		}
	}
	buf.AddLen(len);
	return ResultOk;
}  

Result Uart_SendIrq(UartHandler uart, const Buf & buf)
{
	// TODO:
	return ResultOk;
}

Result Uart_RecvIrq(UartHandler uart, Buf & buf, size_t len)
{
	// TODO:
	return ResultOk;
}

Result Uart_RecvSemph(UartHandler uart, Buf & buf, size_t len, ulong timeout_ms)
{
	// TODO:
	return Uart_RecvWait(uart, buf, len, timeout_ms);
}

#endif	// #ifdef MDR1986VE9x

 
/*******************************  MDR_VE4U  ************************************/	

#ifdef MDR_VE4U

void Uart_InitDrv() 
{
	RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTB, ENABLE);
	RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTC, ENABLE);

	/* Fill PortInit structure*/
	PORT_InitTypeDef PortInit;
	PortInit.PORT_PULL_UP = PORT_PULL_UP_OFF;
	PortInit.PORT_PULL_DOWN = PORT_PULL_DOWN_OFF;
	PortInit.PORT_PD_SHM = PORT_PD_SHM_OFF;
	PortInit.PORT_PD = PORT_PD_DRIVER;
	PortInit.PORT_GFEN = PORT_GFEN_OFF;
	PortInit.PORT_FUNC = PORT_FUNC_MAIN;
	PortInit.PORT_SPEED = PORT_SPEED_MAXFAST;
	PortInit.PORT_MODE = PORT_MODE_DIGITAL;

	PortInit.PORT_OE = PORT_OE_OUT;
	PortInit.PORT_Pin = PORT_Pin_0;
	PORT_Init(MDR_PORTB, &PortInit);
	PORT_Init(MDR_PORTC, &PortInit);

	PortInit.PORT_OE = PORT_OE_IN;
	PortInit.PORT_Pin = PORT_Pin_1;
	PORT_Init(MDR_PORTB, &PortInit);
	PORT_Init(MDR_PORTC, &PortInit);

	/* Enables the CPU_CLK clock on UART1,UART2 */
	RST_CLK_PCLKcmd(RST_CLK_PCLK_UART1, ENABLE);
	RST_CLK_PCLKcmd(RST_CLK_PCLK_UART2, ENABLE);

	/* Set the HCLK division factor = 1 for UART1,UART2*/
	UART_BRGInit(MDR_UART1, UART_HCLKdiv1);
	UART_BRGInit(MDR_UART2, UART_HCLKdiv1);
}
 
// TODO: UART number
UartHandler Uart_Open(short num, ulong speed_bps, ushort word_length, ushort stop_bits, ushort parity)
{
	MDR_UART_TypeDef * uart = (num == -1 ? MDR_UART1 : MDR_UART2);
  UART_DeInit(uart);

	if ( uart == MDR_UART1 ) {
		NVIC_SetPriority(UART1_IRQn, System::MAX_SYSCALL_INTERRUPT_PRIORITY); 
		NVIC_EnableIRQ(UART1_IRQn);	
	} 

  /* UART configuration */
	UART_InitTypeDef UARTInitStructure;
  UARTInitStructure.UART_BaudRate = (98 * speed_bps) / 100;	// Необходимо для соответствия длине импульсов STM
	//UARTInitStructure.UART_BaudRate = speed_bps;
	if ( parity ) 
		-- word_length;  
	switch ( word_length ) {
	case 5 : UARTInitStructure.UART_WordLength = UART_WordLength5b;	break;
	case 6 : UARTInitStructure.UART_WordLength = UART_WordLength6b;	break;
	case 7 : UARTInitStructure.UART_WordLength = UART_WordLength7b;	break;
	default : _ASSERT(false);
	case 8 : UARTInitStructure.UART_WordLength = UART_WordLength8b;	break;
	}
	UARTInitStructure.UART_StopBits = (stop_bits == 1 ? UART_StopBits1 : UART_StopBits2);
	switch ( parity ) {
	default :	_ASSERT(false);
	case 0 : UARTInitStructure.UART_Parity = UART_Parity_No;		break;
	case 1 : UARTInitStructure.UART_Parity = UART_Parity_Odd;		break;
	case 2 : UARTInitStructure.UART_Parity = UART_Parity_Even;	break;
	}
  UARTInitStructure.UART_FIFOMode                = UART_FIFO_ON;	
  UARTInitStructure.UART_HardwareFlowControl     = UART_HardwareFlowControl_RXE | 
                                                   UART_HardwareFlowControl_TXE;
  UART_Init(uart, & UARTInitStructure);

	if ( uart == MDR_UART1 ) 
		UART_ITConfig(MDR_UART1, UART_IT_RX, ENABLE);
		
	UART_Cmd(uart, ENABLE);

	return (UartHandler) uart;
}

Result Uart_Close(UartHandler handler)
{
	MDR_UART_TypeDef * uart = (MDR_UART_TypeDef *) handler;		
  UART_Cmd(uart, DISABLE);
	return ResultOk;
}

Result Uart_SendWait(UartHandler handler, const Buf & buf, ulong timeout_ms)
{
	LazyBoy lb;	
	MDR_UART_TypeDef * uart = (MDR_UART_TypeDef *) handler;		
	const byte * data = buf.Data();
	size_t cnt = buf.Len();
	while ( cnt -- ) {
		while ( UART_GetFlagStatus(uart, UART_FLAG_TXFE ) != SET )
			if ( lb.Spend() > timeout_ms ) 
				return ResultTimeout;
		
		UART_SendData(uart, * data ++);
	}
	return ResultOk;
}

Result Uart_RecvWait(UartHandler handler, Buf & buf, size_t len, ulong timeout_ms)
{
	LazyBoy lb;	
	buf.Alloc(len);
	byte * data = buf.Data();
	size_t cnt = len;
	MDR_UART_TypeDef * uart = (MDR_UART_TypeDef *) handler;		
	while ( cnt -- ) {		
		/* Wait for any data in the receiver buffer */	
		while ( UART_GetFlagStatus(uart, UART_FLAG_RXFE) == SET )
			if ( lb.Spend() > timeout_ms ) 
				return ResultTimeout;

		uint16_t ret = UART_ReceiveData(uart);
		* data ++ = ret;
	}
	buf.AddLen(len);
	return ResultOk;
}  

Result Uart_SendIrq(UartHandler uart, const Buf & buf)
{
	// TODO:
	return ResultOk;
}

Result Uart_RecvIrq(UartHandler uart, Buf & buf, size_t len)
{
	// TODO:
	return ResultOk;
}

static Semaphore s_sem;
static byte s_buf[256];
static byte * s_head = s_buf, * s_tail = s_buf;
inline byte * NextBP(byte * bp) 
{
	return (bp + 1 < & s_buf[countof(s_buf)]) ? (bp + 1) : & s_buf[0];
}

struct UartStat
{
	ulong m_irq_cnt;
	ulong m_ovf_cnt;	
	ulong m_sem_cnt_w, m_sem_cnt_i;
};

static UartStat s_uart_stat;

extern "C" {
void UART1_IRQHandler()
{
	++ s_uart_stat.m_irq_cnt;
	
	while ( UART_GetFlagStatus(MDR_UART1, UART_FLAG_RXFE) != SET ) {
		uint16_t ret = UART_ReceiveData(MDR_UART1);
		* s_tail = ret;
		byte * tail = NextBP(s_tail);
		if ( tail != s_head )
			s_tail = tail;			
		else	
			++ s_uart_stat.m_ovf_cnt;
	}
	UART_ClearITPendingBit(MDR_UART1, UART_IT_RX);
	s_sem.Signal();
}
}

Result Uart_RecvSemph(UartHandler handler, Buf & buf, size_t len, ulong timeout_ms)
{
	buf.Alloc(len);
	byte * data = buf.Data();
	size_t cnt = len;
	MDR_UART_TypeDef * uart = (MDR_UART_TypeDef *) handler;		
	_ASSERT(uart == MDR_UART1);
	for (;;) {		
		while ( s_head != s_tail ) {
			* data ++ = * s_head;
			s_head = NextBP(s_head);
			if ( ! -- cnt ) {
				buf.AddLen(len);
				return ResultOk;
			}
		}				
		Result retcode = s_sem.Wait(10);
		if ( retcode == ResultTimeout ) {
			++ s_uart_stat.m_sem_cnt_w;
		} else {
			++ s_uart_stat.m_sem_cnt_i;
		}
	}
}

#endif	// #ifdef MDR_VE4U

/*****************************  MDR1986VE1T  **********************************/	

#ifdef MDR1986VE1T

void Uart_InitDrv() 
{
	
	/* Set CPUClk Prescaler */
	RST_CLK_CPUclkPrescaler(RST_CLK_CPUclkDIV1);

	/* Select the CPU clock source */
	RST_CLK_CPUclkSelection(RST_CLK_CPUclkCPU_C3);
	
	RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTC | RST_CLK_PCLK_PORTD, ENABLE);  
	
	PORT_InitTypeDef port_init;	
	PORT_StructInit(& port_init);
  port_init.PORT_FUNC  = PORT_FUNC_MAIN;
  port_init.PORT_SPEED = PORT_SPEED_MAXFAST;
  port_init.PORT_MODE  = PORT_MODE_DIGITAL;
	
  port_init.PORT_OE    = PORT_OE_OUT;
  port_init.PORT_Pin   = PORT_Pin_3;
  PORT_Init(MDR_PORTC, & port_init);
  port_init.PORT_Pin   = PORT_Pin_13;
  PORT_Init(MDR_PORTD, & port_init);

  port_init.PORT_OE    = PORT_OE_IN;
  port_init.PORT_Pin   = PORT_Pin_4;
  PORT_Init(MDR_PORTC, & port_init);
  port_init.PORT_Pin   = PORT_Pin_14;
  PORT_Init(MDR_PORTD, & port_init);
	
	RST_CLK_PCLKcmd(RST_CLK_PCLK_UART1, ENABLE);
	UART_BRGInit(MDR_UART1, UART_HCLKdiv1);
	UART_DeInit(MDR_UART1);

	RST_CLK_PCLKcmd(RST_CLK_PCLK_UART2, ENABLE);
	UART_BRGInit(MDR_UART2, UART_HCLKdiv1);
	UART_DeInit(MDR_UART2);
}
 
UartHandler Uart_Open(short num, ulong speed_bps, ushort word_length, ushort stop_bits, ushort parity)
{
	MDR_UART_TypeDef * uart = MDR_UART2;  // TODO: UART number
  UART_DeInit(uart);

	if ( uart == MDR_UART2 ) {  // TODO: для всех
		NVIC_SetPriority(UART2_IRQn, System::MAX_SYSCALL_INTERRUPT_PRIORITY); 
		NVIC_EnableIRQ(UART2_IRQn);	
	}

  /* UART configuration */
	UART_InitTypeDef UARTInitStructure;
  UARTInitStructure.UART_BaudRate = speed_bps; //(98 * speed_bps) / 100;	// Необходимо для соответствия длине импульсов STM
	if ( parity ) 
		-- word_length;  
	switch ( word_length ) {
	case 5 : UARTInitStructure.UART_WordLength = UART_WordLength5b;	break;
	case 6 : UARTInitStructure.UART_WordLength = UART_WordLength6b;	break;
	case 7 : UARTInitStructure.UART_WordLength = UART_WordLength7b;	break;
	default : _ASSERT(false);
	case 8 : UARTInitStructure.UART_WordLength = UART_WordLength8b;	break;
	}
	UARTInitStructure.UART_StopBits = (stop_bits == 1 ? UART_StopBits1 : UART_StopBits2);
	switch ( parity ) {
	default :	_ASSERT(false);
	case 0 : UARTInitStructure.UART_Parity = UART_Parity_No;		break;
	case 1 : UARTInitStructure.UART_Parity = UART_Parity_Odd;		break;
	case 2 : UARTInitStructure.UART_Parity = UART_Parity_Even;	break;
	}
  UARTInitStructure.UART_FIFOMode                = UART_FIFO_OFF;	
  UARTInitStructure.UART_HardwareFlowControl     = UART_HardwareFlowControl_RXE | UART_HardwareFlowControl_TXE;
  UART_Init(uart, & UARTInitStructure);

	if ( uart == MDR_UART2 ) 
		UART_ITConfig(MDR_UART2, UART_IT_RX, ENABLE);
		
	UART_Cmd(uart, ENABLE);

	return (UartHandler) uart;
}

Result Uart_Close(UartHandler handler)
{
	MDR_UART_TypeDef * uart = (MDR_UART_TypeDef *) handler;		
  UART_Cmd(uart, DISABLE);
	return ResultOk;
}


Result Uart_SendWait(UartHandler handler, const Buf & buf, ulong timeout_ms)
{
	LazyBoy lb;	
	MDR_UART_TypeDef * uart = (MDR_UART_TypeDef *) handler;		
	const byte * data = buf.Data();
	size_t cnt = buf.Len();
	while ( cnt -- ) {
		while ( UART_GetFlagStatus(uart, UART_FLAG_TXFF) == SET || 
						UART_GetFlagStatus(uart, UART_FLAG_BUSY) == SET )
			if ( lb.Spend() > timeout_ms ) 
				return ResultTimeout;
		
		UART_SendData(uart, * data ++);
	}
	return ResultOk;
}

Result Uart_RecvWait(UartHandler handler, Buf & buf, size_t len, ulong timeout_ms)
{
	LazyBoy lb;	
	buf.Alloc(len);
	byte * data = buf.Data();
	size_t cnt = len;
	MDR_UART_TypeDef * uart = (MDR_UART_TypeDef *) handler;		
	while ( cnt -- ) {		
		/* Wait for any data in the receiver buffer */	
		while ( UART_GetFlagStatus(uart, UART_FLAG_RXFE) == SET )
			if ( lb.Spend() > timeout_ms ) 
				return ResultTimeout;

		uint16_t ret = UART_ReceiveData(uart);
		* data ++ = ret;  // TODO: UART_Flags(ret) != 0
	}
	buf.AddLen(len);
	return ResultOk;
}  

Result Uart_SendIrq(UartHandler uart, const Buf & buf)
{
	// TODO:
	return ResultOk;
}

Result Uart_RecvIrq(UartHandler uart, Buf & buf, size_t len)
{
	// TODO:
	return ResultOk;
}

static Semaphore s_sem;
static byte s_buf[256];
static byte * s_head = s_buf, * s_tail = s_buf;
inline byte * NextBP(byte * bp) 
{
	return (bp + 1 < & s_buf[countof(s_buf)]) ? (bp + 1) : & s_buf[0];
}

struct UartStat
{
	ulong m_irq_cnt;
	ulong m_ovf_cnt;	
	ulong m_sem_cnt_w, m_sem_cnt_i;
};

static UartStat s_uart_stat;

extern "C" {
void UART2_IRQHandler()
{
	++ s_uart_stat.m_irq_cnt;
	
	while ( UART_GetFlagStatus(MDR_UART2, UART_FLAG_RXFE) != SET ) {
		uint16_t ret = UART_ReceiveData(MDR_UART2);
		* s_tail = ret;
		byte * tail = NextBP(s_tail);
		if ( tail != s_head )
			s_tail = tail;			
		else	
			++ s_uart_stat.m_ovf_cnt;
	}
	UART_ClearITPendingBit(MDR_UART2, UART_IT_RX);
	s_sem.Signal();
}
}


Result Uart_RecvSemph(UartHandler handler, Buf & buf, size_t len, ulong timeout_ms)
{
	buf.Alloc(len);
	byte * data = buf.Data();
	size_t cnt = len;
	MDR_UART_TypeDef * uart = (MDR_UART_TypeDef *) handler;		
	_ASSERT(uart == MDR_UART2);
	for (;;) {		
		while ( s_head != s_tail ) {
			* data ++ = * s_head;
			s_head = NextBP(s_head);
			if ( ! -- cnt ) {
				buf.AddLen(len);
				return ResultOk;
			}
		}				
		Result retcode = s_sem.Wait(10);
		if ( retcode == ResultTimeout ) {
			++ s_uart_stat.m_sem_cnt_w;
		} else {
			++ s_uart_stat.m_sem_cnt_i;
		}
	}
}

#endif	// #ifdef MDR1986VE1T

/*******************************  MDR_VN34  ************************************/

#if MACS_MCU_TS

static UART_TypeDef * s_hal_uarts[] = { LX_UART0, LX_UART1 };

class AdUart
{
public:
	bool m_used;
	Semaphore m_semph;
	bool m_error;
	UART_TypeDef * m_huart;
	static uint s_cnt;
public:
	AdUart() : m_huart(s_hal_uarts[s_cnt ++]) {
		m_used = false;
		m_error = false;
	}
};
uint AdUart::s_cnt;
AdUart s_ad_uarts[countof(s_hal_uarts)];

inline UART_TypeDef * GetUart(UartHandler uart)    { return s_ad_uarts[uart].m_huart; }
inline UartHandler 		GetUart(UART_TypeDef * uart) { return s_hal_uarts[0] == uart ? 0 : 1; }
 
void Uart_InitDrv()
{}

UartHandler Uart_Open(short num, ulong speed_bps, ushort word_length, ushort stop_bits, ushort parity)
{
	if ( num == -1 )
		num = ! s_ad_uarts[0].m_used ? 0 : 1;
	else
		_ASSERT(num < countof(s_hal_uarts));

	if ( s_ad_uarts[num].m_used )
		return INVALID_UART_HANDLER;

	UART_InitTypeDef conf;
	conf.BitRate = 4 * speed_bps;  // коррекция для OverSampling normal
	conf.WorkMode = txrx;
	conf.OverSampling = normal;
	conf.WordLength = word_length;
	conf.StopBits = stop_bits;

	switch ( parity ) {
	default :	_ASSERT(false);
	case 0 : conf.ParityMode = offParity;	break;
	case 1 : conf.ParityMode = odd;	      break;
	case 2 : conf.ParityMode = even;	    break;
	}
	 
	conf.FIFOSize = (WordSize_type) 1;  // в комментариях к hal_uart ошибка.
	conf.TXDMode = direct;
	conf.DMACtrlErr = dis;
	conf.tx_int = conf.rx_int = conf.rx_err_int = conf.ms_int = conf.ud_int = conf.tx_empt_int = conf.rx_to_int = dis;

	int res = UART_Init(s_ad_uarts[num].m_huart, & conf, 25);

	s_ad_uarts[num].m_used = true;

	return num;
}

Result Uart_Close(UartHandler uart)
{
	UART_TypeDef * phuart = GetUart(uart);
	UART_DeInit(phuart);
	s_ad_uarts[uart].m_used = false;
	return ResultOk;
}

////////////////////////////////////////////////////////////////

Result Uart_SendWait(UartHandler uart, const Buf & buf, ulong timeout_ms)
{
	UART_TypeDef * phuart = GetUart(uart);
	int retcode;

	do
		retcode = UART_Send(phuart, (char *) buf.Data(), buf.Len());
	while ( retcode != 0 );

	return retcode == 0 ? ResultOk : ResultErrorInvalidState;
}

Result Uart_SendIrq(UartHandler uart, const Buf & buf)
{
	return Uart_SendWait(uart, buf, INFINITE_TIMEOUT);
}

Result Uart_RecvWait(UartHandler uart, Buf & buf, size_t len, ulong timeout_ms)
{
	buf.Alloc(len);
	UART_TypeDef * phuart = GetUart(uart);
	int retcode;
	do
		retcode = UART_ReceiveWait(phuart, (char *) buf.Data(), len);
	while ( retcode != 0 );

	buf.AddLen(len);
	return ResultOk;
}

Result Uart_RecvIrq(UartHandler uart, Buf & buf, size_t len)
{
	return Uart_RecvWait(uart, buf, len, INFINITE_TIMEOUT);
}

Result Uart_RecvSemph(UartHandler uart, Buf & buf, size_t len, ulong timeout_ms)
{
	return Uart_RecvWait(uart, buf, len, timeout_ms);
}

#endif	// #if MACS_MCU_TS

}	// namespace utils

#endif	// #if MACS_USE_UART
