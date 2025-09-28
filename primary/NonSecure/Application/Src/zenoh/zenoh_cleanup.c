/*
 * zenoh_cleanup.c
 *
 *  Created on: Sep 28, 2025
 *      Author: bens1
 */

#include "tx_api.h"
#include "tx_thread.h"
#include "tx_mutex.h"
#include "tx_semaphore.h"
#include "nx_api.h"

#include "zenoh_cleanup.h"
#include "comms_thread.h"
#include "tx_app.h"
#include "nx_app.h"


void zenoh_cleanup_tx() {

    tx_status_t status = TX_SUCCESS;

    if (_tx_thread_created_count > 0) {
        TX_THREAD *current_thread = _tx_thread_created_ptr;
        TX_THREAD *next_thread    = current_thread->tx_thread_created_next;
        do {
            if (((uint8_t *) current_thread >= zenoh_byte_pool_buffer) && ((uint8_t *) current_thread < (zenoh_byte_pool_buffer + ZENOH_MEM_POOL_SIZE))) {
                status = tx_thread_delete(current_thread);
                if (status != TX_SUCCESS) Error_Handler();
            }
            current_thread = next_thread;
            next_thread    = current_thread->tx_thread_created_next;
        } while ((current_thread != _tx_thread_created_ptr));
    }

    if (_tx_mutex_created_count > 0) {
        TX_MUTEX *current_mutex = _tx_mutex_created_ptr;
        TX_MUTEX *next_mutex    = current_mutex->tx_mutex_created_next;
        do {
            if (((uint8_t *) current_mutex >= zenoh_byte_pool_buffer) && ((uint8_t *) current_mutex < (zenoh_byte_pool_buffer + ZENOH_MEM_POOL_SIZE))) {
                status = tx_mutex_delete(current_mutex);
                if (status != TX_SUCCESS) Error_Handler();
            }
            current_mutex = next_mutex;
            next_mutex    = current_mutex->tx_mutex_created_next;
        } while ((current_mutex != _tx_mutex_created_ptr));
    }

    if (_tx_semaphore_created_count > 0) {
        TX_SEMAPHORE *current_semaphore = _tx_semaphore_created_ptr;
        TX_SEMAPHORE *next_semaphore    = current_semaphore->tx_semaphore_created_next;
        do {
            if (((uint8_t *) current_semaphore >= zenoh_byte_pool_buffer) && ((uint8_t *) current_semaphore < (zenoh_byte_pool_buffer + ZENOH_MEM_POOL_SIZE))) {
                status = tx_semaphore_delete(current_semaphore);
                if (status != TX_SUCCESS) Error_Handler();
            }
            current_semaphore = next_semaphore;
            next_semaphore    = current_semaphore->tx_semaphore_created_next;
        } while ((current_semaphore != _tx_semaphore_created_ptr));
    }
}


void zenoh_cleanup_nx() {

    nx_status_t status = NX_SUCCESS;

    if (nx_ip_instance.nx_ip_udp_created_sockets_count > 0) {
        NX_UDP_SOCKET *current_udp_socket = nx_ip_instance.nx_ip_udp_created_sockets_ptr;
        NX_UDP_SOCKET *next_udp_socket    = current_udp_socket->nx_udp_socket_created_next;
        do {
            if (((uint8_t *) current_udp_socket >= zenoh_byte_pool_buffer) && ((uint8_t *) current_udp_socket < (zenoh_byte_pool_buffer + ZENOH_MEM_POOL_SIZE))) {
                status = nx_udp_socket_delete(current_udp_socket);
                if (status != NX_SUCCESS) Error_Handler();
            }
            current_udp_socket = next_udp_socket;
            next_udp_socket    = current_udp_socket->nx_udp_socket_created_next;
        } while ((current_udp_socket != _tx_semaphore_created_ptr));
    }

    // TODO: Delete TCP sockets when implemented
}
