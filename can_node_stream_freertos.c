#include "can_node_stream_freertos.h"

#include <string.h>

void can_node_stream_freertos_create(struct can_node_stream_freertos *stream, uint8_t *memory, size_t size) {
    stream->handle = xStreamBufferCreateStatic(size, 1, memory, &stream->xstream);
}

void can_node_stream_freertos_reset(struct can_node_stream_freertos *stream) {
    xStreamBufferReset(stream->handle);
}

size_t can_node_stream_freertos_push(struct can_node_stream_freertos *stream, uint8_t *data, size_t size) {
    return xStreamBufferSend(stream->handle, data, size, 0);
}

size_t can_node_stream_freertos_pop(struct can_node_stream_freertos *stream, uint8_t *data, size_t size) {
    return xStreamBufferReceive(stream->handle, data, size, 0);
}

size_t can_node_stream_freertos_push_isr(struct can_node_stream_freertos *stream, uint8_t *data, size_t size) {
    return xStreamBufferSendFromISR(stream->handle, data, size, NULL);
}

size_t can_node_stream_freertos_pop_isr(struct can_node_stream_freertos *stream, uint8_t *data, size_t size) {
    return xStreamBufferReceiveFromISR(stream->handle, data, size, NULL);
}

size_t can_node_stream_freertos_data_available(struct can_node_stream_freertos *stream) {
    return xStreamBufferBytesAvailable(stream->handle);
}

size_t can_node_stream_freertos_space_left(struct can_node_stream_freertos *stream) {
    return xStreamBufferSpacesAvailable(stream->handle);
}

bool can_node_stream_freertos_is_full(struct can_node_stream_freertos *stream) {
    return xStreamBufferIsFull(stream->handle);
}

bool can_node_stream_freertos_is_empty(struct can_node_stream_freertos *stream) {
    return xStreamBufferIsEmpty(stream->handle);
}
