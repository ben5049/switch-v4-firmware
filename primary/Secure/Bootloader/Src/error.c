/*
 * error.c
 *
 *  Created on: Aug 20, 2025
 *      Author: bens1
 */

#include "error.h"
#include "logging.h"


void error_handler(error_t major_error_code, uint8_t minor_error_code) {

    __disable_irq();

    LOG_ERROR("Error handler :(\n");

    while (1) {
    }
}
