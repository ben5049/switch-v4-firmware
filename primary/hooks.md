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


## SystemClock_Config
TBD
The function `SystemClock_Config();` must be exposed in `main.h`.



# Excludes
```
<entry excluding="ST/netxduo/common/drivers/ethernet/nx_stm32_eth_driver.c" flags="VALUE_WORKSPACE_PATH|RESOLVED" kind="sourcePath" name="Middlewares"/>
<entry excluding="Core" flags="VALUE_WORKSPACE_PATH|RESOLVED" kind="sourcePath" name="Application"/>
<entry excluding="Target/nx_stm32_phy_custom_driver.c" flags="VALUE_WORKSPACE_PATH|RESOLVED" kind="sourcePath" name="NetXDuo"/>
<entry excluding="mstp/TestAppTM4C129+88E6352|zenoh-pico/src/system/freertos|mstp/TestAppSTM32+88E6352|mstp/TestAppLPC2387+IP175CD|zenoh-pico/src/system/windows|zenoh-pico/src/system/rpi_pico|mstp/TestAppMK66F+KSZ8794|mstp/simulator|zenoh-pico/src/system/unix|zenoh-pico/examples|zenoh-pico/ci|zenoh-pico/src/system/espidf|zenoh-pico/cmake|zenoh-pico/src/system/zephyr|zenoh-pico/docs|zenoh-pico/src/system/arduino|zenoh-pico/src/system/flipper|zenoh-pico/zephyr|zenoh-pico/src/system/threadx|zenoh-pico/src/system/mbed|zenoh-pico/tools|zenoh-pico/src/system/emscripten|zenoh-pico/tests" flags="VALUE_WORKSPACE_PATH|RESOLVED" kind="sourcePath" name="Libraries"/>
```

# Compiler Defines
```
<listOptionValue builtIn="false" value="HAL_ETH_USE_PTP"/>
<listOptionValue builtIn="false" value="ZENOH_GENERIC"/>
```