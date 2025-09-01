/*
 * memory_tools.h
 *
 *  Created on: Aug 19, 2025
 *      Author: bens1
 */

#ifndef INC_MEMORY_TOOLS_H_
#define INC_MEMORY_TOOLS_H_


#include "stdbool.h"
#include "flash.h"


void swap_banks(void);

void enable_backup_domain(void);

bool check_ns_firmware(uint8_t bank);


#endif /* INC_MEMORY_TOOLS_H_ */
