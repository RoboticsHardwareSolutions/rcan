#ifndef FIRMWARE_CAN_NODE_H
#define FIRMWARE_CAN_NODE_H

#include <stdint.h>
#include <stdbool.h>

#include <stdint.h>
#include <stdbool.h>

#include "can_node_port.h"
#include "can_node_config.h"

enum can_node_error {
    can_node_error_none,
    can_node_error_bus,
    can_node_error_tx,
    can_node_error_init,
    can_node_error_buffer_overrun,
    can_node_error_timeout
};

enum can_bus_error {
    can_bus_error_none,
    can_bus_error_init,
    can_bus_error_callback
};

struct can_bus;
struct can_node;

typedef void (*can_node_cb)(struct can_node *node, uint8_t *data, size_t size);

struct can_node_cb_pair {
    uint16_t receive_can_id;
    can_node_cb cb;
};

struct can_bus {
    CAN_HandleTypeDef *hcan;

    struct can_node *current_node;
    struct can_node *first_node;
    struct can_node *last_node;

    struct can_bus *next;

    enum can_bus_error error;

    uint8_t send_packet_counter;
    uint8_t recv_packet_counter;
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

void can_bus_add(struct can_bus *bus, CAN_HandleTypeDef *hcan);
struct can_bus *can_bus_by_hcan(CAN_HandleTypeDef *hcan);
void can_bus_transmit_next(struct can_bus* bus);

void can_node_add(struct can_node *node, CAN_HandleTypeDef *hcan, void *user_data, uint16_t allowed_request_period);
void can_node_register_cb(struct can_node *node, uint16_t can_id, can_node_cb cb);

struct can_node *can_node_by_id(CAN_HandleTypeDef *hcan, uint16_t can_id, can_node_cb *cb);
void can_node_transmit(struct can_node *node, uint16_t can_id, uint8_t *data, size_t size);
bool can_node_all_done(struct can_node *node);
bool can_node_has_error(struct can_node *node);

void can_bus_activate_all();
void can_bus_loop_all();
void can_bus_reset(struct can_bus *bus);

#endif //FIRMWARE_CAN_NODE_H
