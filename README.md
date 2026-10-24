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
| `hardware_two_pcan` | macOS, Linux | two PCAN-USB (FD) adapters wired together on one terminated bus | `hardware` |
| `hardware_linux_pcan_driver` | Linux | same two adapters, reached through PEAK's proprietary chardev driver instead of mainline `peak_usb` | `hardware` |

`hardware_two_pcan` and `hardware_linux_pcan_driver` are the only variants assumed to need real
hardware, and the only hardware this project assumes is **two PCAN-USB (FD) adapters** — nothing
else (no PCI card). Both run the exact same 16 scenarios, defined once in
`test/hardware_pcan_scenarios.inc` and `#include`'d from each variant's thin front-end file,
which just picks the rcan channel macros and the exported entrypoint name: PCBUSB
(`PEAK_CAN_USBBUS1/2`) on macOS, the native `peak_usb` kernel driver exposing the adapters as
plain SocketCAN interfaces (`SOCKET_CAN0/1`) on Linux by default (`hardware_two_pcan.c`), or
PEAK's own proprietary chardev driver (`PEAK_CAN_USBBUS1/2` via `libpcanbasic`, Linux-only,
`hardware_linux_pcan_driver.c`) - see "Switching Linux CAN drivers" below, since the last one
needs a separate driver install that conflicts with the default SocketCAN path.

Safe default (no hardware/root needed, works in plain CI):
```sh
ctest --test-dir build -R rcan -LE "hardware|root"
```

STM32 (bxCAN/bxCANFD) has no automated test — flash the target and verify with real hardware.

### macOS
`virtual_bus` just works. `hardware_two_pcan` needs the two PCAN-USB adapters plugged in:
```sh
ctest --test-dir build -R rcan_hardware_two_pcan
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

`hardware_two_pcan` needs the same two PCAN-USB adapters as macOS, wired together on one
terminated bus - on Linux they show up as `can0`/`can1` via the kernel's native `peak_usb`
driver (`SOCKET_CAN0`/`SOCKET_CAN1`), no PCI hardware or libpcanbasic involved:
```sh
ctest --test-dir build -R rcan_hardware_two_pcan
```

#### Switching Linux CAN drivers

By default, PCAN-USB (FD) adapters are handled by the mainline `peak_usb` SocketCAN driver,
already part of the Linux kernel since 4.0/6.0 (see PEAK's own docs at
[linux.peak-system.com](https://linux.peak-system.com)) - "out-of-the-box usage, suitable for
most use cases". That's what `hardware_two_pcan` uses on Linux.

`hardware_linux_pcan_driver` instead exercises `rcan`'s `PEAK_CAN_USBBUS1/2` path through
PEAK's own proprietary driver package (needed for the chardev/PCAN-Basic API - not something
SocketCAN exposes). **Installing it blacklists the mainline `peak_usb` driver**, so
`hardware_two_pcan` stops working on this machine while it's active - only one of the two
variants can run against real hardware at a time; switch back and forth as needed.

Install (official package, version 9.3.1 at the time of writing - resolve the current one
via PEAK's own redirect rather than hardcoding a version):
```sh
# Kernel headers are mandatory to build the pcan.ko module (driver/Makefile probes
# /usr/src/linux-headers-$(uname -r) and /lib/modules/$(uname -r)/build, and errors out with
# "Unable to automatically find the Kernel headers" if neither exists) - libpopt-dev/g++ are
# only needed to build the package's own test programs under test/.
sudo apt-get install -y "linux-headers-$(uname -r)" libpopt-dev g++

curl -sL -o peak-linux-driver.tar.gz "https://www.peak-system.com/quick/PCAN-Linux-Driver"
tar -xzf peak-linux-driver.tar.gz
cd peak-linux-driver-*
make clean all                        # default "chardev" build: driver + lib + test + libpcanbasic
sudo make install
```
`sudo make install` does the blacklisting itself (driver/Makefile's `install` target `rmmod`s every
module listed in `driver/udev/blacklist-peak.conf` - which includes `peak_usb` - then copies that
same file to `/etc/modprobe.d/blacklist-peak.conf`) - no separate manual blacklist/rmmod step
needed.
```sh
ctest --test-dir build -R rcan_hardware_linux_pcan_driver
```

Uninstall (return to the default mainline/SocketCAN path):
```sh
cd peak-linux-driver-*
sudo make uninstall
```
<!-- TODO: confirm empirically whether can0/can1 reappear automatically after `make uninstall`,
     or whether `sudo modprobe peak_usb` (or a USB replug) is additionally required, and update
     this note accordingly once verified on real hardware. -->

