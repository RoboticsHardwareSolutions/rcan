#ifndef __RCAN_H
#define __RCAN_H


#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"
#include "rcan_def.h"

#define RCAN_MAX_FRAME_PAYLOAD_SIZE                    8

typedef struct can_iface rcan;

typedef enum {
    nonframe,
    std_id,
    ext_id
} rcan_frame_type;


#pragma pack(push)
#pragma pack(1)

typedef struct {
    uint32_t id;
    uint8_t len;
    rcan_frame_type type;
    bool rtr;
    uint8_t payload[RCAN_MAX_FRAME_PAYLOAD_SIZE];
} rcan_frame;

#pragma pack(pop)

bool rcan_filter_preconfiguration(rcan *can, uint32_t *accepted_ids, uint32_t size);

bool rcan_start(rcan *can, uint32_t channel, uint32_t bitrate);

bool rcan_is_ok(rcan *can);

bool rcan_stop(rcan *can);

bool rcan_send(rcan *can, rcan_frame *frame);

bool rcan_receive(rcan *can, rcan_frame *frame);

void rcan_view_frame(rcan_frame *frame);


#ifdef __cplusplus
}
#endif

#endif