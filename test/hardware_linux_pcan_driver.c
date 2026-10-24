#include "hardware_linux_pcan_driver.h"
#include "rcan.h"
#include "runit.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "time.h"

#if defined(RCAN_HARDWARE_LINUX_PCAN_DRIVER_TEST)

// Same two PCAN-USB (FD) adapters as test/hardware_two_pcan.c, but reached through PEAK's own
// proprietary chardev driver (libpcanbasic talking to /dev/pcanusbN) instead of the mainline
// peak_usb/SocketCAN path - requires that driver installed and loaded (see README.md,
// "Switching Linux CAN drivers"), which blacklists peak_usb while active. Same scenarios as the
// SocketCAN variant, shared via test/hardware_pcan_scenarios.inc.
#    define RCAN_TEST_CHANNEL1 PEAK_CAN_USBBUS1
#    define RCAN_TEST_CHANNEL2 PEAK_CAN_USBBUS2
#    define RCAN_TEST_USES_PEAK_API

#    define RCAN_TEST_ENTRYPOINT test_hardware_linux_pcan_driver
#    include "hardware_pcan_scenarios.inc"
#    undef RCAN_TEST_ENTRYPOINT
#    undef RCAN_TEST_CHANNEL1
#    undef RCAN_TEST_CHANNEL2
#    undef RCAN_TEST_USES_PEAK_API

#endif
