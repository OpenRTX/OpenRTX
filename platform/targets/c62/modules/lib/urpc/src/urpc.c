
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "urpc.h"

//static urpc_cb *rpc_cb = NULL;

int8_t urpc_init(urpc_cb *ctrl_block)
{
	int8_t ret = URPC_ERROR;
	int i;

	if(ctrl_block && ctrl_block->ep_cnt > 0 && ctrl_block->ep_cnt < 15) {  //EP0 should be exist, it is ctrl end point
		for(i = 0; i < ctrl_block->ep_cnt; i++) {
			if((ctrl_block->endpoints[i].handles_cnt && ctrl_block->endpoints[i].handles)
					|| ctrl_block->endpoints[i].handles_cnt == 0) {
			} else {
				break;
			}
		}
		if(i >= ctrl_block->ep_cnt) {
//			rpc_cb = ctrl_block;
			ret = URPC_SUCCESS;
		}
	}

	return ret;
}

int8_t urpc_register_handler(urpc_handler handler, urpc_hdl_info info, urpc_cb *rpc_cb)
{
	int8_t ret = URPC_ERROR;
    uint8_t ep_id, rpc_id;
    urpc_endpoint *endpoint;
    urpc_handle *handle;
    if(rpc_cb) {  //already initialized
    	if((info.ep_id < rpc_cb->ep_cnt) && (info.rpc_id < rpc_cb->endpoints->handles_cnt)) {
    		ep_id = info.ep_id;
    		rpc_id = info.rpc_id;
    		endpoint = &(rpc_cb->endpoints[ep_id]);
    		handle = endpoint->handles;
    		memcpy(handle->desc, info.desc, sizeof(handle->desc));
    		handle->flag = info.flag;
    		handle->handler = handler;
    		ret = URPC_SUCCESS;
    	}
    }

    return ret;
}


int8_t _urpc_handle_rpc(urpc_frame* frame, urpc_cb *rpc_cb)
{
	int8_t ret = URPC_ERROR, err;
	uint8_t ep_id, rpc_id, flags;

	flags = frame->header.flags;
	err = urpc_recv_check(rpc_cb, frame);
	if(err) {
		return err;
	}

	switch(flags & (URPC_FLAG_REQ_MASK | URPC_FLAG_SYNC_MASK | URPC_FLAG_PUSH_MASK)) {
	case (URPC_FLAG_REQUEST | URPC_FLAG_SYNC | URPC_FLAG_NORM):
	case (URPC_FLAG_REQUEST | URPC_FLAG_ASYNC | URPC_FLAG_NORM):
	case (URPC_FLAG_RESPONSE | URPC_FLAG_ASYNC | URPC_FLAG_NORM):
	case (URPC_FLAG_REQUEST | URPC_FLAG_ASYNC | URPC_FLAG_PUSH):
	case (URPC_FLAG_RESPONSE | URPC_FLAG_ASYNC | URPC_FLAG_PUSH):
		ep_id = frame->header.eps;
		rpc_id = frame->rpc.dst_id;
		if(rpc_cb->endpoints[ep_id].handles[rpc_id].handler) {
			(rpc_cb->endpoints[ep_id].handles[rpc_id].handler)(frame);
			ret = URPC_SUCCESS;
		}
		break;
	default:
		break;
	}

    return ret;
}

int8_t urpc_recv_check(urpc_cb *rpc_cb, urpc_frame* frame)
{
	uint8_t ep_id;

	if(frame->header.flags & URPC_FLAG_ERROR) {
		return URPC_ERROR;
	}
	if(frame->header.magic != URPC_VERSION) {
		return URPC_INVALID_HEADER;
	}
	if(frame->header.chksum != _urpc_chksum((uint8_t *)frame + 2)) {
		return URPC_INVALID_CRC;
	}
	if(frame->rpc.dst_id == 0 || frame->rpc.src_id == 0) {  // reserve for error handler
		return URPC_INVALID_ID;
	}
	ep_id = frame->header.eps;
	if(ep_id >= rpc_cb->ep_cnt) {
		return URPC_INVALID_EP;
	}
	if(!((frame->header.flags & URPC_FLAG_SYNC_MASK) && (frame->header.flags & URPC_FLAG_REQ_MASK) == 0)) {
		// not sync-response
		if(frame->rpc.dst_id >= rpc_cb->endpoints[ep_id].handles_cnt) {
			return URPC_INVALID_ID;
		}
	}

	return URPC_SUCCESS;
}

int8_t urpc_send_check(urpc_cb *rpc_cb, urpc_frame* frame)
{
	uint8_t ep_id;

	if(frame->header.eps >= rpc_cb->ep_cnt) {
		return URPC_INVALID_EP;
	}
	if(frame->rpc.dst_id == 0 || frame->rpc.src_id == 0) {  // id0 reserve for error handler
		return URPC_INVALID_ID;
	}
	ep_id = frame->header.eps;
	if(!((frame->header.flags & URPC_FLAG_SYNC_MASK) && (frame->header.flags & URPC_FLAG_REQ_MASK))) {
		// not sync-request
		if(frame->rpc.src_id >= rpc_cb->endpoints[ep_id].handles_cnt) {
			return URPC_INVALID_ID;
		}
	}
	return URPC_SUCCESS;
}

int8_t urpc_send(urpc_stub* stub, urpc_connection* conn, urpc_frame* frame)
{
	uint8_t ch;

	frame->header.magic = URPC_VERSION;
	frame->header.chksum = _urpc_chksum((uint8_t *)frame + 2);

	return stub->_send(conn, (uint8_t *)frame, frame->header.eps);
}

int8_t urpc_recv(urpc_stub* stub, urpc_connection* conn, urpc_frame* frame)
{
//    bzero(frame, sizeof(urpc_frame));
	return stub->_recv(conn, (uint8_t *)frame, 0);
}

int8_t urpc_set_payload(urpc_rpc* rpc, char* payload, uint16_t len)
{
	uint8_t padding;

	if(len > 10) {
		return URPC_ERROR;
	}
	padding = 10 - len;
	if(len) {
		memcpy(rpc->payload, payload, len);
	}
	if(padding) {
		memset(rpc->payload + len, 0, padding);
	}

    return URPC_SUCCESS;
}

void print_frame(urpc_frame* frame)
{
//    printf("===============\n");
//    printf("Version:     %2x\n", frame->header.version);
//    printf("ChkSum :     %2x\n", frame->header.chksum);
//    printf("Session:     %2x\n", frame->header.session);
//    printf("Flags:       %2x\n", frame->rpc.flags);
//    printf("EPS:         %2x\n", frame->rpc.eps);
//    printf("===============\n");
}

int8_t urpc_is_error(urpc_frame* frame)
{
	return frame->header.flags & URPC_FLAG_ERROR;
}


uint8_t _urpc_chksum(uint8_t *msg)
{
    uint8_t cs = 0x00, msg_len = 14;
    for (uint8_t i = 0; i < msg_len; i += 2) {
        cs ^= msg[i];
        cs ^= msg[i + 1];
    }
    return cs;
}
