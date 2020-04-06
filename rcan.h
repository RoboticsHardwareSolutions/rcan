#ifndef __RCAN_H
#define __RCAN_H

#include "stdbool.h"
#include "stm32g4xx_hal.h"
#include "rcan_timing.h"
#include "rcan_filter.h"


typedef enum {
    std,
    ext
} rcan_frame_type;

typedef struct {
    FDCAN_HandleTypeDef handle;
    rcan_timing timing;
    rcan_filter filter;
    bool use_filter;
} rcan;


typedef struct {
    uint32_t id;
    uint8_t len;
    rcan_frame_type type;
    uint8_t *payload;
} rcan_frame;


bool rcan_filter_preconfiguration(rcan *can, const uint32_t *source_sequence, uint32_t size);

bool rcan_start(rcan *can, uint32_t channel, uint32_t bitrate);

bool rcan_is_ok(rcan *can);

void rcan_stop(rcan *can);

bool rcan_write(rcan *can, rcan_frame *frame);

bool rcan_read(rcan *can, rcan_frame *frame);

void rcan_view_frame(rcan_frame *frame);


#endif