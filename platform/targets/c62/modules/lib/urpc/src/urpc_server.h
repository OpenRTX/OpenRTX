/*
 * urpc_server.h
 *
 *  Created on: Jan 30, 2016
 *      Author: rob
 */

#ifndef SRC_URPC_SERVER_H_
#define SRC_URPC_SERVER_H_

#include "urpc.h"
#include "urpc_api.h"

#ifndef URPC_MAX_CLIENTS
#define URPC_MAX_CLIENTS 8
#endif /* URPC_MAX_CLIENTS */

//typedef struct urpc_server_conn {
//    uint8_t state;
//    urpc_connection* conn;
//} urpc_con_slot;
//
//typedef struct urpc_server {
//    urpc_endpoint* endpoint;
//    urpc_connection urpc_server_conn[URPC_MAX_CLIENTS];
//} urpc_server;
//
//typedef struct urpc_server_stub {
//    int8_t (*_send)(urpc_connection* conn, uint8_t* buf, uint16_t len);
//    int8_t (*_peek)(urpc_connection* conn, uint8_t* buf, uint16_t len);
//    int8_t (*_recv)(urpc_connection* conn, uint8_t* buf, uint16_t len);
//    int8_t (*init_server)(urpc_server* server, urpc_endpoint* endpoint);
//    int8_t (*accept)(urpc_server* server, urpc_connection* conn, urpc_frame* frame);
//} urpc_server_stub;


int8_t _urpc_recv_notify_server(urpc_server_stub* stub, urpc_connection* conn, uint8_t chn);

#endif /* SRC_URPC_SERVER_H_ */
