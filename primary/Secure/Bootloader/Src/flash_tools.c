/*
 * flash_tools.c
 *
 *  Created on: Aug 19, 2025
 *      Author: bens1
 */

#include "flash_tools.h"
#include "main.h"


void FLASH_swap_banks() {

    /* In debug mode don't actually swap banks */
#ifdef DEBUG
    Error_Handler();
#endif
}
