/*
 * logging.c
 *
 *  Created on: Aug 21, 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "stdio.h"
#include "stdarg.h"
#include "hal.h"
#include "usart.h"

#include "logging.h"
#include "integrity.h"


extern UART_HandleTypeDef huart4;


#ifdef DEBUG
int _write(int file, char *ptr, int len) {

    hal_status_t status = HAL_OK;

    while (huart4.gState != HAL_UART_STATE_READY);
    status = HAL_UART_Transmit(&huart4, (uint8_t *) ptr, len, 1000);

    if (status == HAL_OK) {
        return len;
    } else {
        return -1;
    }
}
#endif
