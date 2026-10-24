#ifndef __RCAN_DEF_H__
#define __RCAN_DEF_H__

#include "rcan_backend.h"
#include "rcan_filter.h"
#include "rcan_timing.h"

#define RCAN_MAX_FRAME_PAYLOAD_SIZE 8

typedef enum
{
    nonframe,    // not a valid frame; rcan_send() rejects it
    std_id,      // 11-bit standard CAN ID
    ext_id,      // 29-bit extended CAN ID
    error_frame  // bus error/status event, not a CAN message; see rcan_receive() docs in rcan.h
} rcan_frame_type;

// Classification of a bus error/status event delivered as an rcan_frame with type ==
// error_frame (see rcan_receive() docs in rcan.h for the full payload layout).
typedef enum
{
    rcan_err_unspecified,
    rcan_err_bit,         // single-bit error
    rcan_err_form,        // frame format error
    rcan_err_stuff,       // bit-stuffing error
    rcan_err_controller,  // RX/TX overflow, warning/passive level change
    rcan_err_busoff,
    rcan_err_transceiver,
    rcan_err_ack,  // no ACK received on transmission
    rcan_err_arbitration_lost
} rcan_bus_error_type;

// Sentinel for error_frame payload[1]/[2]/[3] (direction/RX counter/TX counter): "not provided by
// this backend" - see rcan_receive() docs in rcan.h.
#define RCAN_ERR_FIELD_UNDEF 0xFFU

#pragma pack(push)
#pragma pack(1)

typedef struct
{
    uint32_t        id;    // CAN identifier (<= RCAN_STD_ID_MAX or RCAN_EXT_ID_MAX, depending on type)
    uint8_t         len;   // payload length in bytes, <= RCAN_MAX_FRAME_PAYLOAD_SIZE
    rcan_frame_type type;  // std_id or ext_id
    bool            rtr;   // true for a Remote Transmission Request (no payload)
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

#if defined(RCAN_BACKEND_HOST)

#    if defined(RCAN_MACOS)

#        include "PCBUSB.h"

#    elif defined(RCAN_UNIX)

#        include "stdlib.h"
#        include <PCANBasic.h>
#        include <fcntl.h>
#        include <libsocketcan.h>
#        include <linux/can.h>
#        include <linux/can/error.h>
#        include <linux/can/raw.h>
#        include <net/if.h>
#        include <sys/ioctl.h>
#        include <sys/socket.h>
#        include <unistd.h>

#    endif

#    include "u_can.h"

#elif defined(RCAN_BACKEND_BXCAN)

#    include "can/bx_can.h"

#elif defined(RCAN_BACKEND_BXCANFD)

#    include "bx_canfd.h"

#endif  // defined(RCAN_BACKEND_HOST)

#endif  // __RCAN_DEF_H
