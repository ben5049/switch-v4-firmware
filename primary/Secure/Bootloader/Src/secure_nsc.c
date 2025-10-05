/*
 * secure_nsc.c
 *
 *  Created on: Oct 5, 2025
 *      Author: bens1
 */

#include "secure_nsc.h"
#include "metadata.h"
#include "error.h"


CMSE_NS_ENTRY void s_save_dhcp_client_record(const NX_DHCP_CLIENT_RECORD *record) {
    HAL_ResumeTick();
    memcpy(&hmeta.metadata.dhcp_record, record, sizeof(NX_DHCP_CLIENT_RECORD));
    hmeta.new_metadata = true;
    HAL_SuspendTick();
}


CMSE_NS_ENTRY void s_load_dhcp_client_record(NX_DHCP_CLIENT_RECORD *record) {
    HAL_ResumeTick();
    if (hmeta.first_boot) {
        record->nx_dhcp_state = NX_DHCP_STATE_NOT_STARTED;
    } else {
        memcpy(record, &hmeta.metadata.dhcp_record, sizeof(NX_DHCP_CLIENT_RECORD));
    }
    HAL_SuspendTick();
}


/* Call this function from a thread in ThreadX at 1Hz to do background work in the secure world */
CMSE_NS_ENTRY void s_background_task(void) {
    HAL_ResumeTick();

    metadata_status_t status = META_OK;

    if (hmeta.new_metadata) {
        status = META_dump_metadata(&hmeta);
        CHECK_STATUS_META(status);
    }

    if (hmeta.new_counters) {
        status = META_dump_counters(&hmeta);
        CHECK_STATUS_META(status);
    }

    HAL_SuspendTick();
}
