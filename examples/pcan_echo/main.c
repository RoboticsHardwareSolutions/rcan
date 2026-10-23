/*
 * Opens a real PCAN-USB adapter, sends one frame, then prints every frame it
 * receives until the link goes down or the process is interrupted.
 *
 * Needs an actual PCAN-USB device plugged in (channel 1) - see examples/virtual_bus
 * for a variant that needs no hardware at all.
 */
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include "stdbool.h"
#include "rcan.h"

static volatile bool quit = false;

static void sigterm(int signo)
{
    (void) signo;
    quit = true;
}

int main(void)
{
    if ((signal(SIGTERM, sigterm) == SIG_ERR) || (signal(SIGINT, sigterm) == SIG_ERR))
    {
        perror("signal");
        return errno;
    }

    rcan can = {0};
    if (!rcan_start(&can, PEAK_CAN_USBBUS1, PCAN_BAUD_1M))
    {
        fprintf(stderr, "failed to start PCAN-USB channel 1\n");
        return 1;
    }

    rcan_frame frame = {.id = 123, .type = std_id, .len = 5, .payload = {0x01, 0x02, 0x03, 0x04, 0x05}};
    rcan_send(&can, &frame);

    while (!quit)
    {
        if (rcan_receive(&can, &frame))
            rcan_view_frame(&frame);

        if (!rcan_is_ok(&can))
            quit = true;
    }

    rcan_stop(&can);
    return 0;
}
