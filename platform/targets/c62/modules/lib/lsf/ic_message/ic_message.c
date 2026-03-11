#include "ic_message.h"
#include "urpc_api.h"
#include <string.h>

#if CONFIG_LSF_CLIENT
#include "rpc_client.h"
#include "ic_proxy.h"
#elif CONFIG_LSF_SERVER
#include "ic_service.h"
#include "rpc_server.h"
#include "venus_log.h"
#else
#error lsf type not configured
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef CONFIG_LSF_IC_MESSAGE_EP_ID
#error The value of CONFIG_LSF_IC_MESSAGE_EP_ID is not defined, uncomment this line to use the default value.
#define CONFIG_LSF_IC_MESSAGE_EP_ID 5
#endif

#if CONFIG_LSF_IC_MESSAGE_LOG

#endif

#if CONFIG_LSF_OS_XOS
/* FIXME: */
#define lsf_task_enter_critcal()
#define lsf_task_exit_critcal()
#endif

typedef struct ic_message_inner_conn_data {
	uint8_t id;
} __attribute__ ((__packed__)) ic_message_inner_conn_data_t;

static ic_message_handle_info_t handle_infos[IC_MESSAGE_ID_MAX] = {0};
static urpc_handle ic_message_ep_handles[IC_MESSAGE_ID_MAX] = {0};
static uint8_t ic_message_inited = 0;

static int32_t ic_message_inner_conn_msg_send(uint8_t id)
{
	ic_message_inner_conn_data_t data = {
		.id = id,
	};

	return ic_message_msg_send_by_id(IC_MESSAGE_ID_INNER_CONN, IC_MESSAGE_MSG_TYPE_CMD, &data, sizeof(data));
}

static void ic_message_urpc_handler(urpc_frame *frame)
{
	uint8_t id = frame->rpc.dst_id;

	if (id >= IC_MESSAGE_ID_MAX) {
		/* error invalid id */
	}

	if (handle_infos[id].cb != NULL) {
		ic_message_msg_info_t msg;
		msg.len = 10;
		msg.msg_type = IC_MESSAGE_MSG_TYPE_CMD;
		msg.msg = &frame->rpc.payload[0];
		(void)((ic_message_callback_t)handle_infos[id].cb)(&handle_infos[id], &msg);
	}
}

int ic_message_register_by_id(uint8_t id, ic_message_callback_t cb, void *user_datas)
{
	if (id >= IC_MESSAGE_ID_MAX) {
		return IC_MESSAGE_ERR_ID_INVALID;
	}

	if (handle_infos[id].cb != NULL) {
		return IC_MESSAGE_ERR_NOT_NULL;
	}

	if (!ic_message_inited) {
		return IC_MESSAGE_ERR_NOT_INITED;
	}

	lsf_task_enter_critcal();
	handle_infos[id].cb = cb;
	handle_infos[id].id = id;
	handle_infos[id].user_datas = user_datas;
	lsf_task_exit_critcal();

	return ic_message_inner_conn_msg_send(id);
}

int ic_message_msg_send_by_id(uint8_t id, uint8_t type, void *msg, uint32_t len)
{
	urpc_frame frame[16] = { 0 };
	int r = 0;

	if (!ic_message_inited) {
		return IC_MESSAGE_ERR_NOT_INITED;
	}

	if (id >= IC_MESSAGE_ID_MAX) {
		return IC_MESSAGE_ERR_ID_INVALID;
	}

	if (len > 10 || msg == NULL ) {
		return IC_MESSAGE_ERR_PARAM_INVALID;
	}

	if (id != IC_MESSAGE_ID_INNER_CONN && !handle_infos[id].remote_conn) {
		return IC_MESSAGE_ERR_REMOTE_NOT_CONN;
	}

	frame->header.eps = CONFIG_LSF_IC_MESSAGE_EP_ID;

	if (type == IC_MESSAGE_MSG_TYPE_EVT) {
		frame->rpc.dst_id = id;
		frame->rpc.src_id = id;
	} else if (type == IC_MESSAGE_MSG_TYPE_CMD) {
		frame->rpc.dst_id = id;
		frame->rpc.src_id = id;
		/* TODO: FIX ME */
	} else {
		return IC_MESSAGE_ERR_PARAM_INVALID;
	}

	memcpy(&frame->rpc.payload[0], msg, len);
	
#if CONFIG_LSF_CLIENT
	r = urpc_send_async_client(RPC_Client_stub, (urpc_frame *)&frame);
#elif CONFIG_LSF_SERVER
	r = urpc_push_event_server(RPC_Server_stub, (urpc_frame *)&frame);
#endif
	if (r) {
		return IC_MESSAGE_ERR_URPC;
	}

	return IC_MESSAGE_ERR_NONE;
}

static int32_t ic_message_inner_conn_cb(ic_message_handle_info_t *handle_info, ic_message_msg_info_t* msg_info)
{
	(void)handle_info;
	ic_message_inner_conn_data_t *inner_conn_data = msg_info->msg;

	if (inner_conn_data->id >= IC_MESSAGE_ID_MAX) {
		return IC_MESSAGE_ERR_ID_INVALID;
	}

	handle_infos[inner_conn_data->id].remote_conn = 1;

	return 0;
}

int ic_message_init(void)
{
	uint8_t i;

	if (ic_message_inited) {
		return IC_MESSAGE_ERR_INITED;
	}

	for (i = 0; i < IC_MESSAGE_ID_MAX; i++) {
		ic_message_ep_handles[i].flag = URPC_FLAG_ASYNC;
		ic_message_ep_handles[i].handler = ic_message_urpc_handler;
	}
	
	/* inner conn handle, uper not care, must before RPC_XXX_START() */
	handle_infos[IC_MESSAGE_ID_INNER_CONN].cb = ic_message_inner_conn_cb;
	handle_infos[IC_MESSAGE_ID_INNER_CONN].id = IC_MESSAGE_ID_INNER_CONN;
	handle_infos[IC_MESSAGE_ID_INNER_CONN].user_datas = NULL;
	handle_infos->remote_conn = 1;

#if CONFIG_LSF_CLIENT
	RPC_Client_eps[CONFIG_LSF_IC_MESSAGE_EP_ID].handles = ic_message_ep_handles;
	RPC_Client_eps[CONFIG_LSF_IC_MESSAGE_EP_ID].handles_cnt = ARRSIZE(ic_message_ep_handles);

	RPC_Client_init();
	IC_Proxy_init();
	RPC_Client_Start();

#elif CONFIG_LSF_SERVER
	RPC_Server_eps[CONFIG_LSF_IC_MESSAGE_EP_ID].handles = ic_message_ep_handles;
	RPC_Server_eps[CONFIG_LSF_IC_MESSAGE_EP_ID].handles_cnt = ARRSIZE(ic_message_ep_handles);

	RPC_Server_init();
	RPC_Server_Start();
	IC_Service_init();
#else
#error lsf config error
#endif

	ic_message_inited = 1;

	return IC_MESSAGE_ERR_NONE;
}

uint8_t ic_message_remote_is_connected(uint8_t id)
{
	return (id < IC_MESSAGE_ID_MAX) & handle_infos[id].remote_conn;
}
