#ifndef __RCAN_DEF_H__
#define __RCAN_DEF_H__

#include "rcan_filter.h"
#include "rcan_timing.h"

#define RCAN_MAX_FRAME_PAYLOAD_SIZE 8

typedef enum
{
    nonframe,
    std_id,
    ext_id
} rcan_frame_type;

#pragma pack(push)
#pragma pack(1)

typedef struct
{
    uint32_t        id;
    uint8_t         len;
    rcan_frame_type type;
    bool            rtr;
    uint8_t         payload[RCAN_MAX_FRAME_PAYLOAD_SIZE];
} rcan_frame;

// TODO INIT ALLOC AND FREE for rcan frame : for init use define

#pragma pack(pop)

#define RCAN_EXT_ID_MAX 0x1FFFFFFFU
#define RCAN_STD_ID_MAX 0x000007FFU

/**
 * argument uint32_t channel in function rcan_start(..)
 */

#define PEAK_CAN_USBBUS1 0x51U  // PCAN-USB interface, channel 1
#define PEAK_CAN_USBBUS2 0x52U  // PCAN-USB interface, channel 2
#define PEAK_CAN_USBBUS3 0x53U  // PCAN-USB interface, channel 3

#define PEAK_CAN_PCIBUS1 0x41U  // PCAN-PCI interface, channel 1
#define PEAK_CAN_PCIBUS2 0x42U  // PCAN-PCI interface, channel 2
#define PEAK_CAN_PCIBUS3 0x43U  // PCAN-PCI interface, channel 3

#define SOCKET_CAN0 0x1224UL  // macro for can socet can iface
#define SOCKET_CAN1 0x1225UL
#define SOCKET_CAN2 0x1226UL

#define SOCKET_VCAN0 0x1234UL  // macro for vcan socet can iface
#define SOCKET_VCAN1 0x1235UL
#define SOCKET_VCAN2 0x1236UL

#define VIRTUAL_INPROC_CAN_BUS0 0x1237UL  // bus for inter thread communication
#define VIRTUAL_INPROC_CAN_BUS1 0x1238UL
#define VIRTUAL_INPROC_CAN_BUS2 0x1239UL

#define VIRTUAL_IPC_CAN_BUS0 0x1240UL  // bus for inter process communication
#define VIRTUAL_IPC_CAN_BUS1 0x1241UL
#define VIRTUAL_IPC_CAN_BUS2 0x1242UL

#if defined(RCAN_MACOS)

#    include "PCBUSB.h"
#    include "u_can.h"

#endif  // defined(RCAN_MACOS)

#if defined(RCAN_UNIX)

#    include "stdlib.h"
#    include "u_can.h"
#    include <PCANBasic.h>
#    include <fcntl.h>
#    include <libsocketcan.h>
#    include <linux/can.h>
#    include <linux/can/raw.h>
#    include <net/if.h>
#    include <sys/ioctl.h>
#    include <sys/socket.h>
#    include <unistd.h>

#endif  // defined(RCAN_UNIX)

#if defined(RCAN_WINDOWS)

#    include "PCANBasic.h"
#    include "u_can.h"
#    include "windows.h"

#endif  // defined(RCAN_UNIX)

#if defined(STM32F767xx) || defined(STM32F765xx) || defined(STM32F072xB) || defined(STM32F091xC) || \
    defined(STM32F103xB) || defined(STM32F407xx) || defined(STM32F429xx) || defined(STM32F103xE)

#    include "can/bx_can.h"

#endif  // defined(STM32F767xx) || defined(STM32F765xx) || defined(STM32F072xB)
// || defined(STM32F091xC) || defined(STM32F103xB)

#if defined(STM32G474xx) || defined(STM32F0B1xx)

#    include "bx_canfd.h"

#endif  // defined(STM32G474xx) || defined(STM32F0B1xx)

#endif  // __RCAN_DEF_H
