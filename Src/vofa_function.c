#include "vofa_function.h"
#include <stdio.h>
#include <string.h>
#include "base_transfer.h"

volatile vofaCommand vofaCommandData;

/* JustFloat 协议帧尾 0x00 0x00 0x80 0x7F */
static const uint8_t justFloatTail[FRAME_TAIL_SIZE] = {0x00, 0x00, 0x80, 0x7F};

/* 接收索引仅本文件使用，不对外暴露；中断中访问故为 volatile */
static volatile uint8_t vofaRxBufferIndex = 0;

/**
* @brief 初始化库（清空命令结构体）
*/
void vofaInit(void)
{
	vofaCommandData.cmdID          = INVALID;
	vofaCommandData.cmdType        = INVALID;
	vofaCommandData.completionFlag = 0U;
	vofaCommandData.floatData      = 0.0f;
	memset((void*)vofaCommandData.uartRxPacket, 0, CMD_FRAME_SIZE);
	vofaRxBufferIndex = 0;
}

/**
* @brief 以JustFloat协议发送数据（拼好整帧后一次发送）
* @param fdata: 指向要发送的浮点数组
* @param chCount: 通道数量（1 ~ CH_COUNT）
*/
void vofaSendJustFloat(const float* fdata, uint8_t chCount)
{
	uint8_t frame[CH_COUNT * 4U + FRAME_TAIL_SIZE];

	if (fdata == NULL || chCount == 0U || chCount > CH_COUNT)
	{
		return;
	}

#if USE_BIG_ENDIAN == 1
	for (uint8_t i = 0; i < chCount; i++)
	{
		float2uint8Array(frame + (uint32_t)i * 4U, &fdata[i]);
	}
#else
	/* 小端平台 float 内存布局与 JustFloat 一致，直接拷贝即可 */
	memcpy(frame, fdata, (uint32_t)chCount * 4U);
#endif

	memcpy(frame + (uint32_t)chCount * 4U, justFloatTail, FRAME_TAIL_SIZE);
	uartSendData(frame, (uint32_t)chCount * 4U + FRAME_TAIL_SIZE);
}

/**
* @brief 以Firewater协议发送数据
* @param fdata: 指向要发送的浮点数据的指针
* @param ulSize： 要发送的数据个数
* @note  浮点 printf 在 MCU 上 flash/耗时开销大，高频场景请用 JustFloat
*/
void vofaSendFirewater(const float* fdata, uint32_t ulSize)
{
	uint32_t i;

	if (fdata == NULL || ulSize == 0U)
	{
		return;
	}

	for (i = 0; i < ulSize - 1U; i++)
	{
		printf("%.6f,", fdata[i]);
	}
	printf("%.6f\n", fdata[i]);
}

/**
* @brief 以rawdata协议发送数据
* @param pData: 指向要发送的单字节数据的指针
* @param ulSize： 要发送的数据个数
*/
void vofaSendRawdata(const uint8_t* pData, uint32_t ulSize)
{
	if (pData == NULL || ulSize == 0U)
	{
		return;
	}
	uartSendData(pData, ulSize);
}

/**
* @brief 将串口收到的数据判断并存入数据包中，并比对帧控制接收完成标志位置位
* @param byte_data： 串口接收到的字节数据
*/
void uartCMDRecv(uint8_t byte_data) //此函数放在串口中断中
{
	/* 先判满再写入，防止越界 */
	if (vofaRxBufferIndex >= CMD_FRAME_SIZE)
	{
		vofaRxBufferIndex              = 0;
		vofaCommandData.completionFlag = 0U;
		memset((void*)vofaCommandData.uartRxPacket, 0, CMD_FRAME_SIZE);
	}

	vofaCommandData.uartRxPacket[vofaRxBufferIndex] = byte_data;

	/* index 为 0 时不与前一字节比较，避免越界读 */
	if (vofaRxBufferIndex > 0U &&
		vofaCommandData.uartRxPacket[vofaRxBufferIndex - 1U] == (uint8_t)'!' &&
		byte_data == (uint8_t)'#')
	{
		vofaCommandData.completionFlag = 1U;
		vofaRxBufferIndex              = 0;
	}
	else
	{
		vofaRxBufferIndex++;
	}
}

/**
* @brief vofa命令帧解析
* @return vofaParseResult 解析结果
* @note  弱符号：自定义命令帧时在自己的工程里定义同名函数覆盖即可。
*        帧格式默认 @ + S/P + 1/2/3 + = + 四字节浮点数据 + ! + #
*/
VOFA_WEAK vofaParseResult vofaCommandParse(void)
{
	uint8_t         packet[CMD_FRAME_SIZE];
	vofaParseResult result = VOFA_PARSE_OK;

	/* 先整体拷到栈上再解析，避免解析过程中被中断改写 */
	memcpy(packet, (const void*)vofaCommandData.uartRxPacket, CMD_FRAME_SIZE);
	memset((void*)vofaCommandData.uartRxPacket, 0, CMD_FRAME_SIZE);

	if (packet[0] != (uint8_t)'@' || packet[3] != (uint8_t)'=' ||
		packet[CMD_FRAME_SIZE - 2U] != (uint8_t)'!' || packet[CMD_FRAME_SIZE - 1U] != (uint8_t)'#')
	{
		return VOFA_PARSE_BAD_FRAME;
	}

	switch (packet[1])
	{
		case 'S': vofaCommandData.cmdType = Speed;
			break;
		case 'P': vofaCommandData.cmdType = Position;
			break;
		default: vofaCommandData.cmdType = INVALID;
			result                       = VOFA_PARSE_UNKNOWN_TYPE;
			break;
	}

	switch (packet[2])
	{
		case '1': vofaCommandData.cmdID = Direct_Assignment;
			break;
		case '2': vofaCommandData.cmdID = Increase;
			break;
		case '3': vofaCommandData.cmdID = Decrease;
			break;
		default: vofaCommandData.cmdID = INVALID;
			if (result == VOFA_PARSE_OK)
			{
				result = VOFA_PARSE_UNKNOWN_ID;
			}
			break;
	}

	vofaCommandData.floatData = uint8Array2Float(&packet[4]);
	return result;
}
