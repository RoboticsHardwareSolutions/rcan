#ifndef FIRMWARE_CAN_NODE_STREAM_FREERTOS_H
#define FIRMWARE_CAN_NODE_STREAM_FREERTOS_H

#include <stdbool.h>

#include "FreeRTOS.h"
#include "stream_buffer.h"

struct can_node_stream_freertos {
    StaticStreamBuffer_t xstream;
    StreamBufferHandle_t handle;
};

void can_node_stream_freertos_create(struct can_node_stream_freertos *stream, uint8_t *memory, size_t size);
void can_node_stream_freertos_reset(struct can_node_stream_freertos *stream);
size_t can_node_stream_freertos_push(struct can_node_stream_freertos *stream, uint8_t *data, size_t size);
size_t can_node_stream_freertos_pop(struct can_node_stream_freertos *stream, uint8_t *data, size_t size);
size_t can_node_stream_freertos_push_isr(struct can_node_stream_freertos *stream, uint8_t *data, size_t size);
size_t can_node_stream_freertos_pop_isr(struct can_node_stream_freertos *stream, uint8_t *data, size_t size);
size_t can_node_stream_freertos_data_available(struct can_node_stream_freertos *stream);
size_t can_node_stream_freertos_space_left(struct can_node_stream_freertos *stream);
bool can_node_stream_freertos_is_full(struct can_node_stream_freertos *stream);
bool can_node_stream_freertos_is_empty(struct can_node_stream_freertos *stream);

#endif //FIRMWARE_CAN_NODE_STREAM_FREERTOS_H
