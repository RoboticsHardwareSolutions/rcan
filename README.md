## rcan

simple C CAN lib for : `unix` | `macos` | `stm32`

supported hardware : `stm32 can`  `PCAN-USB` `PCAN-PCI` `SOCET CAN UNIX`

Runnable versions of the examples below live in [`examples/`](examples) -
`examples/pcan_echo` (needs a real PCAN-USB device) and `examples/virtual_bus`
(no hardware needed, works anywhere):
```sh
cmake -S . -B build && cmake --build build --target rcan_example_virtual_bus
./build/examples/rcan_example_virtual_bus
```

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

## example for BareMetal with Rx Interrupt:

```c
rcan can;

void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (canHandle->Instance == CAN1)
    {
        /* CAN1 clock enable */
        __HAL_RCC_CAN1_CLK_ENABLE();

        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**CAN1 GPIO Configuration
        PA11     ------> CAN1_RX
        PA12     ------> CAN1_TX
        */
        GPIO_InitStruct.Pin       = GPIO_PIN_11 | GPIO_PIN_12;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        // If you want to use Rx Interrupts
        HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
    }
}

// Reference Interrupt Handler
void CAN1_RX0_IRQHandler(void)
{
    rcan_frame frame = {0};

    if (!rcan_is_ok(&can))
    {
        return;
    }

    rcan_receive(&can, &frame);
    // ... your code
}

/*
 * Use rcan_async_receive_start(&can) in your main code when you will be ready to communication
 */

```

## Testing

Build and list the tests (run from inside this `rcan/` directory):
```sh
cmake -S . -B build && cmake --build build --target rcan_test
ctest --test-dir build -N
```

One binary, variant picked at runtime (`./rcan_test <variant>`), one `ctest` entry per variant:

| Variant (`rcan_<variant>`) | Platform | Needs | Label |
|---|---|---|---|
| `virtual_bus` | any | nothing (pure software bus via `rnode`) | — |
| `virtual_can` | Linux | root privileges for SocketCAN, `can-utils` | `root` |
| `hardware_unix` | Linux | real PCAN-PCI card (two modules on one terminated bus) | `hardware` |
| `hardware_macos` | macOS | real PCAN-USB device | `hardware` |

Safe default (no hardware/root needed, works in plain CI):
```sh
ctest --test-dir build -R rcan -LE "hardware|root"
```

STM32 (bxCAN/bxCANFD) has no automated test — flash the target and verify with real hardware.

### macOS
`virtual_bus` just works. `hardware_macos` needs a PCAN-USB device plugged in:
```sh
ctest --test-dir build -R rcan_hardware_macos
```

### Linux
`virtual_bus` works out of the box. `virtual_can` additionally needs:
- `can-utils` installed (`candump`/`cansend`/`cangw`)
- one-time passwordless sudo for the commands that create/configure network interfaces
  (`ip`, `modprobe`, `cangw`, `setcap`) — `libsocketcan`'s netlink calls and `ip link add`
  need `CAP_NET_ADMIN`, which a plain user doesn't have by default:
  ```sh
  echo "$USER ALL=(root) NOPASSWD: /usr/sbin/ip, /usr/sbin/modprobe, /usr/bin/cangw, /usr/sbin/setcap" \
      | sudo tee /etc/sudoers.d/rcan-test
  sudo chmod 440 /etc/sudoers.d/rcan-test
  ```

After that, `cmake --build` grants `rcan_test` `CAP_NET_ADMIN` itself (see the `setcap`
post-build step in `CMakeLists.txt`), so the test binary runs unprivileged - no `sudo` needed
to actually run it. `ctest -R rcan_virtual_can` bridges `vcan0`<->`vcan1` with the Linux
`CAN_GW` kernel gateway before the test and tears it down after (`test/canfortests.sh`,
registered as a `FIXTURES_SETUP`/`FIXTURES_CLEANUP` pair, same pattern as `rserial`'s socat
fixture):
```sh
ctest --test-dir build -R rcan_virtual_can
```

`hardware_unix` needs a real PCAN-PCI card (see the comment at the top of `test/hardware_unix.c`
for the exact two-module, terminated-bus setup it expects).

