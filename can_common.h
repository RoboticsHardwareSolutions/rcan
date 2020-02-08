#ifndef FIRMWARE_CAN_COMMON_H
#define FIRMWARE_CAN_COMMON_H

#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_can.h"

void can_init(CAN_HandleTypeDef *hcan0, CAN_HandleTypeDef *hcan1);

#endif //FIRMWARE_CAN_COMMON_H
