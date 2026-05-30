#ifndef ITM_H
#define ITM_H

#include "main.h"

void ITM_Init(void);

/* Non-blocking ITM character send - never blocks */
static inline int ITM_SendChar_NonBlocking(int ch)
{
    if (((ITM->TCR & ITM_TCR_ITMENA_Msk) != 0UL) && ((ITM->TER & 1UL) != 0UL) && (ITM->PORT[0U].u32 != 0UL))
    {
        ITM->PORT[0U].u8 = (uint8_t) ch;
    }
    return ch;
}

#endif