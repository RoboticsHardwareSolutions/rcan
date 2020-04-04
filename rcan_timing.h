#ifndef __RCAN_TIMING_H
#define __RCAN_TIMING_H

#include "stdbool.h"
#include "stdint.h"
#include "rcan.h"

typedef struct {
    uint16_t bit_rate_prescaler;                        /// [1, 1024]
    uint8_t bit_segment_1;                              /// [1, 16]
    uint8_t bit_segment_2;                              /// [1, 8]
    uint8_t max_resynchronization_jump_width;           /// [1, 4] (recommended value is 1)
} rcan_timing;


bool rcan_calculate_timing(const uint32_t peripheral_clock_rate,
                           const uint32_t target_bitrate,
                           rcan_timing *const out_timings);


#endif