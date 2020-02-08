#ifndef _RCAN_BUS_H
#define _RCAN_BUS_H


typedef enum {
    rcan_bus_error_none,
    rcan_bus_error_init,
    rcan_bus_error_callback
}rcan_bus_error;

typedef enum{
    rcan_br_125kbs,
    rcan_br_250kbs,
    rcan_br_500kbs,
    rcan_br_1mbs
}rcan_bit_rates;


typedef struct  {

#if   RCAN_USE_HAL_CAN_STM32_HS
    CAN_HandleTypeDef *hscan;
#elif RCAN_USE_HAL_CAN_STM32_FD
    FDCAN_HandleTypeDef *fdcan;
#endif
    rcan_node *current_node;
    rcan_node *first_node;
    rcan_node *last_node;

    rcan_bus_error error;

    uint64_t send_packet_counter;
    uint64_t recv_packet_counter;

}rcan_bus;


typedef struct{
    uint32_t id;
    uint8_t len;
}rcan_header;

typedef struct {
    rcan_header header;
    uint8_t *payload;
}rcan_frame;



#endif