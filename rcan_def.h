#ifndef __DEFINES_H
#define __DEFINES_H


#if defined(RCAN_MACOS)
#include "PCBUSB.h"
#endif

#if defined(RCAN_UNIX)
#include "PCANBasic.h"
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <libsocketcan.h>

#define SOCET_CAN0  0x1224 // macro for can socet can iface
#define SOCET_CAN1  0x1225
#define SOCET_CAN2  0x1226

#define SOCET_VCAN0  0x1234 // macro for vcan socet can iface
#define SOCET_VCAN1  0x1235
#define SOCET_VCAN2  0x1236

#endif

#if defined(RCAN_WINDOWS)
#include <minwindef.h>
#include "PCANBasic.h"
#endif

#if defined(STM32G474xx)
#include "stm32g4xx_hal.h"
#endif

#include "rcan_filter.h"
#include "rcan_timing.h"

#if defined(RCAN_WINDOWS) || defined(RCAN_UNIX) || defined (RCAN_MACOS)

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



