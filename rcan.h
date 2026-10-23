#ifndef __RCAN_H__
#define __RCAN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"
#include "stdint.h"
#include "rcan_def.h"

typedef struct can_iface rcan;

/**
 * @brief Configure a hardware acceptance filter so only the given CAN IDs are received.
 * Must be called before rcan_start(). Not all backends support hardware filtering yet
 * (see u_can_filter_preconfiguration()'s TODO for macOS); unsupported backends still
 * accept the call but receive every frame regardless.
 * @param accepted_ids array of CAN IDs to accept.
 * @param size number of entries in accepted_ids (must be > 0).
 * @return true if the filter was computed and applied.
 */
bool rcan_filter_preconfiguration(rcan* can, uint32_t* accepted_ids, uint32_t size);

/**
 * @brief Open and configure a CAN channel, ready for rcan_send()/rcan_receive().
 * @param can   - can instance
 * @param channel  - list of channlles defined in file rcan_def and here all values :
 *
 *   PEAK_CAN_USBBUS1 -  PCAN-USB interface, channel 1 (linux, macos)
 *   PEAK_CAN_USBBUS2 -  PCAN-USB interface, channel 2 (linux, macos)
 *   PEAK_CAN_USBBUS3 -  PCAN-USB interface, channel 3 (linux, macos)
 *
 *   PEAK_CAN_PCIBUS1 -  PCAN-PCI interface, channel 1 (linux only)
 *   PEAK_CAN_PCIBUS2 -  PCAN-PCI interface, channel 2 (linux only)
 *   PEAK_CAN_PCIBUS3 -  PCAN-PCI interface, channel 3 (linux only)
 *
 *   SOCKET_CAN0 - macro for can socet can iface (linux only)
 *   SOCKET_CAN1 - macro for can socet can iface (linux only)
 *   SOCKET_CAN2 - macro for can socet can iface (linux only)
 *
 *   SOCKET_VCAN0 - macro for vcan socet can iface (linux only, in docker NOT WORK(docker not support))
 *   SOCKET_VCAN1 - macro for vcan socet can iface (linux only, in docker NOT WORK(docker not support))
 *   SOCKET_VCAN2 - macro for vcan socet can iface (linux only, in docker NOT WORK(docker not support))
 *
 *   VIRTUAL_INPROC_CAN_BUS0 - bus for inter thread communication (linux, macos, docker) - bus for communication
 *   between one process with several thread
 *   VIRTUAL_INPROC_CAN_BUS1 - bus for inter thread communication (linux, macos, docker) - bus for communication
 *   between one process with several thread
 *   VIRTUAL_INPROC_CAN_BUS2 - bus for inter thread communication (linux, macos, docker) - bus for communication
 *   between one process with several thread
 *
 *   VIRTUAL_IPC_CAN_BUS0 - bus for inter process communication (linux, macos, docker) - bus for communication
 *   between several process or sereveral different app - exaple CAN open master aba CAN open slave
 *   VIRTUAL_IPC_CAN_BUS1 - bus for inter process communication (linux, macos, docker) - bus for communication
 *   between several process or sereveral different app - exaple CAN open master aba CAN open slave
 *   VIRTUAL_IPC_CAN_BUS2 - bus for inter process communication (linux, macos, docker) - bus for communication
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

/**
 * @brief Check whether an already-started channel is still usable (link not down, device
 * still present, etc.). What exactly is checked depends on the backend.
 * @return true if the channel is OK to keep using.
 */
bool rcan_is_ok(rcan* can);

/**
 * @brief Close a channel opened by rcan_start(). Safe to call once; calling it again on an
 * already-stopped channel returns false.
 */
bool rcan_stop(rcan* can);

/**
 * @brief Send one CAN frame on an already-started channel.
 * @return false if can/frame are NULL, the channel isn't ready, frame->type is nonframe,
 * frame->len exceeds RCAN_MAX_FRAME_PAYLOAD_SIZE, or the backend failed to send it.
 */
bool rcan_send(rcan* can, rcan_frame* frame);

/**
 * @brief Poll for one received CAN frame (non-blocking). Call this in a loop.
 * @param frame filled in on success; left untouched if no frame was available.
 * @return true if a frame was received into *frame.
 */
bool rcan_receive(rcan* can, rcan_frame* frame);

/**
 * @brief Print a frame to stdout for debugging (ID, STD/EXT, length, payload bytes).
 */
void rcan_view_frame(rcan_frame* frame);

#if defined(RCAN_BACKEND_BXCAN)
/**
 * @brief Enable the RX-FIFO-pending interrupt (bxCAN only) so the HAL interrupt handler
 * starts delivering received frames; call once you're ready to start receiving.
 */
bool rcan_async_receive_start(rcan* can);
#endif

#ifdef __cplusplus
}
#endif

#endif
