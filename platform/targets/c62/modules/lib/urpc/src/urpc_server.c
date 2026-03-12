/*
 * urpc_server.c
 *
 *  Created on: Jan 30, 2016
 *      Author: rob
 */
#include <string.h>
#include <xtensa/config/core.h>
#include "xos.h"
#include <xtensa/xtutil.h>
#include "urpc_server.h"
#include "urpc.h"
#include "urpc_client.h"
//#include "FreeRTOS.h"
//#include "queue.h"
//#include "semphr.h"

#define URPC_ASYNC_QSIZE     16
#define URPC_SYNC_QSIZE      2
#define URPC_SYNC_TIMEOUT    500   //ms
#define URPC_SERVER_STACK_SIZE  2048

//static QueueHandle_t async_queue, sync_queue;
XOS_MSGQ_ALLOC(async_queue, URPC_ASYNC_QSIZE, sizeof(urpc_frame));
XOS_MSGQ_ALLOC(sync_queue, URPC_SYNC_QSIZE, sizeof(urpc_frame));
static urpc_frame recv_frame __attribute__((aligned(32)));
static uint8_t server_state = URPC_STATE_RESET;
static uint8_t session_id = 0;
static urpc_cb *rpc_cb = NULL;

int8_t urpc_accept(urpc_server_stub* stub)
{
	if(server_state < URPC_STATE_IDLE) {
		return URPC_ERROR;
	}
	int8_t err = stub->accept((urpc_server*)NULL, URPC_CONN_SERVER, NULL);
	if(err == URPC_SUCCESS) {
		server_state = URPC_STATE_CONN;
	}
    return err;
}

//static void urpc_sync_task_server(void *param)
static int32_t urpc_sync_task_server(void * arg, int32_t unused)
{
	urpc_frame frame;
	int8_t ret, chn;

	while(1) {
//		if(xQueueReceive(sync_queue, &(frame), portMAX_DELAY) == pdPASS) { // receive a message
		if(xos_msgq_get(sync_queue, (uint32_t *)&frame) == XOS_OK) {
		    ((urpc_server_stub *)arg)->_peek(URPC_CONN_SERVER, (uint8_t *)&recv_frame, 0); // receive next package
		    ret = _urpc_handle_rpc(&frame, rpc_cb);
			if(ret) {
				// error handle
				chn = frame.header.eps;
				if(rpc_cb->endpoints[chn].handles[0].handler) {
					recv_frame.header.magic = ret;
					(rpc_cb->endpoints[chn].handles[0].handler)(&recv_frame);
				}
			}
		}
	}
	return 0;
}

//static void urpc_async_task_server(void *param)
static int32_t urpc_async_task_server(void * arg, int32_t unused)
{
	urpc_frame frame;
	int8_t ret, chn;

	while(1) {
//		if(xQueueReceive(async_queue, &(frame), portMAX_DELAY) == pdPASS) { // receive a message
		if(xos_msgq_get(async_queue, (uint32_t *)&frame) == XOS_OK) {
		    ((urpc_server_stub *)arg)->_peek(URPC_CONN_SERVER, (uint8_t *)&recv_frame, 0); // receive next package
			ret = _urpc_handle_rpc(&frame, rpc_cb);
			if(ret) {
				// error handle
				chn = frame.header.eps;
				if(rpc_cb->endpoints[chn].handles[0].handler) {
					recv_frame.header.magic = ret;
					(rpc_cb->endpoints[chn].handles[0].handler)(&recv_frame);
				}
			}
		}
	}
	return 0;
}

int8_t _urpc_recv_notify_server(urpc_server_stub* stub, urpc_connection* conn, uint8_t chn)
{
    int32_t resch, ret, err = 0;
	
	do {
		if(chn != recv_frame.header.eps) {
			err = URPC_INVALID_EP;
			break;
		}
		if(recv_frame.header.flags & URPC_FLAG_SYNC) {   // sync channel
//		xQueueSendFromISR(sync_queue, (uint8_t *)(&recv_frame), NULL);
		    ret = xos_msgq_put(sync_queue, (uint32_t *)&recv_frame);
	    } else {   // sync channel
//		xQueueSendFromISR(async_queue, (uint8_t *)(&recv_frame), NULL);
		    ret = xos_msgq_put(async_queue, (uint32_t *)&recv_frame);
        }

		if(ret != XOS_OK) {
			err = URPC_QUE_FULL;
		}
	} while(0);

	if(err) {
		if(rpc_cb->endpoints[chn].handles[0].handler) {
			recv_frame.header.magic = err;
			(rpc_cb->endpoints[chn].handles[0].handler)(&recv_frame);
		}
	}
	// receive next one
	if(recv_frame.header.flags & URPC_FLAG_SYNC) {
		ret = xos_msgq_full(sync_queue);
	} else {
		ret = xos_msgq_full(async_queue);
	}

	if(!ret) {
		urpc_recv((urpc_stub*)stub, URPC_CONN_SERVER, &recv_frame);
	}
	return ret;
}

