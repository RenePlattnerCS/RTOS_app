/*********************************************************************
 *                    SEGGER Microcontroller GmbH                     *
 *       SEGGER SystemView configuration for STM32F407 / FreeRTOS     *
 **********************************************************************
 */
#ifndef SEGGER_SYSVIEW_CONF_H
#define SEGGER_SYSVIEW_CONF_H

// RTT channel 1: leaves channel 0 free for ITM/printf terminal.
#define SEGGER_SYSVIEW_RTT_CHANNEL 1

// RTT buffer size for SystemView events (bytes).
// Increase if events are dropped under high task/ISR load.
#define SEGGER_SYSVIEW_RTT_BUFFER_SIZE (4096 * 2)

// Lowest RAM address on STM32F407 (start of SRAM1).
// Used to compress task/object IDs — set to actual RAM base.
#define SEGGER_SYSVIEW_ID_BASE 0x20000000

// Shift pointer IDs right by 2 (all FreeRTOS objects are 4-byte aligned).
#define SEGGER_SYSVIEW_ID_SHIFT 2

// GET_TIMESTAMP and GET_INTERRUPT_ID are auto-provided by ConfDefaults.h
// for Cortex-M4 (ARMv7-EM): DWT->CYCCNT and ICSR[8:0] respectively.
// You MUST enable the DWT counter before SEGGER_SYSVIEW_Conf() is called:
//
//   CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
//   DWT->CYCCNT       = 0;
//   DWT->CTRL        |= DWT_CTRL_CYCCNTENA_Msk;

// UART transport: notify segger_uart.c to kick the TX whenever an event is recorded.
// Only active in debug builds (SYSVIEW_ENABLE defined by CMake for Debug config).
#ifdef SYSVIEW_ENABLE
void SEGGER_SYSVIEW_X_OnEventRecorded(unsigned NumBytes);
#define SEGGER_SYSVIEW_ON_EVENT_RECORDED(NumBytes) SEGGER_SYSVIEW_X_OnEventRecorded(NumBytes)
#endif

#endif /* SEGGER_SYSVIEW_CONF_H */
