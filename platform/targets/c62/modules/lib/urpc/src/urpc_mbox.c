/*
 * urpc_eth_udp.c
 *
 *  Created on: Dec 20, 2015
 *      Author: rob
 */


#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include "Driver_MBX.h"
#include "urpc_mbox.h"

int8_t urpc_mbox_send(urpc_connection* s_conn, uint8_t* buf, uint16_t chn)
{
	int8_t ret;
	ret = MBX_Send(s_conn, buf, chn);
    return ret;
}

int8_t urpc_mbox_peek(urpc_connection* s_conn, uint8_t* buf, uint16_t chn)
{
//    urpc_connection_mbox* conn = (urpc_connection_mbox*)s_conn;
    return URPC_SUCCESS;
}

int8_t urpc_mbox_recv(urpc_connection* s_conn, uint8_t* buf, uint16_t chn)
{
//    urpc_connection_mbox* conn = (urpc_connection_mbox*)s_conn;
    return MBX_Receive(s_conn, buf, chn);
}
