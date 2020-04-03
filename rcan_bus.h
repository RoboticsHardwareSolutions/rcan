//#ifndef __RCAN_BUS_H
//#define __RCAN_BUS_H
//
//#include "stdbool.h"
//#include "rcan_node.h"
//
//typedef enum {
//    rcan_bus_error_none,
//    rcan_bus_error_init,
//    rcan_bus_error_callback
//} rcan_bus_error;
//
//typedef struct {
//    FDCAN_HandleTypeDef *fdcan;
//    //rcan_node *current_node;
//    //rcan_node *first_node;
//    //rcan_node *last_node;
//    rcan_bus_error error;
//    uint64_t send_packet_counter;
//    uint64_t recv_packet_counter;
//    //rcan_bus *next_bus;
//} rcan_bus;
//
//typedef enum {
//    rcan_extended,
//    rcan_std,
//} rcan_bus_frame_type;
//
//typedef struct {
//    uint32_t id;
//    rcan_bus_frame_type type;
//    uint8_t len;
//} rcan_header;
//
//typedef struct {
//    rcan_header header;
//    uint8_t *payload;
//} rcan_frame;
//
//
//typedef enum {
//    rcan_speed_125kb,
//    rcan_speed_250kb,
//    rcan_speed_500kb,
//    rcan_speed_1mb
//} rcan_speed;
//
//
//#endif