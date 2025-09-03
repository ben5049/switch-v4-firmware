/*
 * memory_tools.h
 *
 *  Created on: Aug 19, 2025
 *      Author: bens1
 */

#ifndef INC_MEMORY_TOOLS_H_
#define INC_MEMORY_TOOLS_H_

#include "stdint.h"
#include "stdbool.h"
#include "flash.h"


bool get_bank_swap(void);
void swap_banks(void);

void enable_backup_domain(void);

bool check_s_firmware(uint8_t bank);
bool check_ns_firmware(uint8_t bank);

bool copy_s_firmware_to_other_bank(bool bank_swap);
bool copy_ns_firmware_to_other_bank(bool bank_swap);
bool copy_ns_firmware(uint8_t from_bank, uint8_t to_bank);


#endif /* INC_MEMORY_TOOLS_H_ */
