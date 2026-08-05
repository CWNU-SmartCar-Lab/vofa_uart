#include "vofa_uart.h"
#include "usart.h" // STM32 HAL 示例；若不用 HAL，在自己的文件中覆盖下面两个函数即可

/**
 * @brief 默认发送实现：STM32 HAL 阻塞式整帧发送
 * @note  弱符号。追求性能时建议覆盖为 HAL_UART_Transmit_DMA / 中断发送
 */
VOFA_WEAK void uartSendData(const uint8_t* data, uint32_t len)
{
	HAL_UART_Transmit(&huart1, (uint8_t*)data, (uint16_t)len, HAL_MAX_DELAY);
}

VOFA_WEAK void uartSendByte(uint8_t c)
{
	uartSendData(&c, 1U);
}
