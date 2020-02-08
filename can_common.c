//
// Created by akapustin on 05.12.2019.
//

#include "can_common.h"
#include "can_node.h"

static struct can_bus can0;
static struct can_bus can1;

void can_init(CAN_HandleTypeDef *hcan0, CAN_HandleTypeDef *hcan1) {
    can_bus_add(&can0, hcan0);
    can_bus_add(&can1, hcan1);
}
