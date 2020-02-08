#include "can_node.h"

#include <memory.h>
#include <stdio.h>

#include "semphr.h"
#include "task.h"

static struct can_node* can_node_next_instance(struct can_node* node);

static void can_node_rx_msg_pending_cb(CAN_HandleTypeDef *hcan, uint32_t rx_fifo);
static void can_node_tx_msg_complete_cb(CAN_HandleTypeDef *hcan, uint32_t tx_fifo);

static void can_node_rx_fifo0_msg_pending_cb(CAN_HandleTypeDef *hcan);
static void can_node_rx_fifo1_msg_pending_cb(CAN_HandleTypeDef *hcan);
static void can_node_tx_mbox0_msg_complete_cb(CAN_HandleTypeDef *hcan);
static void can_node_tx_mbox1_msg_complete_cb(CAN_HandleTypeDef *hcan);
static void can_node_tx_mbox2_msg_complete_cb(CAN_HandleTypeDef *hcan);
static void can_node_error_cb(CAN_HandleTypeDef *hcan);

static void can_node_dump(uint8_t *data, size_t size);

static struct {
    struct can_bus *first_bus;
    struct can_bus *last_bus;
} ci_instance = {0};

struct can_node_cb_record {
    HAL_CAN_CallbackIDTypeDef id;
    void (* cb)(CAN_HandleTypeDef *hcan);
};

static struct can_node_cb_record can_bus_cb_records[] = {
        { HAL_CAN_RX_FIFO0_MSG_PENDING_CB_ID, can_node_rx_fifo0_msg_pending_cb },
        { HAL_CAN_RX_FIFO1_MSG_PENDING_CB_ID, can_node_rx_fifo1_msg_pending_cb },
        { HAL_CAN_TX_MAILBOX0_COMPLETE_CB_ID, can_node_tx_mbox0_msg_complete_cb },
        { HAL_CAN_TX_MAILBOX1_COMPLETE_CB_ID, can_node_tx_mbox1_msg_complete_cb },
        { HAL_CAN_TX_MAILBOX2_COMPLETE_CB_ID, can_node_tx_mbox2_msg_complete_cb },
        { HAL_CAN_ERROR_CB_ID, can_node_error_cb },
};

void can_bus_activate(struct can_bus *bus)
{
    CAN_HandleTypeDef *hcan = bus->hcan;

    int record_count = sizeof(can_bus_cb_records) / sizeof(can_bus_cb_records[0]);
    for (int i = 0; i < record_count; i++) {
        if (HAL_CAN_RegisterCallback(hcan, can_bus_cb_records[i].id, can_bus_cb_records[i].cb) != HAL_OK) {
            bus->error = can_bus_error_init;
            return;
        }
    }

    HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING |
                                       CAN_IT_RX_FIFO1_MSG_PENDING |
                                       CAN_IT_TX_MAILBOX_EMPTY);

    CAN_FilterTypeDef filter;
    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0x0000;
    filter.FilterIdLow = 0x0000;
    filter.FilterMaskIdHigh = 0x0000;
    filter.FilterMaskIdLow = 0x0000;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0 | CAN_RX_FIFO1;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14;
    HAL_CAN_ConfigFilter(hcan, &filter);

    HAL_CAN_Start(hcan);
}

void can_node_add(struct can_node *node, CAN_HandleTypeDef *hcan, void *user_data, uint16_t allowed_request_period) {
    struct can_bus* bus = can_bus_by_hcan(hcan);

    node->bus = bus;
    node->user_data = user_data;
    node->cb_pairs_count = 0;
    node->error = can_node_error_none;
    node->bus_error_counter = 0;
    node->allowed_sending_period = allowed_request_period;
    node->last_tx_in_progress = false;

    memset(node->cb_pairs, 0, sizeof(node->cb_pairs));

    can_node_stream_create(&node->request_buffer, node->request_buffer_mem, CAN_NODE_BUFFER_SIZE);

    if (bus->first_node == NULL) {
        bus->first_node = node;
    } else {
        bus->last_node->next = node;
    }
    bus->last_node = node;

    if (bus->current_node == NULL) {
        bus->current_node = node;
    }
}


void can_node_register_cb(struct can_node *node, uint16_t can_id, can_node_cb cb)
{
    node->cb_pairs[node->cb_pairs_count].receive_can_id = can_id;
    node->cb_pairs[node->cb_pairs_count++].cb = cb;
}

struct can_node *can_node_by_id(CAN_HandleTypeDef *hcan, uint16_t can_id, can_node_cb *cb)
{
    struct can_bus *bus = can_bus_by_hcan(hcan);

    if (bus == NULL) {
        return NULL;
    }

    if (bus->first_node == NULL) {
        return NULL;
    }

    struct can_node *node = bus->first_node;
    do {
        if (node->send_can_id == can_id) {
            return node;
        }

        for (int i = 0; i < node->cb_pairs_count; i++ ) {
            if (node->cb_pairs[i].receive_can_id == can_id) {
                *cb = node->cb_pairs[i].cb;
                return node;
            }
        }

        node = node->next;
    } while (node != NULL);

    return NULL;
}

void can_node_rx_fifo0_msg_pending_cb(CAN_HandleTypeDef *hcan)
{
    can_node_rx_msg_pending_cb(hcan, CAN_RX_FIFO0);
}

void can_node_rx_fifo1_msg_pending_cb(CAN_HandleTypeDef *hcan)
{
    can_node_rx_msg_pending_cb(hcan, CAN_RX_FIFO1);
}

