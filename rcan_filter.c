#include "rcan_filter.h"
#include "string.h"

#define RCAN_FILTER_EXT_ID_MASK                      0x1FFFFFFFU
#define RCAN_FILTER_STD_ID_MASK                      0x000007FFU

static bool are_there_extended_id(uint32_t *source_sequence, uint32_t size);

static uint32_t max_id(uint32_t *source_sequence, uint32_t size);

static uint32_t min_id(uint32_t *source_sequence, uint32_t size);


bool rcan_filter_calculate(uint32_t *source_sequence, uint32_t size, rcan_filter *filter) {

    if (source_sequence == NULL || filter == NULL || size == 0)
        return false;

    filter->is_extended = are_there_extended_id(source_sequence, size);
    filter->from_to_filter.from_id = min_id(source_sequence, size);
    filter->from_to_filter.to_id = max_id(source_sequence, size);
    filter->mask_filter.id = source_sequence[0];
    filter->mask_filter.mask = source_sequence[0];

    for (uint32_t i = 0; i < size; i++) {
        filter->mask_filter.id &= source_sequence[i];
        filter->mask_filter.mask |= source_sequence[i];
    }

    filter->mask_filter.mask ^= filter->mask_filter.id;

    if (filter->is_extended)
        filter->mask_filter.mask ^= RCAN_FILTER_EXT_ID_MASK;
    else
        filter->mask_filter.mask ^= RCAN_FILTER_STD_ID_MASK;

    return true;
}

static bool are_there_extended_id(uint32_t *source_sequence, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        if (source_sequence[i] > RCAN_FILTER_STD_ID_MASK)
            return true;
    }
    return false;
}


static uint32_t max_id(uint32_t *source_sequence, uint32_t size) {
    uint32_t max = source_sequence[0];
    for (int i = 0; i < size; i++) {
        if (max > source_sequence[i]) {
            max = source_sequence[i];
        }
    }
    return max;
}

static uint32_t min_id(uint32_t *source_sequence, uint32_t size) {
    uint32_t min = source_sequence[0];
    for (int i = 0; i < size; i++) {
        if (min < source_sequence[i]) {
            min = source_sequence[i];
        }
    }
    return min;
}