int8_t urpc_send_sync_server(urpc_server_stub* stub, urpc_frame* frame)
{
	int8_t err;

	//session_id should be the same as request, do not change it in frame
	frame->header.flags = URPC_FLAG_NOERR | URPC_FLAG_RESPONSE | URPC_FLAG_SYNC;
	err = urpc_send_check(rpc_cb, frame);
	if(err) {
		return err;
	}

    return urpc_send((urpc_stub*)stub, URPC_CONN_SERVER, frame);
}

int8_t urpc_send_async_server(urpc_server_stub* stub, urpc_frame* frame)
{
	int8_t err;
	uint32_t ps;
	ps  = xos_disable_interrupts();
	frame->header.session = ++session_id;
	xos_restore_interrupts(ps);
	frame->header.flags = URPC_FLAG_NOERR | URPC_FLAG_RESPONSE | URPC_FLAG_ASYNC;
	err = urpc_send_check(rpc_cb, frame);
	if(err) {
		return err;
	}

    return urpc_send((urpc_stub*)stub, URPC_CONN_SERVER, frame);
}

int8_t urpc_push_event_server(urpc_server_stub* stub, urpc_frame* frame)
{
	int8_t err;

	frame->header.session = ++session_id;
	frame->header.flags = URPC_FLAG_PUSH | URPC_FLAG_RESPONSE | URPC_FLAG_ASYNC;
	err = urpc_send_check(rpc_cb, frame);
	if(err) {
		return err;
	}

    return urpc_send((urpc_stub*)stub, URPC_CONN_SERVER, frame);
}

int8_t urpc_init_server(urpc_server_stub* stub, urpc_cb *ctrl_block)
{
//	static StackType_t xStack[URPC_SERVER_STACK_SIZE], xStack_a[URPC_SERVER_STACK_SIZE];
//	static StaticTask_t xTaskBuffer, xTaskBuffer_a;
	static uint8_t xStack[URPC_SERVER_STACK_SIZE], xStack_a[URPC_SERVER_STACK_SIZE];
	static XosThread xTaskBuffer, xTaskBuffer_a;
	server_state = URPC_STATE_RESET;
	if(urpc_init(ctrl_block)) {
		return URPC_ERROR;
	}
	rpc_cb = ctrl_block;
//	async_queue = xQueueCreate(URPC_ASYNC_QSIZE, sizeof(urpc_frame));
	xos_msgq_create(async_queue, URPC_ASYNC_QSIZE, sizeof(urpc_frame), XOS_MSGQ_WAIT_PRIORITY);
//	sync_queue = xQueueCreate(URPC_SYNC_QSIZE, sizeof(urpc_frame));
	xos_msgq_create(sync_queue, URPC_SYNC_QSIZE, sizeof(urpc_frame), XOS_MSGQ_WAIT_PRIORITY);

//	xTaskCreateStatic(urpc_sync_task_server, "rpc_sync_serv",
//			URPC_SERVER_STACK_SIZE, NULL, 23, xStack, &xTaskBuffer);
	xos_thread_create(&xTaskBuffer, 0, urpc_sync_task_server, stub, "rpc_sync_serv",
			xStack, URPC_SERVER_STACK_SIZE, 13, 0, 0);
//	xTaskCreateStatic(urpc_async_task_server, "rpc_async_serv",
//			URPC_SERVER_STACK_SIZE, NULL, 24, xStack_a, &xTaskBuffer_a);
	xos_thread_create(&xTaskBuffer_a, 0, urpc_async_task_server, stub, "rpc_async_serv",
			xStack_a, URPC_SERVER_STACK_SIZE, 14, 0, 0);
	urpc_recv((urpc_stub*)stub, URPC_CONN_SERVER, &recv_frame);
	server_state = URPC_STATE_IDLE;

    return stub->init_server((urpc_server*)NULL, ctrl_block->endpoints);
}
