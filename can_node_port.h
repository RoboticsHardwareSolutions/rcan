#ifndef FIRMWARE_CAN_NODE_PORT_H
#define FIRMWARE_CAN_NODE_PORT_H

#include "stm32f765xx.h"
#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_can.h"

#include "can_node_stream_freertos.h"

#define can_node_stream                 can_node_stream_freertos
#define can_node_stream_create          can_node_stream_freertos_create
#define can_node_stream_reset           can_node_stream_freertos_reset
#define can_node_stream_push            can_node_stream_freertos_push
#define can_node_stream_push_isr        can_node_stream_freertos_push_isr
#define can_node_stream_pop             can_node_stream_freertos_pop
#define can_node_stream_pop_isr         can_node_stream_freertos_pop_isr
#define can_node_stream_data_available  can_node_stream_freertos_data_available
#define can_node_stream_space_left      can_node_stream_freertos_space_left
#define can_node_stream_is_full         can_node_stream_freertos_is_full
#define can_node_stream_is_empty        can_node_stream_freertos_is_empty

#endif //FIRMWARE_CAN_NODE_PORT_H
