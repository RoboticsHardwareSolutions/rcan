#include "runit.h"
#include "test.h"
#include "rcan.h"
#include "virtual_bus.h"
#include "virtual_can.h"
#include "hardware_unix.h"
#include "hardware_macos.h"
#include <string.h>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("usage: %s <virtual_bus|virtual_can|hardware_unix|hardware_macos>\n", argv[0]);
        return 1;
    }

    printf("testing rcan: %s\n", argv[1]);

    if (strcmp(argv[1], "virtual_bus") == 0)
    {
        test_virtual_bus();
    }
#if defined(RCAN_UNIX_TEST_UNIX_VIRTUAL_CAN)
    else if (strcmp(argv[1], "virtual_can") == 0)
    {
        printf("test virtual can must be run from root\n");
        test_virtual_can();
    }
#endif
#if defined(RCAN_HARDWARE_UNIX_TEST)
    else if (strcmp(argv[1], "hardware_unix") == 0)
    {
        test_hardware_unix();
    }
#endif
#if defined(RCAN_HARDWARE_MACOS_TEST)
    else if (strcmp(argv[1], "hardware_macos") == 0)
    {
        test_hardware_macos();
    }
#endif
    else
    {
        printf("unknown or unsupported-on-this-host test target: %s\n", argv[1]);
        return 1;
    }

    runit_report();
    return runit_at_least_one_fail;
}
