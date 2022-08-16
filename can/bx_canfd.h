#ifndef  __BX_CANFD_H_
#define  __BX_CANFD_H_

#if defined(STM32G474xx)

#include "stdbool.h"
#include "rcan_def.h"
#include "rcan_timing.h"
#include "rcan.h"
#include "rcan_filter.h"



#if defined(STM32G474xx)

#include "stm32g4xx_hal.h"

#endif


struct can_iface {
    FDCAN_HandleTypeDef handle;
    rcan_timing timing;
    rcan_filter filter;
    bool use_filter;
};

typedef struct can_iface rcan;

 bool bx_canfd_filter_preconfiguration(rcan *can, uint32_t *accepted_ids, uint32_t size);

 bool bx_canfd_start(rcan *can, uint32_t channel, uint32_t bitrate);

 bool bx_canfd_is_ok(rcan *can);

 bool bx_canfd_stop(rcan *can);

 bool bx_canfd_send(rcan *can, rcan_frame *frame);

 bool bx_canfd_receive(rcan *can, rcan_frame *frame);

#endif // defined(STM32G474xx)

#endif // __BX_CANFD_H_
