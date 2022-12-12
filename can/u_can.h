#pragma once

#if defined(RCAN_WINDOWS) || defined(RCAN_MACOS) || defined(RCAN_UNIX)

#    include "stdbool.h"
#    include "rcan_def.h"
#    include "rcan.h"
#    include "rcan_timing.h"
#    include "rcan_filter.h"

struct can_iface
{
    uint32_t    channel;
    int         fd;
    bool        opened;  // FIXME real need it ?
    rcan_filter filter;
    bool        use_filter;
};

typedef struct can_iface rcan;

bool u_can_filter_preconfiguration(rcan* can, uint32_t* accepted_ids, uint32_t size);

bool u_can_start(rcan* can, uint32_t channel, uint32_t bitrate);

bool u_can_is_ok(rcan* can);

bool u_can_send(rcan* can, rcan_frame* frame);

bool u_can_receive(rcan* can, rcan_frame* frame);

bool u_can_stop(rcan* can);

#endif  // defined(RCAN_UNIX) || defined(RCAN_MACOS) || defined(RCAN_WINDOWS)
