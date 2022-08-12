#ifndef __DEFINES_H
#define __DEFINES_H


#include "rcan_filter.h"
#include "rcan_timing.h"


#define RCAN_EXT_ID_MAX                      0x1FFFFFFFU
#define RCAN_STD_ID_MAX                      0x000007FFU

#define RCAN_MAX_FRAME_PAYLOAD_SIZE                    8

#define SOCKET_CAN0  0x1224UL// macro for can socet can iface
#define SOCKET_CAN1  0x1225UL
#define SOCKET_CAN2  0x1226UL

#define SOCKET_VCAN0  0x1234UL // macro for vcan socet can iface
#define SOCKET_VCAN1  0x1235UL
#define SOCKET_VCAN2  0x1236UL


#if defined(RCAN_MACOS)

#include "PCBUSB.h"

#define PCAN_PCIBUS1 0
#define PCAN_PCIBUS2 0
#define PCAN_PCIBUS3 0

#endif // defined(RCAN_MACOS)

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

#endif // defined(RCAN_UNIX)

#if defined(RCAN_WINDOWS)

#include "windows.h"
#include "PCANBasic.h"

#endif // defined(RCAN_UNIX)


#if defined(STM32F072xB) || defined(STM32F091xC)

#include "stm32f0xx_hal.h"

#endif

#if defined(STM32F103xB)

#include "stm32f1xx_hal.h"

#endif

#if defined(STM32G474xx)

#include "stm32g4xx_hal.h"

#endif

#if defined(STM32F767xx) || defined(STM32F765xx)

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

#elif defined(STM32F072xB) || defined(STM32F091xC) || defined(STM32F103xB) || defined(STM32F767xx) || defined(STM32F765xx)

struct can_iface {
    CAN_HandleTypeDef handle;
    rcan_timing timing;
    rcan_filter filter;
    bool use_filter;
};

#endif

#endif



