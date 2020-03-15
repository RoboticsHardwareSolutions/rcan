#ifndef __RCAN_FILTER_H
#define __RCAN_FILTER_H

#include "stdbool.h"
#include "stdint.h"


/**
 * This is defined by the CAN hardware.
 * Devices with only one CAN interface have 14 filters
 */
#define RCAN_NUM_ACCEPTANCE_FILTERS                            14U

/// Related to rcan frame

#define CANARD_CAN_EXT_ID_MASK                      0x1FFFFFFFU
#define CANARD_CAN_STD_ID_MASK                      0x000007FFU
#define CANARD_CAN_FRAME_EFF                        (1UL << 31U)         ///< Extended frame format
#define CANARD_CAN_FRAME_RTR                        (1UL << 30U)         ///< Remote transmission (not used by UAVCAN)
#define CANARD_CAN_FRAME_ERR                        (1UL << 29U)

// Rx FIFO mailbox identifier register

#define CANARD_STM32_CAN_RIR_RTR               (1U << 1U)  // Bit 1: Remote Transmission Request
#define CANARD_STM32_CAN_RIR_IDE               (1U << 2U)  // Bit 2: Identifier Extension
#define CANARD_STM32_CAN_RIR_EXID_SHIFT        (3U)        // Bit 3-31: Extended Identifier
#define CANARD_STM32_CAN_RIR_EXID_MASK         (0x1FFFFFFFU << CANARD_STM32_CAN_RIR_EXID_SHIFT)
#define CANARD_STM32_CAN_RIR_STID_SHIFT        (21U)       // Bits 21-31: Standard Identifier
#define CANARD_STM32_CAN_RIR_STID_MASK         (0x07FFU << CANARD_STM32_CAN_RIR_STID_SHIFT)


typedef struct {
    uint32_t id;
    uint32_t mask;
} CanardSTM32AcceptanceFilterConfiguration;


bool rcan_config_bitmask_filter(CanardSTM32AcceptanceFilterConfiguration *filter_configs, uint8_t num_filter_configs);

#endif