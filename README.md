# 使用方法

> 写在前面: 作者是将两年前的代码修补了一下并开源出来，如果有问题可以issue，移植教程还是比较详细，仔细阅读下列教程基本上没什么问题。

---

- [使用方法](#使用方法)
	- [一、对外接口](#一对外接口)
	- [二、移植接口](#二移植接口)
	- [三、简单的使用样例](#三简单的使用样例)
		- [3.1 串口中断加入接收函数](#31-串口中断加入接收函数)
		- [3.2 主循环判断接收标志位](#32-主循环判断接收标志位)
	- [四、如何定制自己的命令帧？](#四如何定制自己的命令帧)
	- [五、数据大小端的切换](#五数据大小端的切换)
	- [六、性能建议](#六性能建议)
	- [七、VOFA命令的配置](#七vofa命令的配置)
		- [7.1 新建命令](#71-新建命令)
		- [7.2 根据你的命令帧编辑命令](#72-根据你的命令帧编辑命令)
		- [7.3 将命令绑定到控件](#73-将命令绑定到控件)
	- [八、旧版本迁移说明](#八旧版本迁移说明)

---



本仓库主要是用于串口调参的工具，使用`vofa`软件的`Justfloat`协议以及命令帧解析，可自定义响应帧。

![image-vofa](./images/vofa.png)



## 一、对外接口

- **./Inc/vofa_function.h**

```C
void vofaInit(void);											//库初始化

void vofaSendJustFloat(const float *fdata, uint8_t chCount);	//以JustFloat协议发送数据（1~CH_COUNT个通道）
void vofaSendFirewater(const float *fdata, uint32_t ulSize);	//以Firewater协议发送数据
void vofaSendRawdata(const uint8_t *pData, uint32_t ulSize);	//以rawdata协议发送数据

void uartCMDRecv(uint8_t byte_data);							//uart串口接收单字节并存入vofaCommandData数据包
vofaParseResult vofaCommandParse(void);							//解析命令，返回解析结果

extern volatile vofaCommand vofaCommandData;					//包含命令的结构体
```

发送接口示例：

```C
float chData[2] = {1.0f, -2.5f};

vofaSendJustFloat(chData, 2);	//2个通道
vofaSendFirewater(chData, 2);
```

`vofaCommandParse` 的返回值用于区分解析结果：

```C
typedef enum
{
	VOFA_PARSE_OK = 0,
	VOFA_PARSE_BAD_FRAME,	 //帧头/帧尾/格式错误
	VOFA_PARSE_UNKNOWN_TYPE, //未知命令类型
	VOFA_PARSE_UNKNOWN_ID	 //未知命令ID
} vofaParseResult;
```

## 二、移植接口

串口发送函数在 `./Src/vofa_uart.c` 中是**弱符号（weak）**的默认实现，移植时**不需要修改本库任何源码**，直接在你自己的工程里定义同名函数即可覆盖：

```C
/* 在你自己的源文件中实现即可，例如基于 DMA 的发送 */
void uartSendData(const uint8_t *data, uint32_t len)
{
	HAL_UART_Transmit_DMA(&huart1, (uint8_t *)data, (uint16_t)len);
}
```

```C
void uartSendByte(uint8_t c)
{
	uartSendData(&c, 1);
}
```

- **./Inc/vofa_function.h**

  ```C
  void uartCMDRecv(uint8_t byte_data);		//将此函数放在串口接收中断中调用
  ```

  

## 三、简单的使用样例

### 3.1 串口中断加入接收函数

```C
#include "vofa_function.h"
#include "usart.h"

static uint8_t rx_data = 0;

/* 以 STM32 HAL 为例 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART1)
	{
		uartCMDRecv(rx_data);
		HAL_UART_Receive_IT(&huart1, &rx_data, 1);
	}
}
```

### 3.2 主循环判断接收标志位

```C
#include "vofa_function.h"

/*以主函数中的循环加延时检测为例*/
int main(void)
{
	/*
	*	一些初始化代码
	*/
	vofaInit();

	for(;;)
	{
		if (vofaCommandData.completionFlag == 1)	//收到命令帧
		{
			vofaCommandData.completionFlag = 0;		//清除标志位

			if (vofaCommandParse() == VOFA_PARSE_OK)//解析成功
			{
				/*
				* 根据你的命令做相应处理
				* vofaCommandData.cmdID是接收到的命令ID
				* vofaCommandData.cmdType是接收到的命令类型
				* vofaCommandData.floatData是接收到的浮点数据
				*/

				//使用举例
				switch(vofaCommandData.cmdType)		//判断收到的命令类型
				{
					case Speed:
						switch(vofaCommandData.cmdID)	//判断收到的命令ID
						{
							case Direct_Assignment:
								printf("I recv Command Type Speed, ID Direct_Assignment, data: %.6f", vofaCommandData.floatData);
								break;
							case Increase:
								printf("I recv Command Type Speed, ID Increase, data: %.6f", vofaCommandData.floatData);
								break;
							case Decrease:
								printf("I recv Command Type Speed, ID Decrease, data: %.6f", vofaCommandData.floatData);
								break;
						}
						break;
					case Position:
						switch(vofaCommandData.cmdID)	//判断收到的命令ID
						{
							case Direct_Assignment:
								printf("I recv Command Type Position, ID Direct_Assignment, data: %.6f", vofaCommandData.floatData);
								break;
							case Increase:
								printf("I recv Command Type Position, ID Increase, data: %.6f", vofaCommandData.floatData);
								break;
							case Decrease:
								printf("I recv Command Type Position, ID Decrease, data: %.6f", vofaCommandData.floatData);
								break;
						}
						break;
				}
			}
		}
		osDelay(50);	//延时50ms
	}
}
```

> 注意：`vofaCommandData` 在串口中断与主循环间共享，已声明为 `volatile`。
> 默认流程是先整体拷贝到栈上再解析，常规场景无需额外加锁；
> 若对实时性要求高，建议改用 DMA + 空闲中断接收（见第六节）。

## 四、如何定制自己的命令帧？

命令帧相关的可配置项：

- **./Inc/vofa_function.h**

```C
#define CMD_FRAME_SIZE 10			//将此处更改为你需要的命令帧长度
```

**修改自己的命令类型和ID**

```C
//在此处枚举你需要的命令ID
enum CommandID
{
	Direct_Assignment,
	Increase,
	Decrease
};
//在此处枚举你需要的命令类型
enum CommandType
{
	Speed,
	Position
};
```

`vofaCommandParse` 是弱符号函数，定制解析逻辑时**不需要修改 `./Src/vofa_function.c`**，
直接在你自己的工程里定义同名函数即可覆盖默认实现。默认实现如下，可作为自定义的参考模板：

```C
vofaParseResult vofaCommandParse(void)
{
	uint8_t         packet[CMD_FRAME_SIZE];
	vofaParseResult result = VOFA_PARSE_OK;

	/* 先整体拷到栈上再解析，避免解析过程中被中断改写 */
	memcpy(packet, (const void *)vofaCommandData.uartRxPacket, CMD_FRAME_SIZE);
	memset((void *)vofaCommandData.uartRxPacket, 0, CMD_FRAME_SIZE);

	//帧格式判断		默认命令帧以 @+S/P+1/2/3+=+四字节浮点数据+!+# 为例
	//其中@是帧头 S/P对应命令类型 1/2/3对应命令ID !#为帧尾
	if (packet[0] != '@' || packet[3] != '=' ||
		packet[CMD_FRAME_SIZE - 2] != '!' || packet[CMD_FRAME_SIZE - 1] != '#')
	{
		return VOFA_PARSE_BAD_FRAME;
	}

	//此处修改字节比对，改为你需要的类型
	switch (packet[1])
	{
		case 'S': vofaCommandData.cmdType = Speed;
			break;
		case 'P': vofaCommandData.cmdType = Position;
			break;
		default: vofaCommandData.cmdType = INVALID;
			result = VOFA_PARSE_UNKNOWN_TYPE;
			break;
	}
	//此处修改字节比对，改为你需要的ID
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
```

> 注意：`uartCMDRecv` 默认按 `!#` 帧尾判定一帧接收完成，定制帧格式时若帧尾不同，
> 请同步调整 `uartCMDRecv` 中的帧尾比对。

## 五、数据大小端的切换

`JustFloat`处理浮点数是按照IEEE的标准，使用四个字节来标识float类型数据，那么这时候就存在一个问题，就是需要清楚数据在你的RAM或ROM中存储的方式是[大端存储(Big Endian)](https://baike.baidu.com/item/%E5%A4%A7%E7%AB%AF%E6%A0%BC%E5%BC%8F/6665553?fromModule=search-result_lemma)还是[小端存储(Little Endian)](https://baike.baidu.com/item/%E5%B0%8F%E7%AB%AF%E5%AD%98%E5%82%A8/3432023)，以便转换函数按照正确的格式转换你的数据。

```C
//使用小端(Little-endian)的架构：
//最常用x86架构(包括x86_64)，还有 6502 (including 65802, 65C816), ARM Cortex, Z80 (including Z180, eZ80 etc.), MCS-48, 8051, DEC Alpha, Altera Nios, Atmel AVR, SuperH, VAX, 和 PDP-11 等等；

//使用大端(Big-endian)的架构：
// Motorola 6800 and 68k, Xilinx Microblaze, IBM POWER, system/360, System/370 等等。

//支持配置endian的架构：
//如下架构有配置endian为大端、小端中任一种的功能， ARM, PowerPC, Alpha, SPARC V9, MIPS, PA-RISC 和 IA-64 等等。
```

需要在 `base_transfer.h`中通过宏定义进行配置:

```C
#ifndef BASE_TRANSFER_H__
#define BASE_TRANSFER_H__

#define USE_BIG_ENDIAN		0		//启用大端存储格式
#define USE_LITTLE_ENDIAN	1		//启用小端存储格式

#include <stdint.h>
#include <string.h>

float uint8Array2Float(const uint8_t* u8Array);
void float2uint8Array(uint8_t* u8Array, const float* fdata);

#endif
```

样例代码由于是在`STM32`上测试的，是`ARM Cortex`架构，故将`#define USE_LITTLE_ENDIAN`配置为了`1`，以启用小端存储格式解析。

> 小端平台下 JustFloat 发送走的是 `memcpy` 零转换路径，大小端配置只影响大端平台。

## 六、性能建议

- **发送**：`vofaSendJustFloat` / `vofaSendRawdata` 默认已拼好整帧后调用一次 `uartSendData`。
  默认移植实现是 HAL 阻塞发送，高频发送场景建议把 `uartSendData` 覆盖为 DMA 发送（见第二节），CPU 占用可降到接近零。
- **Firewater 协议**：`vofaSendFirewater` 依赖浮点 `printf`，在 MCU 上会增加约 10~20KB flash 且耗时较长，仅建议低频调试使用；高频波形请使用 JustFloat。
- **接收**：默认是每字节中断 + 主循环轮询标志位，命令响应延迟取决于你的轮询周期。
  对响应速度有要求时，建议改用 UART 空闲中断 + DMA 接收，收到一整帧后再置位解析。

## 七、VOFA命令的配置

### 7.1 新建命令

![vofa-new_cmd](./images/vofa_new_cmd.png)

### 7.2 根据你的命令帧编辑命令

![vofa-edit-cmd](./images/vofa_edit_cmd.png)

注意编辑完成后一定要切换为HEX模式保存

![vofa-save-with-hex](./images/vofa_save_with_hex.png)

### 7.3 将命令绑定到控件

在控件上右键单击，选择`绑定命令`，选择我们刚刚保存的命令即可完成绑定

![vofa_bond_cmd](./images/vofa_bond_cmd.png)

## 八、旧版本迁移说明

如果你使用的是旧版本接口，请按以下对照修改：

| 旧接口 | 新接口 |
|--------|--------|
| `vofaJustFloatInit()` | `vofaInit()` |
| `vofaSendJustFloat(&JustFloat_Data)`（结构体方式） | `vofaSendJustFloat(fdata数组, 通道数)` |
| `vofaCommandParse()` 返回 `void` | 返回 `vofaParseResult`，可判断解析是否成功 |
| 修改 `./Src/vofa_uart.c` 移植串口发送 | 在自己的工程里定义 `uartSendData` / `uartSendByte` 覆盖弱符号 |
| 修改 `./Src/vofa_function.c` 定制命令帧 | 在自己的工程里定义 `vofaCommandParse` 覆盖弱符号 |

其他变化：

- 全局变量 `JustFloat_Data`、结构体 `vofaJustFloatFrame`、`vofaRxBufferIndex` 已不再对外暴露；
- `vofaCommand` 结构体中的 `validData` 成员已移除（解析结果直接写入 `floatData`）；
- `vofaCommandData` 现在声明为 `volatile`。
