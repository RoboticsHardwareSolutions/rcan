#include "rcan_hs_bus.h"


void rcan_hs_bus_transmit_next(struct can_bus* bus)
{
    CAN_HandleTypeDef *hcan = bus->hcan;
    struct can_node *node = bus->current_node;

    bool any_mbox_empty =
            __HAL_CAN_GET_FLAG(hcan, CAN_FLAG_TME0) ||
            __HAL_CAN_GET_FLAG(hcan, CAN_FLAG_TME1) ||
            __HAL_CAN_GET_FLAG(hcan, CAN_FLAG_TME2);

    if (!any_mbox_empty) {
        return;
    }

    uint32_t now = HAL_GetTick();

    for (;;) {
        if (now - node->last_tx_at > node->allowed_sending_period) {
            size_t bytes_available = can_node_stream_data_available(&node->request_buffer);
            if (bytes_available > sizeof(size_t)) {
                break;
            }
        }

        node = can_node_next_instance(node);
        if (node == bus->current_node) {
            return;
        }
    }

    node->last_tx_at = HAL_GetTick();

    CAN_TxHeaderTypeDef header;

#pragma pack(push)
#pragma pack(1)
    struct {
        uint16_t can_id;
        uint8_t data[CAN_NODE_MAX_PACKET_SIZE];
    } data_portion;
#pragma pack(pop)

    size_t size;
    can_node_stream_pop(&node->request_buffer, &size, sizeof(size_t));
    can_node_stream_pop(&node->request_buffer, &data_portion, sizeof(uint16_t) + size);

    header.StdId = data_portion.can_id;
    header.RTR = CAN_RTR_DATA;
    header.IDE = CAN_ID_STD;
    header.DLC = size;

    uint32_t mbox;
    if(HAL_CAN_AddTxMessage(hcan, &header, data_portion.data, &mbox) != HAL_OK) {
        node->error = can_node_error_tx;
        return;
    }

    bus->send_packet_counter = (bus->send_packet_counter + 1) % 10;

    node->last_tx_in_progress = true;
    node->last_tx_mbox = mbox;
    bus->current_node = can_node_next_instance(node);
}