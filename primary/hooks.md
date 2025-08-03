# Hooks

Almost all code is contained in the Application and Libraries folders, however certain functions must interact with auto-generated code. This page lists all such scenarios so they can be reimplemented if lost due to re-auto-generation. 

## Set MAC address

## ThreadX init

## STP Release packet callback:

In [`primary\Middlewares\ST\netxduo\common\drivers\ethernet\nx_stm32_eth_driver.c`](primary\Middlewares\ST\netxduo\common\drivers\ethernet\nx_stm32_eth_driver.c):

```C

#include "stp_extra.h"

...

void HAL_ETH_TxCpltCallback(ETH_HandleTypeDef *heth){

    if (__atomic_load_n(&bpdu_transmitted, __ATOMIC_ACQUIRE)){
        if (stp_ReleaseTxPacket(heth)){
            // TODO: Notify STP thread
            return;
        }
    }

    ... // NetXduo process its packet complete
}

...
```