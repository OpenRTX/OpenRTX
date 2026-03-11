/*
 * urpc_api.h
 *
 */

#ifndef URPC_API_H_
#define URPC_API_H_

#include <stdint.h>

#define URPC_WITH_DESC        1


#define URPC_SUCCESS                 0

#define URPC_DRV_ERROR              -1 ///< Unspecified error
#define URPC_DRV_BUSY               -2 ///< Driver is busy
#define URPC_DRV_TIMEOUT            -3 ///< Timeout occurred
#define URPC_DRV_UNSUPPORTED        -4 ///< Operation not supported
#define URPC_DRV_PARAMETER          -5 ///< Parameter error
#define URPC_DRV_SPECIFIC           -6 ///< Start of driver specific errors

#define URPC_ERROR                 -11
#define URPC_INVALID_CRC           -12
#define URPC_RPC_TOO_LARGE         -13
#define URPC_INVALID_HEADER        -14
#define URPC_TIMEOUT               -15
#define URPC_INVALID_EP            -16
#define URPC_INVALID_ID            -17
#define URPC_QUE_FULL              -18
#define URPC_INVALID_SYNC          -19
#define URPC_INVALID_SEQ           -20

#define URPC_FLAG_NOERR       (0x0 << 0)
#define URPC_FLAG_ERROR       (0x1 << 0)
#define URPC_FLAG_ERR_MASK    (0x1 << 0)

#define URPC_FLAG_RESPONSE    (0x0 << 1)
#define URPC_FLAG_REQUEST     (0x1 << 1)
#define URPC_FLAG_REQ_MASK    (0x1 << 1)

#define URPC_FLAG_ASYNC       (0x0 << 2)
#define URPC_FLAG_SYNC        (0x1 << 2)
#define URPC_FLAG_SYNC_MASK   (0x1 << 2)

#define URPC_FLAG_NORM        (0x0 << 3)
#define URPC_FLAG_PUSH        (0x1 << 3)
#define URPC_FLAG_PUSH_MASK   (0x1 << 3)

#define URPC_STATE_RESET      0U
#define URPC_STATE_IDLE       1U
#define URPC_STATE_CONN       2U
#define URPC_STATE_ERROR      3U

#define URPC_CONN_SERVER      (urpc_connection *)0x10
#define URPC_CONN_CLIENT      (urpc_connection *)0x20

#define URPC_DESC_SIZE        11

#if URPC_WITH_DESC
#define URPC_DESC(desc)       desc
#else
#define URPC_DESC(desc)
#endif

typedef struct urpc_frame_header {
    int8_t magic;
    uint8_t chksum;
    uint8_t session;
    uint8_t flags : 4;
    uint8_t eps   : 4;
} urpc_frame_header;

typedef struct urpc_rpc {
	uint8_t dst_id;
    uint8_t src_id;
    uint8_t payload[10];
} urpc_rpc;

typedef struct urpc_frame {
    urpc_frame_header header;
    urpc_rpc rpc;
} urpc_frame;

typedef void (*urpc_handler)(urpc_frame* frame);

typedef struct urpc_handle {
	urpc_handler handler;
	uint8_t flag;   //request or response
#if URPC_WITH_DESC
	char desc[URPC_DESC_SIZE];
#endif
//	uint16_t state;
} urpc_handle;

typedef struct urpc_endpoint {
	urpc_handle *handles;
	uint8_t handles_cnt;
#if URPC_WITH_DESC
    char desc[URPC_DESC_SIZE];
#endif
} urpc_endpoint;

typedef struct urpc_cb {
	urpc_endpoint *endpoints;
	uint8_t  ep_cnt;
}urpc_cb;

typedef struct urpc_hdl_info {
	uint8_t ep_id;    // should be less than 15
	uint8_t rpc_id;    // should be less than 255
	uint8_t flag;     //request or response
#if URPC_WITH_DESC
    char desc[URPC_DESC_SIZE];
#endif
} urpc_hdl_info;

typedef struct urpc_connection {
    uint8_t transport;
    /* virtual local
     * virtual remote
     */
} urpc_connection;

typedef struct urpc_client {
    urpc_connection* conn;
} urpc_client;

typedef struct urpc_client_stub {
    int8_t (*_send)(urpc_connection* conn, uint8_t* buf, uint16_t len);
    int8_t (*_peek)(urpc_connection* conn, uint8_t* buf, uint16_t len);
    int8_t (*_recv)(urpc_connection* conn, uint8_t* buf, uint16_t len);
    int8_t (*init_client)(urpc_client* client);
    int8_t (*connect)(urpc_client* client, urpc_connection* conn, urpc_frame* frame);
} urpc_client_stub;

typedef struct urpc_server_conn {
    urpc_connection* conn;
} urpc_con_slot;

typedef struct urpc_server {
	urpc_connection* conn;
} urpc_server;

typedef struct urpc_server_stub {
    int8_t (*_send)(urpc_connection* conn, uint8_t* buf, uint16_t len);
    int8_t (*_peek)(urpc_connection* conn, uint8_t* buf, uint16_t len);
    int8_t (*_recv)(urpc_connection* conn, uint8_t* buf, uint16_t len);
    int8_t (*init_server)(urpc_server* server, urpc_endpoint* endpoint);
    int8_t (*accept)(urpc_server* server, urpc_connection* conn, urpc_frame* frame);
} urpc_server_stub;


int8_t urpc_accept(urpc_server_stub* stub);

int8_t urpc_send_sync_server(urpc_server_stub* stub, urpc_frame* frame);

int8_t urpc_send_async_server(urpc_server_stub* stub, urpc_frame* frame);

int8_t urpc_init_server(urpc_server_stub* stub, urpc_cb *ctrl_block);

int8_t urpc_push_event_server(urpc_server_stub* stub, urpc_frame* frame);

urpc_server_stub* urpc_mbox_get_server_stub(void);


int8_t urpc_connect(urpc_client_stub* stub);

int8_t urpc_trans_sync_client(urpc_client_stub* stub, urpc_frame* send, urpc_frame* recv);

int8_t urpc_send_async_client(urpc_client_stub* stub, urpc_frame* frame);

int8_t urpc_init_client(urpc_client_stub* stub, urpc_cb *ctrl_block);

int8_t urpc_open_event_client(urpc_client_stub* stub, urpc_frame* frame);

int8_t urpc_close_event_client(urpc_client_stub* stub, urpc_frame* frame);

urpc_client_stub* urpc_mbox_get_client_stub(void);

#endif /* URPC_API_H_ */
