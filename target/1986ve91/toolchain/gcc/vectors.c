#include "exception_handlers.h"

void Default_Handler (void) __attribute__((weak));

/* ARMCM3 Specific Interrupts */
void WDT_IRQHandler      (void) __attribute__ ((weak, alias("Default_Handler")));
void RTC_IRQHandler      (void) __attribute__ ((weak, alias("Default_Handler")));
void TIM0_IRQHandler     (void) __attribute__ ((weak, alias("Default_Handler")));
void TIM2_IRQHandler     (void) __attribute__ ((weak, alias("Default_Handler")));
void MCIA_IRQHandler     (void) __attribute__ ((weak, alias("Default_Handler")));
void MCIB_IRQHandler     (void) __attribute__ ((weak, alias("Default_Handler")));
void UART0_IRQHandler    (void) __attribute__ ((weak, alias("Default_Handler")));
void UART1_IRQHandler    (void) __attribute__ ((weak, alias("Default_Handler")));
void UART2_IRQHandler    (void) __attribute__ ((weak, alias("Default_Handler")));
void UART4_IRQHandler    (void) __attribute__ ((weak, alias("Default_Handler")));
void AACI_IRQHandler     (void) __attribute__ ((weak, alias("Default_Handler")));
void CLCD_IRQHandler     (void) __attribute__ ((weak, alias("Default_Handler")));
void ENET_IRQHandler     (void) __attribute__ ((weak, alias("Default_Handler")));
void USBDC_IRQHandler    (void) __attribute__ ((weak, alias("Default_Handler")));
void USBHC_IRQHandler    (void) __attribute__ ((weak, alias("Default_Handler")));
void CHLCD_IRQHandler    (void) __attribute__ ((weak, alias("Default_Handler")));
void FLEXRAY_IRQHandler  (void) __attribute__ ((weak, alias("Default_Handler")));
void CAN_IRQHandler      (void) __attribute__ ((weak, alias("Default_Handler")));
void LIN_IRQHandler      (void) __attribute__ ((weak, alias("Default_Handler")));
void I2C_IRQHandler      (void) __attribute__ ((weak, alias("Default_Handler")));
void CPU_CLCD_IRQHandler (void) __attribute__ ((weak, alias("Default_Handler")));
void UART3_IRQHandler    (void) __attribute__ ((weak, alias("Default_Handler")));
void SPI_IRQHandler      (void) __attribute__ ((weak, alias("Default_Handler")));

extern unsigned int __stack;

typedef void (*const pHandler)(void);

// The vector table.
// This relies on the linker script to place at correct location in memory.

pHandler __isr_vectors[] __attribute__ ((section(".isr_vector"),used)) =  {
        (pHandler) &__stack,                      // The initial stack pointer
        Reset_Handler,                            // The reset handler

        NMI_Handler,                              // The NMI handler
        HardFault_Handler,                        // The hard fault handler

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
        MemManage_Handler,                        // The MPU fault handler
        BusFault_Handler,// The bus fault handler
        UsageFault_Handler,// The usage fault handler
#else
        0, 0, 0,				  // Reserved
#endif
        0,                                        // Reserved
        0,                                        // Reserved
        0,                                        // Reserved
        0,                                        // Reserved
        SVC_Handler,                              // SVCall handler
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
        DebugMon_Handler,                         // Debug monitor handler
#else
        0,					  // Reserved
#endif
        0,                                        // Reserved
        PendSV_Handler,                           // The PendSV handler
        SysTick_Handler,                          // The SysTick handler

		/* External interrupts */
		WDT_IRQHandler,                           /*  0:  Watchdog Timer            */
		RTC_IRQHandler,                           /*  1:  Real Time Clock           */
		TIM0_IRQHandler,                          /*  2:  Timer0 / Timer1           */
		TIM2_IRQHandler,                          /*  3:  Timer2 / Timer3           */
		MCIA_IRQHandler,                          /*  4:  MCIa                      */
		MCIB_IRQHandler,                          /*  5:  MCIb                      */
		UART0_IRQHandler,                         /*  6:  UART0 - DUT FPGA          */
		UART1_IRQHandler,                         /*  7:  UART1 - DUT FPGA          */
		UART2_IRQHandler,                         /*  8:  UART2 - DUT FPGA          */
		UART4_IRQHandler,                         /*  9:  UART4 - not connected     */
		AACI_IRQHandler,                          /* 10: AACI / AC97                */
		CLCD_IRQHandler,                          /* 11: CLCD Combined Interrupt    */
		ENET_IRQHandler,                          /* 12: Ethernet                   */
		USBDC_IRQHandler,                         /* 13: USB Device                 */
		USBHC_IRQHandler,                         /* 14: USB Host Controller        */
		CHLCD_IRQHandler,                         /* 15: Character LCD              */
		FLEXRAY_IRQHandler,                       /* 16: Flexray                    */
		CAN_IRQHandler,                           /* 17: CAN                        */
		LIN_IRQHandler,                           /* 18: LIN                        */
		I2C_IRQHandler,                           /* 19: I2C ADC/DAC                */
		0,                                        /* 20: Reserved                   */
		0,                                        /* 21: Reserved                   */
		0,                                        /* 22: Reserved                   */
		0,                                        /* 23: Reserved                   */
		0,                                        /* 24: Reserved                   */
		0,                                        /* 25: Reserved                   */
		0,                                        /* 26: Reserved                   */
		0,                                        /* 27: Reserved                   */
		CPU_CLCD_IRQHandler,                      /* 28: Reserved - CPU FPGA CLCD   */
		0,                                        /* 29: Reserved - CPU FPGA        */
		UART3_IRQHandler,                         /* 30: UART3    - CPU FPGA        */
		SPI_IRQHandler                            /* 31: SPI Touchscreen - CPU FPGA */
};

// Processor ends up here if an unexpected interrupt occurs or a specific
// handler is not present in the application code.
__attribute__ ((section(".after_vectors")))
void Default_Handler (void)
{
	while (1) ;
}
