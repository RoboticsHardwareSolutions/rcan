#ifndef __RCAN_TIMING_H
#define __RCAN_TIMING_H

#include "stdbool.h"
#include "stdint.h"

typedef struct
{
    uint16_t bit_rate_prescaler;                /// [1, 1024]
    uint32_t bit_segment_1;                     /// [1, 16]
    uint32_t bit_segment_2;                     /// [1, 8]
    uint32_t max_resynchronization_jump_width;  /// [1, 4] (recommended value is 1)
} rcan_timing;

/**
 * @brief Compute bxCAN/bxCANFD bit-timing register values for a target bitrate, aiming for
 * a ~87.5% sample point (CANopen/DeviceNet recommendation). Used internally by the STM32
 * backends - not needed for the host (PCAN/SocketCAN) backend, which takes a bitrate directly.
 * @param peripheral_clock_rate CAN peripheral clock, in Hz.
 * @param target_bitrate desired CAN bitrate, in bit/s (minimum 1000).
 * @param out_timings filled in on success.
 * @return false if the exact bitrate can't be reached with a valid timing, or any argument is invalid.
 */
bool rcan_calculate_timing(uint32_t peripheral_clock_rate, uint32_t target_bitrate, rcan_timing* out_timings);

#endif
