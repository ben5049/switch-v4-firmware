
# Vector Tables

## Non-secure Vector Table

The macro `VTOR_TABLE_NS_START_ADDR` must be set to the address of the non-secure vector table. This address is set in `primary\NonSecure\STM32H573IIKXQ_FLASH_MMT_TEMPLATE.ld`:

```
FLASH	(rx)	: ORIGIN = 0x08010000, LENGTH = 960K
```

The macro can then be set in `Secure/Core/Src/main.c`:

```C
#include "config.h"

...

/* USER CODE BEGIN VTOR_TABLE */

/* Non-secure Vector table to jump to                                         */
/* Caution: address must correspond to non-secure internal Flash where is     */
/*          mapped in the non-secure vector table                             */

#define VTOR_TABLE_NS_START_ADDR (FLASH_NS_BANK1_BASE_ADDR + FLASH_NS_REGION_OFFSET) /* = 0x08010000 */

/* USER CODE END VTOR_TABLE*/
```

This applies regardless of which firmware image is used since the current bank will always start at `0x08000000`.