#include "hardware_two_pcan.h"
#include "rcan.h"
#include "runit.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "time.h"

#if defined(RCAN_HARDWARE_TWO_PCAN_TEST)

// The only real hardware this test suite assumes: two PCAN-USB (FD) adapters wired together on
// one terminated CAN bus. Which rcan channel macro reaches them depends on the platform - PCBUSB
// (PEAK_CAN_USBBUS1/2) on macOS, or the native peak_usb kernel driver exposing them as plain
// SocketCAN interfaces (SOCKET_CAN0/1) on Linux. See test/hardware_pcan_scenarios.inc for the
// actual scenarios (shared with test/hardware_linux_pcan_driver.c, the proprietary-driver
// variant - avoids duplicating the same ~450 lines a second time).
#    if defined(RCAN_MACOS)
#        define RCAN_TEST_CHANNEL1 PEAK_CAN_USBBUS1
#        define RCAN_TEST_CHANNEL2 PEAK_CAN_USBBUS2
#        define RCAN_TEST_USES_PEAK_API
#    elif defined(RCAN_UNIX)
#        define RCAN_TEST_CHANNEL1 SOCKET_CAN0
#        define RCAN_TEST_CHANNEL2 SOCKET_CAN1
#    endif

#    define RCAN_TEST_ENTRYPOINT test_hardware_two_pcan
#    include "hardware_pcan_scenarios.inc"
#    undef RCAN_TEST_ENTRYPOINT
#    undef RCAN_TEST_CHANNEL1
#    undef RCAN_TEST_CHANNEL2
#    undef RCAN_TEST_USES_PEAK_API

#endif
