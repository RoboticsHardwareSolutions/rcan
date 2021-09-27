#ifndef __DEFINES_H
#define __DEFINES_H


#include "rcan_filter.h"
#include "rcan_timing.h"

#if defined(RCAN_MACOS)
#include "PCBUSB.h"
#endif

#if defined(RCAN_UNIX)
#include "PCANBasic.h"
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include "stdlib.h"

#include <linux/can.h>
#include <linux/can/raw.h>
#include <libsocketcan.h>
#include <fcntl.h>

#define SOCKET_CAN0  0x1224 // macro for can socet can iface
#define SOCKET_CAN1  0x1225
#define SOCKET_CAN2  0x1226

#define SOCKET_VCAN0  0x1234 // macro for vcan socet can iface
#define SOCKET_VCAN1  0x1235
#define SOCKET_VCAN2  0x1236

#endif

#if defined(RCAN_WINDOWS)
#include "windows.h"
#include "PCANBasic.h"
#endif

#if defined(STM32G474xx)
#include "stm32g4xx_hal.h"
#endif

#if defined(STM32F767xx)

#include "stm32f7xx_hal.h"

#endif

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

#elif defined(STM32F767xx)

struct can_iface {
    CAN_HandleTypeDef handle;
    rcan_timing timing;
    rcan_filter filter;
    bool use_filter;
};

#endif

#endif



