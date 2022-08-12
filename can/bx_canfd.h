#ifndef  __BX_CANFD_H_
#define  __BX_CANFD_H_

#if defined(STM32G474xx)

#include "stdbool.h"
#include "stm32g4xx_hal.h"
#include "rcan_def.h"

inline bool bx_canfd_filter_preconfiguration(rcan *can, uint32_t *accepted_ids, uint32_t size);

inline bool bx_canfd_start(rcan *can, uint32_t channel, uint32_t bitrate);

inline bool bx_canfd_is_ok(rcan *can);

inline bool bx_canfd_stop(rcan *can);

inline bool bx_canfd_send(rcan *can, rcan_frame *frame);

inline bool bx_canfd_receive(rcan *can, rcan_frame *frame);

#endif // defined(STM32G474xx)

#endif
