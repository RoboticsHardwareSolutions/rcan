#pragma once

#if defined(STM32F767xx) || defined(STM32F765xx) || defined(STM32F072xB) || defined(STM32F091xC) || \
    defined(STM32F103xB) || defined(STM32F407xx) || defined(STM32F429xx) || defined(STM32F103xE)

#    include "stdbool.h"
#    include "rcan_def.h"
#    include "rcan.h"
#    include "rcan_timing.h"
#    include "rcan_filter.h"

#    if defined(STM32F103xB) || defined(STM32F103xE)

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

typedef enum
{
    CE_OK,
    CE_EPV,
    /*
     * Error Passive Flag
     * Set when the Error Passive limit has been reached
     * (Receive Error Counter or Transmit Error Counter greater than 127).
     * This Flag is cleared only by hardware.
     */
    CE_XMTFULL, /* Transmit buffer in CAN controller is full */
    CE_OVERRUN, /* CAN controller was read too late */
    CE_STATE_ERROR,
    CE_ERRI,
    CE_SOME_REC,
    CE_SOME_LEC,
    CE_SOME_TEC,
} can_errors_t;

struct can_iface
{
    CAN_HandleTypeDef handle;
    rcan_timing       timing;
    rcan_filter       filter;
    bool              use_filter;
    uint16_t          errors;
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
