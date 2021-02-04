
#include "uart.h"

#include <stdio.h>
#include "usrlib.h"
#include "sys.h"

#define UART_TASK_MAX 10 

static void UartRecByteDefault(uint8_t byte);

typedef void (*UartRecByteProcess)(uint8_t byte);
UartRecByteProcess g_recFuncBuf[10];
UartRecByteProcess g_recFunc = UartRecByteDefault;
uint8_t g_cnt = 1; /* 从1开始，0被默认函数占用 */

static void UartRecByteDefault(uint8_t byte __attribute__((unused)))
{

}

int32_t UartFuncRegister(void (*cb)(uint8_t byte))
{
    if ((g_cnt >= UART_TASK_MAX) || (cb == NULL)) {
		return UART_RET_ERROR;
	}

    g_recFuncBuf[g_cnt++] = cb;
    g_recFunc = cb;
    return UART_RET_SUCCESS;
}

int32_t UartFuncUnregister(void (*cb)(uint8_t byte))
{
    uint8_t i;
    if (cb == NULL) {
		return UART_RET_ERROR;
	}
    
    for (i = 1; i < g_cnt; i++) {
        if (cb == g_recFuncBuf[i]) {
            break;
        }   
    }
    if (i == g_cnt) {
    	return UART_RET_ERROR;
    } 
    for (; i < g_cnt - 1; i++) {
        g_recFuncBuf[i] = g_recFuncBuf[i + 1]; 
    }
	g_recFuncBuf[i] = NULL;
    g_cnt--;
    if (g_recFunc == cb) {
        g_recFunc = g_recFuncBuf[i - 1];
    }
    
    return UART_RET_SUCCESS;
}

/* PB10 TX PB11 RX */
void Uart3Init(u32 bound)
{
    //GPIO端口设置
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    memset_s(g_recFuncBuf, 0, sizeof(g_recFuncBuf));
    g_recFuncBuf[0] = UartRecByteDefault;
    g_recFunc = UartRecByteDefault;

    RCC_APB2PeriphClockCmd(RCC_APB1Periph_USART3|RCC_APB2Periph_GPIOB, ENABLE);	//使能USART3，GPIOA时钟

    //USART3_TX   GPIOA.9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; //PA.9
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
    GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化GPIOA.9

    //USART3_RX	  GPIOA.10初始化
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;//PA10
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
    GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化GPIOA.10  

    //USART3 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器

    //USART 初始化设置
    USART_InitStructure.USART_BaudRate = bound;//串口波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
    USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

    USART_Init(USART3, &USART_InitStructure); //初始化串口1
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//开启串口接受中断
    USART_Cmd(USART3, ENABLE);                  //使能串口1
}

void USART3_IRQHandler(void)                	//串口1中断服务程序
{
    if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) { //接收中断(接收到的数据必须是0x0d 0x0a结尾)
        g_recFunc(USART_ReceiveData(USART3));
    } 
}


