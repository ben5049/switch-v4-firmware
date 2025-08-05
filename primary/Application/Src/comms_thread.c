/*
 * comms_thread.c
 *
 *  Created on: Aug 1, 2025
 *      Author: bens1
 */

#include "nx_api.h"
#include "nx_stm32_eth_config.h"

#include "zenoh-pico.h"
#include "comms_thread.h"
#include "utils.h"


#define WINDOW_SIZE 512

// #define LINK_PRIORITY                         11

#define NULL_ADDRESS                         0

#define DEFAULT_PORT                         6000
#define TCP_SERVER_PORT                      DEFAULT_PORT
#define TCP_SERVER_ADDRESS                   IP_ADDRESS(192, 168, 1, 1)

#define MAX_PACKET_COUNT                     100
#define DEFAULT_MESSAGE                      "TCP Client on STM32H573-DK"

#define NX_APP_CABLE_CONNECTION_CHECK_PERIOD (1 * NX_IP_PERIODIC_RATE)

#define NX_APP_DEFAULT_TIMEOUT               (10 * NX_IP_PERIODIC_RATE)

ULONG          IpAddress;
NX_IP          NetXDuoEthIpInstance;
NX_TCP_SOCKET  TCPSocket;
NX_PACKET_POOL NxAppPool;

uint8_t   comms_thread_stack[COMMS_THREAD_STACK_SIZE];
TX_THREAD comms_thread_ptr;


void comms_thread_entry(uint32_t initial_input) {

    UINT ret;
    UINT count = 0;

    ULONG bytes_read;
    UCHAR data_buffer[512];

    ULONG source_ip_address;
    UINT  source_port;

    NX_PACKET *server_packet;
    NX_PACKET *data_packet;

    /* create the TCP socket */
    ret = nx_tcp_socket_create(&NetXDuoEthIpInstance, &TCPSocket, "TCP Server Socket", NX_IP_NORMAL, NX_FRAGMENT_OKAY,
                               NX_IP_TIME_TO_LIVE, WINDOW_SIZE, NX_NULL, NX_NULL);
    if (ret != NX_SUCCESS) {
        Error_Handler();
    }

    /* bind the client socket for the DEFAULT_PORT */
    ret = nx_tcp_client_socket_bind(&TCPSocket, DEFAULT_PORT, NX_WAIT_FOREVER);

    if (ret != NX_SUCCESS) {
        Error_Handler();
    }

    /* connect to the remote server on the specified port */
    ret = nx_tcp_client_socket_connect(&TCPSocket, TCP_SERVER_ADDRESS, TCP_SERVER_PORT, NX_WAIT_FOREVER);

    if (ret != NX_SUCCESS) {
        Error_Handler();
    }

    while (count++ < MAX_PACKET_COUNT) {
        TX_MEMSET(data_buffer, '\0', sizeof(data_buffer));

        /* allocate the packet to send over the TCP socket */
        ret = nx_packet_allocate(&NxAppPool, &data_packet, NX_UDP_PACKET, TX_WAIT_FOREVER);

        if (ret != NX_SUCCESS) {
            break;
        }

        /* append the message to send into the packet */
        ret = nx_packet_data_append(data_packet, (VOID *) DEFAULT_MESSAGE, sizeof(DEFAULT_MESSAGE), &NxAppPool, TX_WAIT_FOREVER);

        if (ret != NX_SUCCESS) {
            nx_packet_release(data_packet);
            break;
        }

        /* send the packet over the TCP socket */
        ret = nx_tcp_socket_send(&TCPSocket, data_packet, NX_APP_DEFAULT_TIMEOUT);

        if (ret != NX_SUCCESS) {
            break;
        }

        /* wait for the server response */
        ret = nx_tcp_socket_receive(&TCPSocket, &server_packet, NX_APP_DEFAULT_TIMEOUT);

        if (ret == NX_SUCCESS) {
            /* get the server IP address and  port */
            nx_udp_source_extract(server_packet, &source_ip_address, &source_port);

            /* retrieve the data sent by the server */
            nx_packet_data_retrieve(server_packet, data_buffer, &bytes_read);

            /* print the received data */
            // PRINT_DATA(source_ip_address, source_port, data_buffer);

            /* release the server packet */
            nx_packet_release(server_packet);

            /* toggle the green led on success */
            HAL_GPIO_WritePin(GPIOI, GPIO_PIN_9, GPIO_PIN_SET);
        } else {
            /* no message received exit the loop */
            break;
        }
    }

    /* release the allocated packets */
    nx_packet_release(server_packet);

    /* disconnect the socket */
    nx_tcp_socket_disconnect(&TCPSocket, NX_APP_DEFAULT_TIMEOUT);

    /* unbind the socket */
    nx_tcp_client_socket_unbind(&TCPSocket);

    /* delete the socket */
    nx_tcp_socket_delete(&TCPSocket);

    /* print test summary on the UART */
    if (count == MAX_PACKET_COUNT + 1) {
        printf("\n-------------------------------------\n\tSUCCESS : %u / %u packets sent\n-------------------------------------\n", count - 1, MAX_PACKET_COUNT);
        //        Success_Handler();
    } else {
        printf("\n-------------------------------------\n\tFAIL : %u / %u packets sent\n-------------------------------------\n", count - 1, MAX_PACKET_COUNT);
        Error_Handler();
    }
}
