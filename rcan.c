#include "rcan.h"
#include "stdio.h"
#include "stdint.h"
#include "inttypes.h"

#if defined(RCAN_BACKEND_HOST)
#    define RCAN_BACKEND(fn) u_can_##fn
#elif defined(RCAN_BACKEND_BXCAN)
#    define RCAN_BACKEND(fn) bx_can_##fn
#elif defined(RCAN_BACKEND_BXCANFD)
#    define RCAN_BACKEND(fn) bx_canfd_##fn
#endif

bool rcan_filter_preconfiguration(rcan* can, uint32_t* accepted_ids, uint32_t size)
{
    if (can == NULL || accepted_ids == NULL || size == 0)
    {
        return false;
    }

    return RCAN_BACKEND(filter_preconfiguration)(can, accepted_ids, size);
}

bool rcan_start(rcan* can, uint32_t channel, uint32_t bitrate)
{
    if (can == NULL || channel == 0)
    {
        return false;
    }

    return RCAN_BACKEND(start)(can, channel, bitrate);
}

bool rcan_is_ok(rcan* can)
{
    if (can == NULL)
    {
        return false;
    }

    return RCAN_BACKEND(is_ok)(can);
}

bool rcan_stop(rcan* can)
{
    if (can == NULL)
    {
        return false;
    }

    return RCAN_BACKEND(stop)(can);
}

bool rcan_send(rcan* can, rcan_frame* frame)
{
    if (can == NULL || frame == NULL || frame->type == nonframe || frame->len > RCAN_MAX_FRAME_PAYLOAD_SIZE)
    {
        return false;
    }

    return RCAN_BACKEND(send)(can, frame);
}

bool rcan_receive(rcan* can, rcan_frame* frame)
{
    if (can == NULL || frame == NULL)
    {
        return false;
    }

    return RCAN_BACKEND(receive)(can, frame);
}

void rcan_view_frame(rcan_frame* frame)
{
    if (frame == NULL)
    {
        return;
    }

    if (frame->rtr)
    {
        printf("ID : %8" PRIx32 " RTR ", frame->id);
        return;
    }

    printf("ID : %8" PRIx32 " | %s | LEN : %2d | DATA : ",
           frame->id,
           frame->type == std_id ? "STD" : "EXT",
           frame->len);
    for (uint8_t i = 0; i < frame->len; i++)
    {
        printf("%02x ", frame->payload[i]);
    }
    printf("\n");
}

#if defined(RCAN_BACKEND_BXCAN)
bool rcan_async_receive_start(rcan* can)
{
    HAL_CAN_ActivateNotification(&can->handle, CAN_IT_RX_FIFO0_MSG_PENDING);
    return 0;
}
#endif
