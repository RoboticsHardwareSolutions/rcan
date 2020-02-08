#ifndef __RCAN_NODE_H
#define __RCAN_NODE_H

enum can_node_error {
    can_node_error_none,
    can_node_error_bus,
    can_node_error_tx,
    can_node_error_init,
    can_node_error_buffer_overrun,
    can_node_error_timeout
};

struct can_node;

typedef void (*can_node_cb)(struct can_node *node, uint8_t *data, size_t size);

struct can_node_cb_pair {
    uint16_t receive_can_id;
    can_node_cb cb;
};

struct can_node {
    uint16_t send_can_id;

    struct can_node_cb_pair cb_pairs[CAN_NODE_MAX_CB_PAIRS];
    int cb_pairs_count;

    uint32_t allowed_sending_period;

    volatile uint32_t last_tx_at;
    volatile uint32_t last_tx_mbox;
    volatile bool last_tx_in_progress;

    uint32_t last_received_at;

    uint8_t request_buffer_mem[CAN_NODE_BUFFER_SIZE];
    struct can_node_stream request_buffer;

    struct can_bus *bus;

    enum can_node_error error;
    unsigned int bus_error_counter;

    void *user_data;

    struct can_node *next;
};

void can_node_add(struct can_node *node, CAN_HandleTypeDef *hcan, void *user_data, uint16_t allowed_request_period);
void can_node_register_cb(struct can_node *node, uint16_t can_id, can_node_cb cb);

struct can_node *can_node_by_id(CAN_HandleTypeDef *hcan, uint16_t can_id, can_node_cb *cb);
void can_node_transmit(struct can_node *node, uint16_t can_id, uint8_t *data, size_t size);
bool can_node_all_done(struct can_node *node);
bool can_node_has_error(struct can_node *node);

#endif