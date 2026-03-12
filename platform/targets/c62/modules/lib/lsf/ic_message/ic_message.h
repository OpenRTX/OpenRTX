#ifndef __IC_MESSAGE_H__
#define __IC_MESSAGE_H__

#include <stdint.h>

#ifndef ARRSIZE
#define ARRSIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

enum {
	IC_MESSAGE_ID_RESERVE_0 = 0,
	IC_MESSAGE_ID_RESERVE_1,
	IC_MESSAGE_ID_RESERVE_2,
	IC_MESSAGE_ID_RESERVE_3,
	IC_MESSAGE_ID_INNER_CONN, /* Do not touch this id */
	IC_MESSAGE_ID_OCR,
	IC_MESSAGE_ID_STREAM,
	IC_MESSAGE_ID_COMMON,
	IC_MESSAGE_ID_TTS,
	IC_MESSAGE_ID_TRANS,
	IC_MESSAGE_ID_MIC,
	IC_MESSAGE_ID_RESOURCE,
	IC_MESSAGE_ID_DISK_MEM,
	IC_MESSAGE_ID_CSP,
	IC_MESSAGE_ID_WSP,
	IC_MESSAGE_ID_COMP_RES,
	IC_MESSAGE_ID_SPD,
	IC_MESSAGE_ID_CSPS,
	IC_MESSAGE_ID_WAKEUP,
	IC_MESSAGE_ID_MAX,
};

typedef enum {
	IC_MESSAGE_MSG_TYPE_ERR,
	IC_MESSAGE_MSG_TYPE_CMD,
	IC_MESSAGE_MSG_TYPE_EVT,
	IC_MESSAGE_MSG_TYPE_STREAM,
} ic_message_msg_type_t;

typedef struct ic_message_handle_info {
	void *user_datas;
	void *cb;
	uint8_t id;
	uint8_t remote_conn;
} ic_message_handle_info_t;

typedef struct ic_message_msg_info {
	void *msg;
	uint32_t len;
	uint8_t msg_type;
} ic_message_msg_info_t;

typedef int32_t (*ic_message_callback_t)(ic_message_handle_info_t *, ic_message_msg_info_t*);

int ic_message_init();
int ic_message_register_by_id(uint8_t service_id, ic_message_callback_t cb, void *user_datas);
int ic_message_msg_send_by_id(uint8_t service_id, uint8_t type, void *msg, uint32_t len);
uint8_t ic_message_remote_is_connected(uint8_t id);

enum {
    IC_MESSAGE_ERR_NONE = 0,
    IC_MESSAGE_ERR_NOT_NULL,
    IC_MESSAGE_ERR_ID_INVALID,
    IC_MESSAGE_ERR_PARAM_INVALID,
    IC_MESSAGE_ERR_INITED,
    IC_MESSAGE_ERR_URPC,
    IC_MESSAGE_ERR_REMOTE_NOT_CONN,
    IC_MESSAGE_ERR_NOT_INITED,
};

#endif
