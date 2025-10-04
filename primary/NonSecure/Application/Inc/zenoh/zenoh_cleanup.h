/*
 * zenoh_cleanup.h
 *
 *  Created on: Sep 28, 2025
 *      Author: bens1
 */

#ifndef INC_ZENOH_ZENOH_CLEANUP_H_
#define INC_ZENOH_ZENOH_CLEANUP_H_

#ifdef __cplusplus
extern "C" {
#endif


void zenoh_cleanup_tx(void);
void zenoh_cleanup_nx(void);


#ifdef __cplusplus
}
#endif

#endif /* INC_ZENOH_ZENOH_CLEANUP_H_ */
