#ifndef __RCAN_BUS_H
#define __RCAN_BUS_H

#include "stdbool.h"
#include "rcan_node.h"



typedef struct {
    FDCAN_HandleTypeDef can_handle;

}rcan;

typedef struct {
    uint32_t id;
    uint8_t len;
    uint8_t *payload;
} can_frame;


bool can_start(can_dev *dev, uint32_t chanel, uint32_t baudrate);

bool can_is_ok(can_dev *dev);

void can_stop(can_dev *dev);

void can_stop_all(void);

bool can_write(can_dev *dev, can_frame *frame);

bool can_read(can_dev *dev, can_frame *frame);

void can_view_frame(can_frame *frame);


#endif