#include "bsp_usart1.h"
#include <string.h>   
#include <stdarg.h>  
#include "main.h"
#include "FIFO.h"
#include "bsp_uartcomm.h"
#include "print_monitor.h"
#include "stm32f1xx_ll_usart.h"

UART_HandleTypeDef g_uart1Handle = {
    .Instance = USART1,
    .Init.BaudRate = 115200,
    .Init.WordLength = UART_WORDLENGTH_8B,
    .Init.StopBits = UART_STOPBITS_1,
    .Init.Parity = UART_PARITY_NONE,
    .Init.Mode = UART_MODE_TX_RX,
    .Init.HwFlowCtl = UART_HWCONTROL_NONE,
    .Init.OverSampling = UART_OVERSAMPLING_16,
};
UART_PARA_STRUCT g_UARTPara = {
    .periph = USART1,
    .uartHandle = &g_uart1Handle,
    .dmaUsed = true,
    .dmaBusy = false,
};

DMA_HandleTypeDef g_hdma_usart1_tx;

#define UART1_BUFF_SIZE 	(200)
static INT8U g_buffSend[350];
static INT8U g_buffRec[UART1_BUFF_SIZE];

void UART1_init(void)
{
    FIFO_Init(&g_UARTPara.fifo.sfifo, g_buffSend, sizeof(g_buffSend));	
    FIFO_Init(&g_UARTPara.fifo.rfifo, g_buffRec, sizeof(g_buffRec));
	
    com_registHandler(&g_UARTPara);
    if (HAL_UART_Init(&g_uart1Handle) != HAL_OK)
    {
    }

	LL_USART_EnableIT_RXNE(g_uart1Handle.Instance);
    if (g_UARTPara.dmaUsed == false){
        //LL_USART_EnableIT_TC(g_uart1Handle.Instance);
    }
    //__HAL_DMA_ENABLE_IT(&g_hdma_usart1_tx, DMA_IT_TC);
    
    PrintMonitorInit();
}
__weak void HAL_UART_IRQHandler(UART_HandleTypeDef *huart)
{
    uint32_t isrflags = READ_REG(huart->Instance->SR);
	uint32_t cr1its     = READ_REG(huart->Instance->CR1);

    /* If no error occurs */
    /* UART in mode Receiver ---------------------------------------------------*/
    if (((isrflags & USART_SR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
	{
        FIFO_Write(&g_UARTPara.fifo.rfifo, (uint8_t)READ_REG(huart->Instance->DR));
    }
	if (isrflags & USART_SR_TC){
		huart->gState = HAL_UART_STATE_READY;
		PrintMonitor_PostdMsg(g_UARTPara.periph, false);
	}
    __HAL_UART_CLEAR_FLAG(huart, USART_SR_PE | USART_SR_FE | USART_SR_ORE | USART_SR_NE | USART_SR_TC);
}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
    if (!FIFO_Empty(&g_UARTPara.fifo.sfifo)){
        PrintMonitor_PostdMsg(g_UARTPara.periph, false);
    }else{
        g_UARTPara.fifo.status &= ~UART_SENDING;
    }
}

/**
  * @brief  Rx Transfer completed callback
  * @param  UartHandle: UART handle
  * @note   This example shows a simple way to report end of DMA Rx transfer, and 
  *         you can add your own implementation.
  * @retval None
  */
void UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
    uint32_t isrflags = READ_REG(UartHandle->Instance->SR);
	uint32_t cr1its     = READ_REG(UartHandle->Instance->CR1);

    /* If no error occurs */
    /* UART in mode Receiver ---------------------------------------------------*/
    if (((isrflags & USART_SR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
	{
        FIFO_Write(&g_UARTPara.fifo.rfifo, (uint8_t)READ_REG(UartHandle->Instance->DR));
    }
}
