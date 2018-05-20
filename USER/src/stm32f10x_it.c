/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTI

  AL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"
#include "includes.h"
/** @addtogroup STM32F10x_StdPeriph_Template
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

u8 g_rx_buf[1024]; // 使用全局的数据缓存
u8 g_num = 0;

// 串口中断服务函数
// 把接收到的数据存在一个数组缓冲区里面，当接收到的的值等于">"时，把值返回
void macUSART1_IRQHandler( void )
{
    if ( USART_GetITStatus( macUSART1, USART_IT_RXNE ) != RESET )
    {
        g_rx_buf[g_num] = USART_ReceiveData( macUSART1 );

        if ( g_rx_buf[g_num] == FRAME_START )
        {
            memset( g_rx_buf, 0, sizeof( g_rx_buf ) );
            g_rx_buf[0] = FRAME_START;
            g_num = 0;
            g_num++;
        }
        else if ( g_rx_buf[g_num] == FRAME_END ) // 当接收到的值等于'>'时，结束一帧数据
        {

            if(POSITIVE_ACK == g_rx_buf[2])      // 如果是正应答帧,不压栈
            {
                if ( g_rx_buf[1] == g_uiCurNum ) //卡取走信息序号判断,如果序号
                {
                    g_uiCurNum = 0;
                    g_siCardTakeMsgTime = 0;// 不再重发
                    g_siStatusOverTimeS = 0;
                    g_ucaDeviceStatus[g_ucCurOutCardId - 1] = 0;
                }
                g_num = 0;
                g_rx_buf[g_num] = 0;
            }
            else if (NAGATIVE_ACK == g_rx_buf[2])
            {
                g_num = 0;
                g_rx_buf[g_num] = 0;
            }
            else
            {

                // 禁止串口接收中断
                USART_ITConfig(macUSART1, USART_IT_RXNE, DISABLE);
                g_tP_RsctlFrame.RSCTL = g_rx_buf[1];
                printf ("%s\n",(char *)&g_tP_RsctlFrame);   //发送正应答帧
                // 使能串口接收中断
                USART_ITConfig(macUSART1, USART_IT_RXNE, ENABLE);

                g_rx_buf[g_num + 1] = 0;    // 加上行尾标识符
                /* 发布消息到消息队列 queue */
                uartInQueue( &g_tUARTRxQueue, g_rx_buf ); // 不考虑竞争,所以不设置自旋锁
            }
        }

        // 当值不等时候，则继续接收下一个
        else
        {

            g_num++;

            if ( g_num > 50 ) //一帧数据最大50字节,超出则丢弃
            {

                // 禁止串口接收中断
                USART_ITConfig(macUSART1, USART_IT_RXNE, DISABLE);
                g_tN_RsctlFrame.RSCTL = g_rx_buf[1];
                printf ("%s\n",(char *)&g_tN_RsctlFrame);   //发送负应答帧
                // 使能串口接收中断
                USART_ITConfig(macUSART1, USART_IT_RXNE, ENABLE);

                g_num = 0;
                g_rx_buf[g_num] = 0;
            }
        }
    }
}

#if uart4

// 串口中断服务函数
// 把接收到的数据存在一个数组缓冲区里面，当接收到的的值等于">"时，把值返回
void macUSART4_IRQHandler( void )
{
    if ( USART_GetITStatus( macUSART4, USART_IT_RXNE ) != RESET )
    {
        g_rx_buf[g_num] = USART_ReceiveData( macUSART4 );

        if ( g_rx_buf[g_num] == FRAME_START )
        {
            memset( g_rx_buf, 0, sizeof( g_rx_buf ) );
            g_rx_buf[0] = FRAME_START;
            g_num = 0;
            g_num++;
        }
        else if ( g_rx_buf[g_num] == FRAME_END ) // 当接收到的值等于'>'时，结束一帧数据
        {
            g_rx_buf[g_num + 1] = 0;    // 加上行尾标识符

            /* 发布消息到消息队列 queue */
            uartInitQueue( &g_tUARTRxQueue, g_rx_buf ); // 不考虑竞争,所以不设置自旋锁
        }

        // 当值不等时候，则继续接收下一个
        else
        {
            g_num++;

            if ( g_num > 50 ) //一帧数据最大50字节,超出则丢弃
            {
                g_num = 0;
            }
        }
    }
}
#endif

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
