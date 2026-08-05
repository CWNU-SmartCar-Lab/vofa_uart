#ifndef VOFA_UART_H__
#define VOFA_UART_H__

#include <stdint.h>

/* 弱符号宏：GCC 用 __attribute__((weak))，Keil/IAR 用 __weak */
#if defined(__GNUC__)
#define VOFA_WEAK __attribute__((weak))
#elif defined(__ICCARM__) || defined(__CC_ARM) || defined(__ARMCC_VERSION)
#define VOFA_WEAK __weak
#else
#define VOFA_WEAK
#endif

/**
 * 串口发送移植接口（弱符号）。
 * 移植时无需修改本库源码，在自己的工程里定义同名函数覆盖即可。
 * 默认实现见 Src/vofa_uart.c（STM32 HAL 阻塞发送，建议覆盖为 DMA 发送）。
 */
VOFA_WEAK void uartSendByte(uint8_t c);
VOFA_WEAK void uartSendData(const uint8_t* data, uint32_t len);

#endif
