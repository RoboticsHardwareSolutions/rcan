#pragma once

#include "stdbool.h"
#include "stdint.h"

/**
 * The logic of the hardware acceptance filters can be described as follows:
 *
 *  accepted -> if(received_id & filter_mask ==  filter_id )
 *
 * Where:
 *  - accepted      - if true, the frame will be accepted by the filter.
 *  - received_id   - the CAN ID of the received frame, either 11-bit or 29-bit, with extension bits
 *                    marking extended frames, error frames, etc.
 *  - filter_id     - the value of the filter ID register.
 *  - filter_mask   - the value of the filter mask register.
 *
 * There are special bits that are not members of the CAN ID field:
 *  - EFF - set for extended frames (29-bit), cleared for standard frames (11-bit)
 *  - RTR - like above, indicates Remote Transmission Request frames.
 *
 * The following truth table summarizes the logic (where: FM - filter mask, FID - filter ID, RID - received
 * frame ID, A - true if accepted, X - any state):
 *
 *  FM  FID RID A
 *  0   X   X   1
 *  1   0   0   1
 *  1   1   0   0
 *  1   0   1   0
 *  1   1   1   1
 *
 * One would expect that for the purposes of hardware filtering, the special bits should be treated
 * in the same way as the real ID bits. However, this is not the case with bxCAN. The following truth
 * table has been determined empirically (this behavior was not documented as of 2017):
 *
 *  FM  FID RID A
 *  0   0   0   1
 *  0   0   1   0       <-- frame rejected!
 *  0   1   X   1
 *  1   0   0   1
 *  1   1   0   0
 *  1   0   1   0
 *  1   1   1   1
 */

typedef struct
{
    struct
    {
        uint32_t id;    // bits common to every ID in the accepted set
        uint32_t mask;  // bits that vary across the accepted set ("don't care" bits)
    } mask_filter;      // the only one the current backends (bx_can, bx_canfd) actually program

    struct
    {
        uint32_t from_id;
        uint32_t to_id;
    } from_to_filter;  // computed but not wired into any backend yet

    bool is_extended;  // true if any ID in the accepted set needs 29-bit addressing
} rcan_filter;

/**
 * @brief Compute a hardware acceptance filter (ID+mask) covering every ID in source_sequence.
 * Used internally by rcan_filter_preconfiguration() - not normally called directly.
 * @param source_sequence array of CAN IDs the filter must accept.
 * @param size number of entries in source_sequence (must be > 0).
 * @param filter filled in on success.
 * @return false if any argument is NULL or size is 0.
 */
bool rcan_filter_calculate(uint32_t* source_sequence, uint32_t size, rcan_filter* filter);
