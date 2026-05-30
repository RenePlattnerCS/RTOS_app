#include "logger/itm.h"

void ITM_Init(void)
{
    // Enable the TPIU (Trace Port Interface Unit)
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    // Unlock ITM
    ITM->LAR = 0xC5ACCE55;

    // Enable ITM and set sync packets
    ITM->TCR = ITM_TCR_ITMENA_Msk | ITM_TCR_SYNCENA_Msk | (1 << ITM_TCR_TraceBusID_Pos);

    // Enable stimulus port 0 for printf output
    ITM->TER = (1 << 0);
}

/* Override _write to route printf/fwrite to ITM stimulus port 0 */
int _write(int file, char *ptr, int len)
{
    int i = 0;
    for (i = 0; i < len; i++)
    {
        ITM_SendChar((uint32_t) *ptr++);
    }
    return len;
}