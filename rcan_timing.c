#include "rcan_timing.h"
#include "string.h"


bool rcan_calculate_timing(const uint32_t peripheral_clock_rate,
                           const uint32_t target_bitrate,
                           rcan_timing *const out_timings) {

    if (target_bitrate == 0 || peripheral_clock_rate == 0 || out_timings == NULL || target_bitrate < 1000)
        return false;

    /**
     * Hardware configuration
     */
    static const uint8_t max_bs1 = 16;
    static const uint8_t max_bs2 = 8;

    /**
     * Ref. "Automatic Baudrate Detection in CANopen Networks", U. Koppe, MicroControl GmbH & Co. KG
     *      CAN in Automation, 2003
     *
     * According to the source, optimal quanta per bit are:
     *   Bitrate        Optimal Maximum
     *   1000 kbps      8       10
     *   500  kbps      16      17
     *   250  kbps      16      17
     *   125  kbps      16      17
     */

    const uint8_t max_quanta_per_bit = (uint8_t)((target_bitrate >= 1000000) ? 10 : 17);

    if (!(max_quanta_per_bit <= (max_bs1 + max_bs2)))
        return false;

    static const uint16_t max_sample_point_location_permill = 900;

    /**
     * Computing (prescaler * BS):
     *   BITRATE = 1 / (PRESCALER * (1 / PCLK) * (1 + BS1 + BS2))       -- See the Reference Manual
     *   BITRATE = PCLK / (PRESCALER * (1 + BS1 + BS2))                 -- Simplified
     * let:
     *   BS = 1 + BS1 + BS2                                             -- Number of time quanta per bit
     *   PRESCALER_BS = PRESCALER * BS
     * ==>
     *   PRESCALER_BS = PCLK / BITRATE
     */

    const uint32_t prescaler_bs = peripheral_clock_rate / target_bitrate;

    /**
     * Searching for such prescaler value so that the number of quanta per bit is highest.
     */

    uint8_t bs1_bs2_sum = (uint8_t)(max_quanta_per_bit - 1);    // NOLINT

    while ((prescaler_bs % (1U + bs1_bs2_sum)) != 0) {

        if (bs1_bs2_sum <= 2)
            return false;

        bs1_bs2_sum--;
    }

    const uint32_t prescaler = prescaler_bs / (1U + bs1_bs2_sum);

    if ((prescaler < 1U) || (prescaler > 1024U))
        return false;


    /**
     * Now we have a constraint: (BS1 + BS2) == bs1_bs2_sum.
     * We need to find such values so that the sample point is as close as possible to the optimal value,
     * which is 87.5%, which is 7/8.
     *
     *   Solve[(1 + bs1)/(1 + bs1 + bs2) == 7/8, bs2]  (* Where 7/8 is 0.875, the recommended sample point location *)
     *   {{bs2 -> (1 + bs1)/7}}
     *
     * Hence:
     *   bs2 = (1 + bs1) / 7
     *   bs1 = (7 * bs1_bs2_sum - 1) / 8
     *
     * Sample point location can be computed as follows:
     *   Sample point location = (1 + bs1) / (1 + bs1 + bs2)
     *
     * Since the optimal solution is so close to the maximum, we prepare two solutions, and then pick the best one:
     *   - With rounding to nearest
     *   - With rounding to zero
     */

    uint8_t bs1 = (uint8_t)(((7 * bs1_bs2_sum - 1) + 4) / 8);       // Trying rounding to nearest first
    uint8_t bs2 = (uint8_t)(bs1_bs2_sum - bs1);


    if (!(bs1_bs2_sum > bs1))
        return false;

    const uint16_t sample_point_permill = (uint16_t)(1000U * (1U + bs1) / (1U + bs1 + bs2));


    if (sample_point_permill > max_sample_point_location_permill) {
        bs1 = (uint8_t)((7 * bs1_bs2_sum - 1) / 8);   // Nope, too far; now rounding to zero
        bs2 = (uint8_t)(bs1_bs2_sum - bs1);
    }

    const bool valid = (bs1 >= 1) && (bs1 <= max_bs1) && (bs2 >= 1) && (bs2 <= max_bs2);

    if ((target_bitrate != (peripheral_clock_rate / (prescaler * (1U + bs1 + bs2)))) || !valid)
        return false; //This actually means that the algorithm has a logic error

    out_timings->bit_rate_prescaler = (uint16_t) prescaler;
    out_timings->max_resynchronization_jump_width = 1;      // One is recommended by CANOpen, and DeviceNet
    out_timings->bit_segment_1 = bs1;
    out_timings->bit_segment_2 = bs2;

    return true;
}


