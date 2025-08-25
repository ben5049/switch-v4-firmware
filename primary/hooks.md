# Hooks

## Non-Secure

Almost all code is contained in the Application and Libraries folders, however certain functions must interact with auto-generated code. This page lists all such scenarios so they can be reimplemented if lost due to re-auto-generation. 

### Set MAC address

### ThreadX init

### NetX init

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

## Secure

### Main

#include "boot_main.h"

boot_main();

### Keys

[primary/Secure/Core/Src/aes.c](primary/Secure/Core/Src/aes.c):

```C
/* USER CODE BEGIN 0 */
#include "secrets.h"
/* USER CODE END 0 */

void MX_SAES_AES_Init(void) {

    /* USER CODE BEGIN SAES_Init 0 */
    set_saes_key(pKeySAES);
    set_saes_init_vector(pInitVectSAES);
    /* USER CODE END SAES_Init 0 */

    ...
}
```