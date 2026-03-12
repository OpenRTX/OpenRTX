/*
 * urpc_eth_udp.h
 *
 *  Created on: Dec 20, 2015
 *      Author: rob
 */

#ifndef URPC_ETH_UDP_H_
#define URPC_ETH_UDP_H_

/* TODO remove */
#include <stdio.h>
#include "urpc.h"

typedef struct urpc_endpoint_mbox {
    urpc_endpoint super;
    uint32_t ch_num;
} urpc_endpoint_mbox;

typedef struct urpc_connection_mbox {
    urpc_connection super;
    urpc_endpoint_mbox local;
} urpc_connection_mbox;

int8_t urpc_mbox_send(urpc_connection* s_conn, uint8_t* buf, uint16_t chn);

int8_t urpc_mbox_peek(urpc_connection* s_conn, uint8_t* buf, uint16_t chn);

int8_t urpc_mbox_recv(urpc_connection* s_conn, uint8_t* buf, uint16_t chn);

extern urpc_stub URPC_ETH_UDP_STUB;

#endif /* URPC_ETH_UDP_H_ */
