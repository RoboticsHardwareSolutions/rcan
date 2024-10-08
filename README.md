## rcan

simple C CAN lib for : `unix` | `macos` | `windows` | `stm32`

supported hardware : `stm32 can`  `PCAN-USB` `PCAN-PCI` `SOCET CAN UNIX`


## example :
 
```
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include "stdbool.h"
#include "rcan.h"


volatile bool quit = false;

rcan can;

rcan_frame frame = {.id = 123, .type = std_id, .len = 5,
        .payload = {0x01, 0x02, 0x03, 0x04, 0x05}};


void sigterm(int signo) {
    quit = true;
}

int main(int argc, char *argv[]) {

    if ((signal(SIGTERM, sigterm) == SIG_ERR) ||
        (signal(SIGINT, sigterm) == SIG_ERR)) {
        perror("Error");
        return errno;
    }

    rcan_start(&can, PCAN_USBBUS1, PCAN_BAUD_1M);
    rcan_send(&can, &frame);

    while (!quit) {

        if (rcan_receive(&can, &frame))
            rcan_view_frame(&frame);

        if (!rcan_is_ok(&can))
            quit = true;
    }
    return 0;
}
```

