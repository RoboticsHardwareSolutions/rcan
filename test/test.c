#include "runit.h"
#include "test.h"
#include "rcan.h"

int main(void)
{
    printf("testing rcan \n");
    test_virtual_bus();
    return runit_at_least_one_fail;
}
