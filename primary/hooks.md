# Hooks

Almost all code is contained in the Application and Libraries folders, however certain functions must interact with auto-generated code. This page lists all such scenarios so they can be reimplemented if lost due to re-auto-generation. 

## Set MAC address

## ThreadX init

## NetX init

```C
#include "nx_app.h"

...

UINT MX_NetXDuo_Init(VOID *memory_ptr) {

    ...

    /* USER CODE BEGIN 0 */
    ret = nx_user_init(memory_ptr);
    /* USER CODE END 0 */

    ...

    return ret;
}
```

## Ethernet TIM Trigger Remap

```C
static void MX_TIM2_Init(void) {

    ...

    /* USER CODE BEGIN TIM2_Init 2 */
    HAL_TIMEx_RemapConfig(&htim2, TIM_TIM2_ETR_ETH_PPS);
    /* USER CODE END TIM2_Init 2 */

    ...
}
```