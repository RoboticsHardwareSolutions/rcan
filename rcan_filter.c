#include "rcan_filter.h"
#include "string.h"

#define RCAN_FILTER_EXT_ID_MASK                      0x1FFFFFFFU
#define RCAN_FILTER_STD_ID_MASK                      0x000007FFU

bool rcan_filter_calculate_ext(const uint32_t *source_sequence, uint32_t size,
                               rcan_mask_filter_config *const filter_config) {

    if (source_sequence == NULL || filter_config == NULL || size == 0)
        return false;

    filter_config->id = source_sequence[1];
    filter_config->mask = source_sequence[1];

    for (uint32_t i = 0; i < size; i++) {

        if (source_sequence[i] < RCAN_FILTER_STD_ID_MASK)
            continue;

        filter_config->id &= source_sequence[i];
        filter_config->mask |= source_sequence[i];
    }
    filter_config->mask ^= filter_config->id;
    filter_config->mask ^= RCAN_FILTER_EXT_ID_MASK;
}


bool rcan_filter_calculate_std(const uint32_t *source_sequence, uint32_t size,
                               rcan_mask_filter_config *const filter_config) {

    if (source_sequence == NULL || filter_config == NULL || size == 0)
        return false;

    filter_config->id = source_sequence[1];
    filter_config->mask = source_sequence[1];

    for (uint32_t i = 0; i < size; i++) {

        if (source_sequence[i] > RCAN_FILTER_STD_ID_MASK)
            continue;

        filter_config->id &= source_sequence[i];
        filter_config->mask |= source_sequence[i];
    }
    filter_config->mask ^= filter_config->id;
    filter_config->mask ^= RCAN_FILTER_STD_ID_MASK;

}