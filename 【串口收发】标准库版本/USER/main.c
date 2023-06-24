#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"

#define FRAME_HEADER 0x01
#define FRAME_TAIL 0x02

int receive_flag = 0; //串口接收完成标志位  0：未完成  1：已完成
u8 Receive_Data[8];
void uart_init(u32 bound)
{
  //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
  
	//USART1_TX   GPIOA.9
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
  GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA.9
   
  //USART1_RX	  GPIOA.10初始化
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;//PA10
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
  GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA.10  

  //Usart1 NVIC 配置
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
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

  USART_Init(USART1, &USART_InitStructure); //初始化串口1
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启串口接受中断
  USART_Cmd(USART1, ENABLE);                    //使能串口1 
}


int main(void)
{	
	delay_init();	    //延时函数初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	uart_init(115200);	 //串口初始化为115200	
	
//	USART_SendData(USART1, 0x11);
//	while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束

	while(1)
	{
		if(receive_flag == 1)
		{
				for(int i=0;i<8;i++)
				{
					USART_SendData(USART1, Receive_Data[i]);//向串口1发送数据
					while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束				
				}	
				receive_flag = 0;			
		}
	}		
}


void USART1_IRQHandler(void)
{	
	static u8 Count=0;  //定义计数静态变量
	u8 Usart_Receive;   //定义中间传递变量

	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) //判断是否接收到数据
	{
			Usart_Receive = USART_ReceiveData(USART1); //读取数据
		
			if(receive_flag == 0)
			{
					//串口数据填入数组
					Receive_Data[Count]=Usart_Receive;
					
					//确保数组第一个数据为FRAME_HEADER（帧头）
					if(Usart_Receive == FRAME_HEADER||Count>0) 
						Count++; 
					else 
						Count=0;
					
					if (Count == 8) //验证数据包的长度
					{   
						Count=0; //为串口数据重新填入数组做准备
						
						if(Receive_Data[7] == FRAME_TAIL) //验证数据包的帧尾
						{
							//接收到完整的一帧数据之后，在这里面进行处理
							receive_flag = 1;												
						}
					}					
			}			
	}
}




