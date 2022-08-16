#ifndef __RCAN_DEF_H
#define __RCAN_DEF_H


#include "rcan_filter.h"
#include "rcan_timing.h"

#define RCAN_MAX_FRAME_PAYLOAD_SIZE                    8

typedef enum {
    nonframe,
    std_id,
    ext_id
} rcan_frame_type;

#pragma pack(push)
#pragma pack(1)

typedef struct {
    uint32_t id;
    uint8_t len;
    rcan_frame_type type;
    bool rtr;
    uint8_t payload[RCAN_MAX_FRAME_PAYLOAD_SIZE];
} rcan_frame;

#pragma pack(pop)


#define RCAN_EXT_ID_MAX                      0x1FFFFFFFU
#define RCAN_STD_ID_MAX                      0x000007FFU


#define SOCKET_CAN0  0x1224UL// macro for can socet can iface
#define SOCKET_CAN1  0x1225UL
#define SOCKET_CAN2  0x1226UL

#define SOCKET_VCAN0  0x1234UL // macro for vcan socet can iface
#define SOCKET_VCAN1  0x1235UL
#define SOCKET_VCAN2  0x1236UL


#if defined(RCAN_MACOS)

#include "PCBUSB.h"
#include "u_can.h"

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
#include "u_can.h"

#endif // defined(RCAN_UNIX)

#if defined(RCAN_WINDOWS)

#include "windows.h"
#include "PCANBasic.h"
#include "u_can.h"

#endif // defined(RCAN_UNIX)


#if defined(STM32F767xx) || defined(STM32F765xx) || defined(STM32F072xB) || defined(STM32F091xC) || defined(STM32F103xB)

#include "bx_can.h"

#endif //defined(STM32F767xx) || defined(STM32F765xx) || defined(STM32F072xB) || defined(STM32F091xC) || defined(STM32F103xB)


#if defined(STM32G474xx)

#include "bx_canfd.h"

#endif


#endif // __RCAN_DEF_H



