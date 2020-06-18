#ifndef __RCAN_H
#define __RCAN_H


#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"
#include "stm32g4xx_hal.h"
#include "rcan_timing.h"
#include "rcan_filter.h"

typedef enum {
    nonframe,
    std,
    ext
} rcan_frame_type;

typedef struct {
    FDCAN_HandleTypeDef handle;
    rcan_timing timing;
    rcan_filter filter;
    bool use_filter;
} rcan;

#pragma pack(push)
#pragma pack(1)

typedef struct {
    uint32_t id;
    uint8_t len;
    rcan_frame_type type;
    bool rtr;
    uint8_t *payload;
} rcan_frame;

#pragma pack(pop)

bool rcan_filter_preconfiguration(rcan *can, uint32_t *accepted_ids, uint32_t size);

bool rcan_start(rcan *can, uint32_t channel, uint32_t bitrate);

bool rcan_is_ok(rcan *can);

void rcan_stop(rcan *can);

bool rcan_send(rcan *can, rcan_frame *frame);

bool rcan_receive(rcan *can, rcan_frame *frame);

void rcan_view_frame(rcan_frame *frame);


#ifdef __cplusplus
}
#endif

#endif