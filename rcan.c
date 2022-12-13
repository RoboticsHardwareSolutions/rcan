#include "rcan.h"
#include "stdio.h"
#include "stdint.h"

#include "u_can.h"
#include "bx_can.h"
#include "bx_canfd.h"

bool rcan_filter_preconfiguration(rcan* can, uint32_t* accepted_ids, uint32_t size)
{
    if (can == NULL || accepted_ids == NULL || size == 0)
        return false;

#if defined(RCAN_WINDOWS) || defined(RCAN_MACOS) || defined(RCAN_UNIX)
    return u_can_filter_preconfiguration(can, accepted_ids, size);
#endif

#if defined(STM32F767xx) || defined(STM32F765xx) || defined(STM32F072xB) || defined(STM32F091xC) || defined(STM32F103xB) || defined(STM32F429xx) || defined(STM32F407xx)
    return bx_can_filter_preconfiguration(can, accepted_ids, size);
#endif

#if defined(STM32G474xx)
    return bx_canfd_filter_preconfiguration(can, accepted_ids, size);
#endif
}

bool rcan_start(rcan* can, uint32_t channel, uint32_t bitrate)
{
    if (can == NULL || channel == 0 || bitrate == 0)
        return false;

#if defined(RCAN_WINDOWS) || defined(RCAN_MACOS) || defined(RCAN_UNIX)
    return u_can_start(can, channel, bitrate);
#endif

#if defined(STM32F767xx) || defined(STM32F765xx) || defined(STM32F072xB) || defined(STM32F091xC) || defined(STM32F103xB) || defined(STM32F429xx) || defined(STM32F407xx)
    return bx_can_start(can, channel, bitrate);
#endif

#if defined(STM32G474xx)
    return bx_canfd_start(can, channel, bitrate);
#endif
}

bool rcan_is_ok(rcan* can)
{
    if (can == NULL)
        return false;

#if defined(RCAN_WINDOWS) || defined(RCAN_MACOS) || defined(RCAN_UNIX)
    return u_can_is_ok(can);
#endif

#if defined(STM32F767xx) || defined(STM32F765xx) || defined(STM32F072xB) || defined(STM32F091xC) || defined(STM32F103xB) || defined(STM32F429xx) || defined(STM32F407xx)
    return bx_can_is_ok(can);
#endif

#if defined(STM32G474xx)
    return bx_canfd_is_ok(can);
#endif
}

bool rcan_stop(rcan* can)
{
    if (can == NULL)
        return false;

#if defined(RCAN_WINDOWS) || defined(RCAN_MACOS) || defined(RCAN_UNIX)
    return u_can_stop(can);
#endif

#if defined(STM32F767xx) || defined(STM32F765xx) || defined(STM32F072xB) || defined(STM32F091xC) || defined(STM32F103xB) || defined(STM32F429xx) || defined(STM32F407xx)
    return bx_can_stop(can);
#endif

#if defined(STM32G474xx)
    return bx_canfd_stop(can);
#endif
}

bool rcan_send(rcan* can, rcan_frame* frame)
{
    if (can == NULL || frame == NULL || frame->type == nonframe || frame->len > RCAN_MAX_FRAME_PAYLOAD_SIZE)
        return false;

#if defined(RCAN_WINDOWS) || defined(RCAN_MACOS) || defined(RCAN_UNIX)
    return u_can_send(can, frame);
#endif

#if defined(STM32F767xx) || defined(STM32F765xx) || defined(STM32F072xB) || defined(STM32F091xC) || defined(STM32F103xB) || defined(STM32F429xx) || defined(STM32F407xx)
    return bx_can_send(can, frame);
#endif

#if defined(STM32G474xx)
    return bx_canfd_send(can, frame);
#endif
}

bool rcan_receive(rcan* can, rcan_frame* frame)
{
    if (can == NULL || frame == NULL)
        return false;

#if defined(RCAN_WINDOWS) || defined(RCAN_MACOS) || defined(RCAN_UNIX)
    return u_can_receive(can, frame);
#endif

#if defined(STM32F767xx) || defined(STM32F765xx) || defined(STM32F072xB) || defined(STM32F091xC) || defined(STM32F103xB) || defined(STM32F429xx) || defined(STM32F407xx)
    return bx_can_receive(can, frame);
#endif

#if defined(STM32G474xx)
    return bx_canfd_receive(can, frame);
#endif
}

void rcan_view_frame(rcan_frame* frame)
{
    if (frame == NULL)
        return;

    if (frame->rtr)
    {
        printf("ID : %8x RTR ", frame->id);
        return;
    }

    printf("ID : %8x | %s | LEN : %2d | DATA : ", frame->id, frame->type == std_id ? "STD" : "EXT", frame->len);
    for (uint8_t i = 0; i < frame->len; i++)
    {
        printf("%02x ", frame->payload[i]);
    }
    printf("\n");
}
