#ifndef __RCAN_TIMING_H
#define __RCAN_TIMING_H

#include "stdbool.h"
#include "stdint.h"

typedef struct {
    uint16_t bit_rate_prescaler;                        /// [1, 1024]
    uint32_t bit_segment_1;                              /// [1, 16]
    uint32_t bit_segment_2;                              /// [1, 8]
    uint32_t max_resynchronization_jump_width;           /// [1, 4] (recommended value is 1)
} rcan_timing;


bool rcan_calculate_timing(uint32_t peripheral_clock_rate,
                           uint32_t target_bitrate,
                           rcan_timing *out_timings);


#endif