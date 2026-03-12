/*
 * urpc_client.h
 *
 *  Created on: Feb 1, 2016
 *      Author: rob
 */

#ifndef SRC_URPC_CLIENT_H_
#define SRC_URPC_CLIENT_H_

#include "urpc.h"
#include "urpc_api.h"

//typedef struct urpc_client {
//    urpc_connection* conn;
//} urpc_client;
//
//typedef struct urpc_client_stub {
//    int8_t (*_send)(urpc_connection* conn, uint8_t* buf, uint16_t len);
//    int8_t (*_peek)(urpc_connection* conn, uint8_t* buf, uint16_t len);
//    int8_t (*_recv)(urpc_connection* conn, uint8_t* buf, uint16_t len);
//    int8_t (*init_client)(urpc_client* client);
//    int8_t (*connect)(urpc_client* client, urpc_connection* conn, urpc_frame* frame);
//} urpc_client_stub;


int8_t _urpc_recv_notify_client(urpc_client_stub* stub, urpc_connection* conn, uint8_t chn);

#endif /* SRC_URPC_CLIENT_H_ */
