#include "rcan_filter.h"
#include "string.h"

bool rcan_config_bitmask_filter(CanardSTM32AcceptanceFilterConfiguration *filter_configs,
                                uint8_t num_filter_configs) {
    // Note that num_filter_configs = 0 is a valid configuration, which rejects all frames.
    if ((filter_configs == NULL) || (num_filter_configs > RCAN_NUM_ACCEPTANCE_FILTERS))
        return false;

    /*
     * First we disable all filters. This may cause momentary RX frame losses, but the application
     * should be able to tolerate that.
     */
    //CANARD_STM32_CAN1->FA1R = 0;
    //TODO add disable filters

    /*
     * Having filters disabled we can update the configuration.
     * Register mapping: FR1 - ID, FR2 - Mask
     */
    for (uint8_t i = 0; i < num_filter_configs; i++) {
        /*
         * Converting the ID and the Mask into the representation that can be chewed by the hardware.
         * If Mask asks us to accept both STDID and EXTID, we need to use EXT mode on the filter,
         * otherwise it will reject all EXTID frames. This logic is not documented in the RM.
         *
         * The logic of the hardware acceptance filters can be described as follows:
         *
         *  accepted = (received_id & filter_mask) == (filter_id & filter_mask)
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
        uint32_t id = 0;
        uint32_t mask = 0;

        CanardSTM32AcceptanceFilterConfiguration *cfg = filter_configs + i;

        if ((cfg->id & CANARD_CAN_FRAME_EFF) || !(cfg->mask & CANARD_CAN_FRAME_EFF)) {
            id = (cfg->id & CANARD_CAN_EXT_ID_MASK) << 3U;
            mask = (cfg->mask & CANARD_CAN_EXT_ID_MASK) << 3U;
            id |= CANARD_STM32_CAN_RIR_IDE;
        } else {
            id = (cfg->id & CANARD_CAN_STD_ID_MASK) << 21U;
            mask = (cfg->mask & CANARD_CAN_STD_ID_MASK) << 21U;
        }

        if (cfg->id & CANARD_CAN_FRAME_RTR) {
            id |= CANARD_STM32_CAN_RIR_RTR;
        }

        if (cfg->mask & CANARD_CAN_FRAME_EFF) {
            mask |= CANARD_STM32_CAN_RIR_IDE;
        }

        if (cfg->mask & CANARD_CAN_FRAME_RTR) {
            mask |= CANARD_STM32_CAN_RIR_RTR;
        }

        /*
         * Applying the converted representation to the registers.
         */
        uint8_t filter_index = i;

        //CANARD_STM32_CAN1->FilterRegister[filter_index].FR1 = id;
        //CANARD_STM32_CAN1->FilterRegister[filter_index].FR2 = mask;

        //CANARD_STM32_CAN1->FA1R |= 1U << filter_index;      // Enable
    }

    return 0;
}