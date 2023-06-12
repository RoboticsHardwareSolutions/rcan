#include "../runit/src/runit.h"
#include "test.h"
#include "rcan.h"
#include "virtual_bus.h"
#include "virtual_can.h"
#include "hardware_unix.h"
#include "hardware_macos.h"

int main(void)
{
    printf("testing rcan \n");
#if defined(RCAN_VIRTUAL_BUS_TEST)
    test_virtual_bus();
#endif

#if defined(RCAN_UNIX_TEST_UNIX_VIRTUAL_CAN)
    printf("test virtual can must be run from root\n");
    test_virtual_can();
#endif

#if defined(RCAN_HARDWARE_UNIX_TEST)
    test_hardware_unix();
#endif

#if defined(RCAN_HARDWARE_MACOS_TEST)
    test_hardware_macos();
#endif

    runit_report();
    return runit_at_least_one_fail;
}
