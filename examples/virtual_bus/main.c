/*
 * Two CAN nodes talking to each other over rcan's virtual in-process bus -
 * no hardware and no root privileges needed, works the same on macOS and
 * Linux. Handy for trying the API or prototyping application logic before
 * real hardware is available.
 */
#include <stdio.h>
#include "rcan.h"

int main(void)
{
    rcan sender = {0}, receiver = {0};

    if (!rcan_start(&sender, VIRTUAL_INPROC_CAN_BUS0, 0) || !rcan_start(&receiver, VIRTUAL_INPROC_CAN_BUS0, 0))
    {
        fprintf(stderr, "failed to start virtual bus nodes\n");
        return 1;
    }

    rcan_frame frame = {.id = 0x100, .type = std_id, .len = 5, .payload = {1, 2, 3, 4, 5}};
    if (!rcan_send(&sender, &frame))
    {
        fprintf(stderr, "failed to send frame\n");
        return 1;
    }

    rcan_frame received = {0};
    while (!rcan_receive(&receiver, &received))
    {
        // the virtual bus delivers asynchronously; poll until the frame arrives
    }

    printf("received: ");
    rcan_view_frame(&received);

    rcan_stop(&sender);
    rcan_stop(&receiver);
    return 0;
}
