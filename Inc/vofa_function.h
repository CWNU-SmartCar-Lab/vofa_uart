#ifndef VOFA_FUNCTION_H__
#define VOFA_FUNCTION_H__

#define INVALID 0xFF
#define FRAME_TAIL_SIZE (4U)
#define CH_COUNT (8U)		//支持的最大通道数量
#define CMD_FRAME_SIZE 10	//命令帧长度，按需修改

#include <stdint.h>
#include "vofa_uart.h"

enum CommandID
{
	Direct_Assignment,
	Increase,
	Decrease
};

enum CommandType
{
	Speed,
	Position
};

/* 命令帧解析结果 */
typedef enum
{
	VOFA_PARSE_OK = 0,
	VOFA_PARSE_BAD_FRAME,	 //帧头/帧尾/格式错误
	VOFA_PARSE_UNKNOWN_TYPE, //未知命令类型
	VOFA_PARSE_UNKNOWN_ID	 //未知命令ID
} vofaParseResult;

typedef struct vofaCommand
{
	uint8_t cmdType;
	uint8_t cmdID;
	uint8_t uartRxPacket[CMD_FRAME_SIZE]; //串口数据包接收数组
	uint8_t completionFlag;
	float   floatData;
} vofaCommand;

#ifdef __cplusplus
extern "C" {
#endif

void vofaInit(void); //库初始化（命令结构体清零）

void vofaSendJustFloat(const float* fdata, uint8_t chCount);  //以JustFloat协议发送数据
void vofaSendFirewater(const float* fdata, uint32_t ulSize);  //以Firewater协议发送数据
void vofaSendRawdata(const uint8_t* pData, uint32_t ulSize);  //以rawdata协议发送数据

void uartCMDRecv(uint8_t byte_data); //uart串口接收单字节并存入vofaCommandData数据包，放在串口接收中断中调用

/* 弱符号：需要自定义命令帧时，在自己的工程里定义同名函数覆盖即可，无需修改本库源码 */
VOFA_WEAK vofaParseResult vofaCommandParse(void); //解析命令

/* 在串口中断与主循环间共享，故声明为 volatile */
extern volatile vofaCommand vofaCommandData; //包含命令的结构体

#ifdef __cplusplus
}
#endif

#endif
