#pragma once

#if defined(STM32F767xx) || defined(STM32F765xx) || defined(STM32F072xB) || defined(STM32F091xC) || defined(STM32F103xB) || defined(STM32F407xx) || defined(STM32F429xx)

#    include "stdbool.h"
#    include "rcan_def.h"
#    include "rcan.h"
#    include "rcan_timing.h"
#    include "rcan_filter.h"

#    if defined(STM32F103xB)

#        include "stm32f1xx_hal.h"

#    endif  // defined(STM32F103xB)

#    if defined(STM32F767xx) || defined(STM32F765xx)

#        include "stm32f7xx_hal.h"

#    endif  // defined(STM32F103xB)

#    if defined(STM32F072xB) || defined(STM32F091xC)

#        include "stm32f0xx_hal.h"

#    endif  // defined(STM32F103xB)

#    if defined(STM32F407xx) || defined(STM32F429xx)

#        include "stm32f4xx_hal.h"

#    endif

struct can_iface
{
    CAN_HandleTypeDef handle;
    rcan_timing       timing;
    rcan_filter       filter;
    bool              use_filter;
};

typedef struct can_iface rcan;

bool bx_can_filter_preconfiguration(rcan* can, uint32_t* accepted_ids, uint32_t size);

bool bx_can_start(rcan* can, uint32_t channel, uint32_t bitrate);

bool bx_can_stop(rcan* can);

bool bx_can_is_ok(rcan* can);

bool bx_can_send(rcan* can, rcan_frame* frame);

bool bx_can_receive(rcan* can, rcan_frame* frame);

#endif  // defined(STM32F767xx) || defined(STM32F765xx) || defined(STM32F072xB) || defined(STM32F091xC) ||
        // defined(STM32F103xB)
