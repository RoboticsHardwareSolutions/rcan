#ifndef __RCAN_H__
#define __RCAN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"
#include "stdint.h"
#include "rcan_def.h"

typedef struct can_iface rcan;

bool rcan_filter_preconfiguration(rcan* can, uint32_t* accepted_ids, uint32_t size);


/**
 * @param can   - can instance
 * @param channel  - list of channlles defined in file rcan_def and here all values :
 *
 *   PEAK_CAN_USBBUS1 -  PCAN-USB interface, channel 1 (linux , win, macos)
 *   PEAK_CAN_USBBUS2 -  PCAN-USB interface, channel 2 (linux , win, macos)
 *   PEAK_CAN_USBBUS3 -  PCAN-USB interface, channel 3 (linux , win, macos)
 *
 *   PEAK_CAN_PCIBUS1 -  PCAN-PCI interface, channel 1 (linux and win only)
 *   PEAK_CAN_PCIBUS2 -  PCAN-PCI interface, channel 2 (linux and win only)
 *   PEAK_CAN_PCIBUS3 -  PCAN-PCI interface, channel 3 (linux and win only)
 *
 *   SOCKET_CAN0 - macro for can socet can iface (linux only)
 *   SOCKET_CAN1 - macro for can socet can iface (linux only)
 *   SOCKET_CAN2 - macro for can socet can iface (linux only)
 *
 *   SOCKET_VCAN0 - macro for vcan socet can iface (linux only, in docker NOT WORK(docker not support))
 *   SOCKET_VCAN1 - macro for vcan socet can iface (linux only, in docker NOT WORK(docker not support))
 *   SOCKET_VCAN2 - macro for vcan socet can iface (linux only, in docker NOT WORK(docker not support))
 *
 *   VIRTUAL_INPROC_CAN_BUS0 - bus for inter thread communication (linux, win, macos, docker) - bus for communication
 *   between one process with several thread
 *   VIRTUAL_INPROC_CAN_BUS1 - bus for inter thread communication (linux, win, macos, docker) - bus for communication
 *   between one process with several thread
 *   VIRTUAL_INPROC_CAN_BUS2 - bus for inter thread communication (linux, win, macos, docker) - bus for communication
 *   between one process with several thread
 *
 *   VIRTUAL_IPC_CAN_BUS0 - bus for inter process communication (linux, win, macos, docker) - bus for communication
 *   between several process or sereveral different app - exaple CAN open master aba CAN open slave
 *   VIRTUAL_IPC_CAN_BUS1 - bus for inter process communication (linux, win, macos, docker) - bus for communication
 *   between several process or sereveral different app - exaple CAN open master aba CAN open slave
 *   VIRTUAL_IPC_CAN_BUS2 - bus for inter process communication (linux, win, macos, docker) - bus for communication
 *   between several process or sereveral different app - exaple CAN open master aba CAN open slave
 *
 *   FDCAN1_BASE - FDCAN1 -for STM32G4
 *   FDCAN2_BASE - FDCAN1 -for STM32G4
 *   FDCAN3_BASE - FDCAN1 -for STM32G4
 *
 *   CAN1_BASE - For CAN BX - typical bx can STM32 - F407 ....
 *   CAN2_BASE - For CAN BX - typical bx can STM32 - F407 ....
 *   CAN3_BASE - For CAN BX - typical bx can STM32 - F407 ....
 *
 * @param bitrate - 0 - 1000000 Bit/Sec
 * @return true if started
 *
 */
bool rcan_start(rcan* can, uint32_t channel, uint32_t bitrate);

bool rcan_is_ok(rcan* can);

bool rcan_stop(rcan* can);

bool rcan_send(rcan* can, rcan_frame* frame);

bool rcan_receive(rcan* can, rcan_frame* frame);

void rcan_view_frame(rcan_frame* frame);

#if !(defined(__unix__) || defined(__APPLE__) || defined(__CYGWIN__))
bool rcan_async_receive_start(rcan* can);
#endif

#ifdef __cplusplus
}
#endif

#endif
