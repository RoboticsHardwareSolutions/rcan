#ifndef __DEFINES_H
#define __DEFINES_H


#if defined(RCAN_MACOS)
#include "PCBUSB.h"
#endif

#if defined(UNIX)
#include "PCANBasic.h"
#endif

#if defined(WINDOWS)
#include <minwindef.h>
#include "PCANBasic.h"
#endif

#if defined(STM32G474xx)
#include "stm32g4xx_hal.h"
#endif

#include "rcan_filter.h"
#include "rcan_timing.h"

#if defined(WINDOWS) || defined(UNIX) || defined (RCAN_MACOS)

struct can_iface {
    uint32_t channel;
    int fd;
    bool opened; // FIXME real need it ?
    rcan_filter filter;
    bool use_filter;
};

#elif defined(STM32G474xx)

struct can_iface {
    FDCAN_HandleTypeDef handle;
    rcan_timing timing;
    rcan_filter filter;
    bool use_filter;
};

#endif

#endif