void can_node_rx_msg_pending_cb(CAN_HandleTypeDef *hcan, uint32_t rx_fifo)
{
    CAN_RxHeaderTypeDef header;
    uint8_t data[8];

    HAL_CAN_GetRxMessage(hcan, rx_fifo, &header, data);

    if (header.IDE == CAN_ID_STD) {
        can_node_cb callback = NULL;
        struct can_node *node = can_node_by_id(hcan, header.StdId, &callback);

        if (node != NULL) {
            struct can_bus* bus = node->bus;

            node->last_received_at = HAL_GetTick();
            bus->recv_packet_counter = (bus->recv_packet_counter + 1) % 10;

            if (callback != NULL) {
                callback(node, data, header.DLC);
            }
        }
    }

//    can_bus_transmit_next(hcan);
}

static void can_node_tx_mbox0_msg_complete_cb(CAN_HandleTypeDef *hcan)
{
    can_node_tx_msg_complete_cb(hcan, CAN_TX_MAILBOX0);
}

static void can_node_tx_mbox1_msg_complete_cb(CAN_HandleTypeDef *hcan)
{
    can_node_tx_msg_complete_cb(hcan, CAN_TX_MAILBOX1);
}

static void can_node_tx_mbox2_msg_complete_cb(CAN_HandleTypeDef *hcan)
{
    can_node_tx_msg_complete_cb(hcan, CAN_TX_MAILBOX2);
}

void can_node_tx_msg_complete_cb(CAN_HandleTypeDef *hcan, uint32_t tx_mbox)
{
    struct can_bus* bus = can_bus_by_hcan(hcan);
    struct can_node *node = bus->first_node;

    do {
        if (node->last_tx_mbox = tx_mbox) {
            node->last_tx_in_progress = false;
        }

        node = node->next;
    } while (node != NULL);

//    can_bus_transmit_next(hcan);
}

void can_bus_transmit_next(struct can_bus* bus)
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

void can_node_dump(uint8_t *data, size_t size)
{
    for (int i = 0; i < size; i++) {
        printf("%.2X ", data[i]);
    }

    printf("\n\n");
}

void can_node_transmit(struct can_node *node, uint16_t can_id, uint8_t *data, size_t size)
{
    if (node->error != can_node_error_none) {
        return;
    }

    int bytes_left = can_node_stream_space_left(&node->request_buffer);
    if (bytes_left < size + sizeof(size_t)) {
        node->error = can_node_error_buffer_overrun;
        return;
    }

#pragma pack(push)
#pragma pack(1)
    struct {
        size_t size;
        uint16_t can_id;
        uint8_t data[CAN_NODE_MAX_PACKET_SIZE];
    } data_portion;
#pragma pack(pop)

    data_portion.size = size;
    data_portion.can_id = can_id;
    memcpy(data_portion.data, data, size);

    can_node_stream_push(&node->request_buffer, &data_portion, sizeof(size_t) + sizeof(uint16_t) + size);
//    can_bus_transmit_next(node->hcan);
}

struct can_node *can_node_next_instance(struct can_node *node)
{
    struct can_bus *bus = node->bus;
    return node == bus->last_node ? bus->first_node : node->next;
}

void can_node_error_cb(CAN_HandleTypeDef *hcan)
{
    struct can_bus *bus = can_bus_by_hcan(hcan);
    struct can_node *node = bus->first_node;

    bus->error = can_bus_error_callback;
    node->error = can_node_error_bus;

    do {
        node->error = can_node_error_bus;
        node = node->next;
    } while (node != NULL);
}

bool can_node_all_done(struct can_node *node)
{
    int bytes_available = can_node_stream_data_available(&node->request_buffer);
    return bytes_available == 0;
}

void can_bus_reset(struct can_bus *bus) {
    bus->error = can_bus_error_none;

    HAL_CAN_ResetError(bus->hcan);

    struct can_node *node = bus->first_node;
    do {
        node->error = can_node_error_none;
        can_node_stream_reset(&node->request_buffer);
        node->bus_error_counter = 0;

        HAL_CAN_AbortTxRequest(bus->hcan, node->last_tx_mbox);

        node = node->next;
    } while (node != NULL);
}

void can_bus_add(struct can_bus *bus, CAN_HandleTypeDef *hcan) {
    bus->hcan = hcan;
    bus->current_node = bus->first_node = bus->last_node = NULL;

    bus->recv_packet_counter = bus->send_packet_counter = 0;

    if (ci_instance.first_bus == NULL) {
        ci_instance.first_bus = bus;
    } else {
        ci_instance.last_bus->next = bus;
    }
    ci_instance.last_bus = bus;
}

struct can_bus *can_bus_by_hcan(CAN_HandleTypeDef *hcan) {
    if (ci_instance.first_bus == NULL) {
        return NULL;
    }

    struct can_bus *bus = ci_instance.first_bus;
    do {
        if (bus->hcan->Instance == hcan->Instance) {
            return bus;
        }

        bus = bus->next;
    } while (bus != NULL);

    return NULL;
}

void can_node_check_timeout(struct can_node *node, uint32_t timeout)
{
    if (!can_node_all_done(node)) {
        if (HAL_GetTick() - node->last_tx_at > timeout) {
            node->error = can_node_error_timeout;
        }
    }
}


void can_bus_loop_all() {
    struct can_bus *bus = ci_instance.first_bus;
    do {
        can_bus_transmit_next(bus);
        bus = bus->next;
    } while (bus != NULL);
}

void can_bus_activate_all() {
    struct can_bus *bus = ci_instance.first_bus;
    do {
        can_bus_activate(bus);
        bus = bus->next;
    } while (bus != NULL);
}

bool can_node_has_error(struct can_node *node) {
    return node->error != can_node_error_none;
}