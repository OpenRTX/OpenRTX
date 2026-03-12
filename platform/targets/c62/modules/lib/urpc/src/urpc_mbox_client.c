/*
 * urpc_eth_udp_client.c
 *
 *  Created on: Jan 31, 2016
 *      Author: rob
 */
#include "urpc_os_port_internal.h"
#include "urpc.h"
#include "urpc_mbox.h"
#include "urpc_mbox_client.h"
#include "Driver_MBX.h"
#include "urpc_client.h"

static urpc_client_stub URPC_MBOX_CLIENT_STUB;
static urpc_sem_t send_sema;

//static urpc_endpoint *urpc_ep_array[URPC_EP_COUNT];
static int8_t mbox_cb(uint32_t event, uint32_t param)
{
	urpc_base_type resch;
	int8_t ret = 0;
	switch(event) {
	case CSK_MBX_EVENT_RECEIVE_COMPLETE:
		ret = _urpc_recv_notify_client((urpc_client_stub *)&URPC_MBOX_CLIENT_STUB, URPC_CONN_CLIENT, param);
		break;
	case CSK_MBX_EVENT_SEND_COMPLETE:
		urpc_sem_give_from_isr(send_sema, &resch);
		portYIELD_FROM_ISR(resch);
		break;
	default:
//		CLOGD("rx err\n");
		break;
	}
	return ret;
}

urpc_client_stub* urpc_mbox_get_client_stub(void) {
    return &URPC_MBOX_CLIENT_STUB;
}

int8_t urpc_mbox_send_client(urpc_connection* s_conn, uint8_t* buf, uint16_t chn)
{
	urpc_sem_take(send_sema, urpc_max_delay);
	return urpc_mbox_send(s_conn, buf, chn);
}

int8_t urpc_mbox_init_client(urpc_client* client) {
	send_sema = xSemaphoreCreateBinary();
	urpc_sem_give(send_sema);
	MBX_Initialize(URPC_CONN_CLIENT, mbox_cb);
    return URPC_SUCCESS;
}

int8_t urpc_mbox_connect(urpc_client* client, urpc_connection* conn, urpc_frame* frame)
{
	//through end point 0, to connect server
	urpc_frame req, resp;
	int8_t err;

	req.header.eps = 0;
	req.rpc.dst_id = 1;
	req.rpc.src_id = 0xff;
	err = urpc_trans_sync_client((urpc_client_stub *)&URPC_MBOX_CLIENT_STUB, &req, &resp);
	if(err) { // server not ready
		//clear send_sema
		urpc_sem_give_from_isr(send_sema, NULL);  //set semaphore
		return URPC_ERROR;
	}

    return URPC_SUCCESS;
}

int8_t urpc_mbox_peek_client(urpc_connection* s_conn, uint8_t* buf, uint16_t chn)
{
	uint32_t arg;

	urpc_task_enter_critcal();
	MBX_Control(s_conn, CSK_MBX_AP_CTRL_GET_RX, &arg);
	if(!arg) {
		// receive next
		MBX_Receive(s_conn, buf, chn);
		MBX_Control(s_conn, CSK_MBX_AP_CTRL_NOTIFY, NULL);
	}
	urpc_task_exit_critcal();
    return URPC_SUCCESS;
}

/* Note that this would be PROGMEM for embedded platformds */
static urpc_client_stub URPC_MBOX_CLIENT_STUB = {
    ._send       = &urpc_mbox_send_client,
    ._peek       = &urpc_mbox_peek_client,
    ._recv       = &urpc_mbox_recv,
    .init_client = &urpc_mbox_init_client,
    .connect     = &urpc_mbox_connect,
};
