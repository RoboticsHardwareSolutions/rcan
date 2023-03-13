#pragma once

#if defined(RCAN_WINDOWS) || defined(RCAN_MACOS) || defined(RCAN_UNIX)

#    include <uuid/uuid.h>

#    include "stdbool.h"
#    include "rcan_def.h"
#    include "rcan.h"
#    include "rcan_timing.h"
#    include "rcan_filter.h"


struct can_iface {
    uint32_t channel;  // look defines in rcan_def.h
    int fd; // for socket CAN
    void *ctx;  // for virtual can (zeroMQ)
    void *pub; // for virtual can (zeroMQ)
    void *sub; // for virtual can (zeroMQ)
    uuid_t node_id; // for virtual can (zeroMQ)
    bool can_ready;
    rcan_filter filter;
    bool use_filter;
};

#pragma pack(push)
#pragma pack(1)
typedef struct {

    uuid_t uuid;
    rcan_frame can_frame;

} virtual_can_frame;
#pragma pack(pop)

typedef struct can_iface rcan;

bool u_can_filter_preconfiguration(rcan *can, uint32_t *accepted_ids, uint32_t size);

bool u_can_start(rcan *can, uint32_t channel, uint32_t bitrate);

bool u_can_is_ok(rcan *can);

bool u_can_send(rcan *can, rcan_frame *frame);

bool u_can_receive(rcan *can, rcan_frame *frame);

bool u_can_stop(rcan *can);

#endif  // defined(RCAN_UNIX) || defined(RCAN_MACOS) || defined(RCAN_WINDOWS)
