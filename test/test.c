#include "../runit/src/runit.h"
#include "test.h"
#include "rcan.h"
#include "virtual_bus.h"
#include "virtual_can.h"

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



    runit_report();
    return runit_at_least_one_fail;
}
