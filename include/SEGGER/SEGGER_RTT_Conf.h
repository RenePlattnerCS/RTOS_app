/*********************************************************************
 *                    SEGGER Microcontroller GmbH                     *
 *       SEGGER RTT configuration for STM32F407 / FreeRTOS            *
 **********************************************************************
 */
#ifndef SEGGER_RTT_CONF_H
#define SEGGER_RTT_CONF_H

// Number of up-buffers (target → host).
// Must be >= 2: buffer 0 for terminal, buffer 1 for SystemView.
#define SEGGER_RTT_MAX_NUM_UP_BUFFERS 2

// Number of down-buffers (host → target).
#define SEGGER_RTT_MAX_NUM_DOWN_BUFFERS 2

// Default size of buffer 0 (terminal printf channel).
#define SEGGER_RTT_BUFFER_SIZE_UP   (4 * 1024 * 2)
#define SEGGER_RTT_BUFFER_SIZE_DOWN 16

// Mode for buffer 0: block if full (safest for debug output).
#define SEGGER_RTT_MODE_DEFAULT SEGGER_RTT_MODE_NO_BLOCK_SKIP

// Place RTT control block in a fixed RAM section so OpenOCD can find it.
#define SEGGER_RTT_SECTION ".noinit"

// Locking: use BASEPRI on Cortex-M4 to allow ISR-safe RTT writes.
#define SEGGER_RTT_LOCK()                    \
    {                                        \
        unsigned int _SEGGER_RTT__LockState; \
        __asm volatile("mrs %0, primask\n\t cpsid i" : "=r"(_SEGGER_RTT__LockState) : : "memory");
#define SEGGER_RTT_UNLOCK()                                                       \
    __asm volatile("msr primask, %0" : : "r"(_SEGGER_RTT__LockState) : "memory"); \
    }

#endif /* SEGGER_RTT_CONF_H */
