#ifdef SYSVIEW_ENABLE
#include "SEGGER_SYSVIEW_REC.h"
#include "stm32f4xx_hal.h"

extern UART_HandleTypeDef huart2;

static volatile uint8_t _rxByte;
static volatile uint8_t _txByte;
static volatile int     _txBusy = 0;

static void _TryStartTx(void)
{
    int n = SYSVIEW_REC_GetOutgoing(&_txByte, 1);
    if (n > 0) {
        _txBusy = 1;
        HAL_UART_Transmit_IT(&huart2, (uint8_t *)&_txByte, 1);
    } else {
        _txBusy = 0;
    }
}

// Called by SEGGER_SYSVIEW_Conf.h macro on every recorded event — kicks TX.
void SEGGER_SYSVIEW_X_OnEventRecorded(unsigned NumBytes)
{
    (void)NumBytes;
    if (_txBusy == 0) {
        _TryStartTx();
    }
}

// TX complete — immediately send next byte if available.
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        _TryStartTx();
    }
}

// RX complete — feed byte into REC state machine, send response if handshake step done.
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        int n = SYSVIEW_REC_ProcessIncoming(&_rxByte, 1);
        if (n > 0 && _txBusy == 0) {
            _TryStartTx();
        }
        HAL_UART_Receive_IT(&huart2, (uint8_t *)&_rxByte, 1);
    }
}

// Call after MX_USART2_UART_Init() — arms the receive interrupt.
void SYSVIEW_UART_Config(void)
{
    HAL_UART_Receive_IT(&huart2, (uint8_t *)&_rxByte, 1);
}

#endif /* SYSVIEW_ENABLE */
