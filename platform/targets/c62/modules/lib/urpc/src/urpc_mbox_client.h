/*
 * urpc_client_eth_udp.h
 *
 *  Created on: Jan 30, 2016
 *      Author: rob
 */

#ifndef SRC_URPC_ETH_UDP_CLIENT_H_
#define SRC_URPC_ETH_UDP_CLIENT_H_

#include "urpc.h"
#include "urpc_client.h"
#include "urpc_mbox.h"


int8_t urpc_mbox_send_client(urpc_connection* s_conn, uint8_t* buf, uint16_t chn);

int8_t urpc_mbox_init_client(urpc_client* client);

int8_t urpc_mbox_connect(urpc_client* client, urpc_connection* conn, urpc_frame* frame);

#endif /* SRC_URPC_ETH_UDP_CLIENT_H_ */
