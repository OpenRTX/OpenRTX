/*
 * urpc_eth_udp_server.h
 *
 *  Created on: Jan 31, 2016
 *      Author: rob
 */

#ifndef SRC_URPC_ETH_UDP_SERVER_H_
#define SRC_URPC_ETH_UDP_SERVER_H_

#include "urpc.h"
#include "urpc_server.h"
#include "urpc_mbox.h"

int8_t urpc_mbox_send_server(urpc_connection* s_conn, uint8_t* buf, uint16_t chn);

int8_t urpc_mbox_init_server(urpc_server* server, urpc_endpoint* s_endpoint);

int8_t urpc_mbox_accept(urpc_server* server, urpc_connection* conn, urpc_frame* frame);


#endif /* SRC_URPC_ETH_UDP_SERVER_H_ */
