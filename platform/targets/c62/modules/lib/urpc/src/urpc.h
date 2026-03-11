/*
 * urpc.h
 *
 */

#ifndef URPC_H_
#define URPC_H_

#include <stdint.h>
#include "urpc_api.h"

#ifndef URPC_MAX_REQUEST_HANDLERS
#define URPC_MAX_REQUEST_HANDLERS  5
#endif /* URPC_MAX_REQUEST_HANDLERS */

#ifndef URPC_MAX_RESPONSE_HANDLERS
#define URPC_MAX_RESPONSE_HANDLERS 5
#endif /* URPC_MAX_RESPONSE_HANDLERS */

#define _URPC_CONTROL_REQUEST_HANDLERS 2
#define _URPC_CONTROL_RESPONSE_HANDLERS 2

/*
 * Block ciphers operate in set block sizes. The AES that is used here works
 * on 16 byte blocks. So that we can encrypt/decrypt in place, we keep the
 * frame size in a set multiple of chunks for the frame payload.
 */
#ifndef URPC_CHUNK_SIZE
#define URPC_CHUNK_SIZE 16
#endif /* URPC_CHUNK_SIZE */

/*
 * Allow users to override the max frame size by overriding the number of
 * chunks per frame. Default to a 64 byte frame payload.
 */
#ifndef URPC_MAX_FRAME_CHUNKS
#define URPC_MAX_FRAME_CHUNKS 4
#endif /* URPC_MAX_FRAME_CHUNKS */

#define _URPC_CRC_POLY        0x96

#define URPC_VERSION          0x10

#define URPC_EP_0    0x00
#define URPC_EP_1    0x01
#define URPC_EP_2    0x02
#define URPC_EP_3    0x03
#define URPC_EP_4    0x04
#define URPC_EP_5    0x05
#define URPC_EP_6    0x06
#define URPC_EP_7    0x07
#define URPC_EP_8    0x08
#define URPC_EP_9    0x09
#define URPC_EP_10   0x0a
#define URPC_EP_11   0x0b
#define URPC_EP_12   0x0c
#define URPC_EP_13   0x0d
#define URPC_EP_14   0x0e
#define URPC_EP_15   0x0f


typedef struct urpc_stub {
    int8_t (*_send)(urpc_connection* conn, uint8_t* buf, uint16_t chn);
    int8_t (*_peek)(urpc_connection* conn, uint8_t* buf, uint16_t chn);
    int8_t (*_recv)(urpc_connection* conn, uint8_t* buf, uint16_t chn);
} urpc_stub;

int8_t urpc_init(urpc_cb *ctrl_block);

int8_t urpc_register_handler(urpc_handler handler, urpc_hdl_info info, urpc_cb *rpc_cb);

int8_t _urpc_handle_rpc(urpc_frame* frame, urpc_cb *rpc_cb);

int8_t urpc_send(urpc_stub* stub, urpc_connection* conn, urpc_frame* frame);

int8_t urpc_recv(urpc_stub* stub, urpc_connection* conn, urpc_frame* frame);

int8_t urpc_set_payload(urpc_rpc* rpc, char* payload, uint16_t len);

int8_t urpc_is_error(urpc_frame* frame);

uint8_t _urpc_chksum(uint8_t *msg);

int8_t urpc_send_check(urpc_cb *rpc_cb, urpc_frame* frame);

int8_t urpc_recv_check(urpc_cb *rpc_cb, urpc_frame* frame);

#endif /* URPC_H_ */
