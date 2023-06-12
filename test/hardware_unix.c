#include "../runit/src/runit.h"
#include "rcan.h"

#if defined(RCAN_HARDWARE_UNIX_TEST)

char text[] =
    "/** For this test need hardware ! \n* PC with installed PCAN PCI wtih two CAN module where both\n "
    "CAN-out connected ih one bus * with terminator \n IPEH-002065 *\n IPEH-002067 *\n IPEH-003027 *\n IPEH-003021 *\n "
    "IPEH-003027 *\n IPEH-003045 *\n IPEH-003049 *\n https://www.peak-system.com/PC-Interfaces.196.0.html?&L=1*/\n";

void test_hardware_unix_start_stop_pcan(void)
{
    rcan pcan1 = {0}, pcan2 = {0};
    runit_true(rcan_start(&pcan1, PCAN_PCIBUS1, 1000000));
    runit_true(rcan_start(&pcan2, PCAN_PCIBUS2, 500000));
    runit_true(rcan_stop(&pcan1));
    runit_true(rcan_stop(&pcan2));
}

void test_hardware_unix_start_stop_pcan_twice(void)
{
    rcan pcan1 = {0};
    runit_true(rcan_start(&pcan1, PCAN_PCIBUS1, PCAN_BAUD_100K));
    runit_false(rcan_start(&pcan1, PCAN_PCIBUS1, PCAN_BAUD_100K));
    runit_true(rcan_stop(&pcan1));
    runit_false(rcan_stop(&pcan1));
}

void test_hardware_unix_invalid_arg(void)
{
    rcan pcan1 = {0};
    runit_false(rcan_start(NULL, PCAN_PCIBUS1, 0));
    runit_false(rcan_start(&pcan1, PCAN_PCIBUS1, 4359843));
    runit_false(rcan_start(&pcan1, 0, 0));
    runit_false(rcan_start(&pcan1, 495868907U, PCAN_BAUD_100K));
    runit_false(rcan_stop(NULL));
}

void test_hardware_unix_send_std_frame(void)
{
    rcan       pcan1 = {0}, pcan2 = {0};
    uint8_t    payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7};
    rcan_frame frame_sended = {0}, frame_received = {0};
    frame_sended.id   = 0x123;
    frame_sended.type = std_id;
    frame_sended.rtr  = false;
    frame_sended.len  = 8;
    memcpy(frame_sended.payload, payload, sizeof(payload));

    runit_true(rcan_start(&pcan1, PCAN_PCIBUS1, PCAN_BAUD_10K));
    runit_true(rcan_start(&pcan2, PCAN_PCIBUS2, PCAN_BAUD_10K));
    runit_true(rcan_send(&pcan1, &frame_sended));
    while (!rcan_receive(&pcan2, &frame_received))
    {
    }
    runit_true(rcan_stop(&pcan1));
    runit_true(rcan_stop(&pcan2));
    runit_true(memcmp(&frame_received, &frame_sended, sizeof(rcan_frame)) == 0);
}

void test_hardware_unix_send_ext_frame(void)
{
    rcan       pcan1 = {0}, pcan2 = {0};
    uint8_t    payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {7, 6, 5, 4, 3, 2, 1, 0};
    rcan_frame frame_sended = {0}, frame_received = {0};
    frame_sended.id   = 0x12345;
    frame_sended.type = ext_id;
    frame_sended.rtr  = false;
    frame_sended.len  = 8;
    memcpy(frame_sended.payload, payload, sizeof(payload));

    runit_true(rcan_start(&pcan1, PCAN_PCIBUS1, 125000));
    runit_true(rcan_start(&pcan2, PCAN_PCIBUS2, 1250000));
    runit_true(rcan_send(&pcan1, &frame_sended));
    while (!rcan_receive(&pcan2, &frame_received))
    {
    }
    runit_true(rcan_stop(&pcan1) == true);
    runit_true(rcan_stop(&pcan2) == true);
    runit_true(memcmp(&frame_received, &frame_sended, sizeof(rcan_frame)) == 0);
}

void test_hardware_unix_send_rtr_frame(void)
{
    rcan       pcan1 = {0}, pcan2 = {0};
    rcan_frame frame_sended = {0}, frame_received = {0};

    frame_sended.id   = 0x123;
    frame_sended.type = std_id;
    frame_sended.rtr  = true;

    runit_true(rcan_start(&pcan1, PCAN_PCIBUS1, 1000000));
    runit_true(rcan_start(&pcan2, PCAN_PCIBUS2, 1000000));
    runit_true(rcan_send(&pcan1, &frame_sended));
    while (!rcan_receive(&pcan2, &frame_received))
    {
    }
    runit_true(rcan_stop(&pcan1));
    runit_true(rcan_stop(&pcan2));
    runit_true(memcmp(&frame_received, &frame_sended, sizeof(rcan_frame)) == 0);
}

void test_hardware_unix_send_ext_rtr_frame(void)
{
    rcan       pcan1 = {0}, pcan2 = {0};
    rcan_frame frame_sended = {0}, frame_received = {0};

    frame_sended.id   = 0x12345;
    frame_sended.type = ext_id;
    frame_sended.rtr  = true;
    frame_sended.len  = 0;

    runit_true(rcan_start(&pcan1, PCAN_PCIBUS1, 500000));
    runit_true(rcan_start(&pcan2, PCAN_PCIBUS2, 500000));
    runit_true(rcan_send(&pcan1, &frame_sended));
    while (!rcan_receive(&pcan2, &frame_received))
    {
    }
    runit_true(rcan_stop(&pcan1));
    runit_true(rcan_stop(&pcan2));
    runit_true(memcmp(&frame_received, &frame_sended, sizeof(rcan_frame)) == 0);
}

void test_hardware_unix_send_invalid(void)
{
    rcan       pcan1                                = {0};
    uint8_t    payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7};
    rcan_frame frame_sended                         = {0};

    frame_sended.id   = 0x123;
    frame_sended.type = std_id;
    frame_sended.rtr  = false;
    frame_sended.len  = 8;
    memcpy(frame_sended.payload, payload, sizeof(payload));

    runit_false(rcan_send(NULL, &frame_sended));
    runit_false(rcan_send(&pcan1, &frame_sended));
    runit_true(rcan_start(&pcan1, PCAN_PCIBUS1, 10000));
    runit_true(rcan_send(&pcan1, &frame_sended));

    frame_sended.len = CAN_MAX_DLC + 1;
    runit_false(rcan_send(&pcan1, &frame_sended));
    frame_sended.len = 8;

    frame_sended.type = nonframe;
    runit_false(rcan_send(&pcan1, &frame_sended));
    frame_sended.type = std_id;

    frame_sended.id = RCAN_STD_ID_MAX + 1;
    runit_false(rcan_send(&pcan1, &frame_sended));
    frame_sended.id = 0x123;

    frame_sended.type = ext_id;
    frame_sended.id   = RCAN_EXT_ID_MAX + 1;
    runit_false(rcan_send(&pcan1, &frame_sended));
    frame_sended.id = 123;

    runit_true(rcan_stop(&pcan1));
    runit_false(rcan_send(&pcan1, &frame_sended));
}
void test_hardware_unix(void)
{
    printf("test hardware unix\n");
    printf("%s", text);
    test_hardware_unix_start_stop_pcan();
    test_hardware_unix_start_stop_pcan_twice();
    test_hardware_unix_invalid_arg();
    test_hardware_unix_send_std_frame();
    test_hardware_unix_send_ext_frame();
    test_hardware_unix_send_rtr_frame();
    test_hardware_unix_send_ext_rtr_frame();
    test_hardware_unix_send_invalid();
}

#endif  // RCAN_HARDWARE_UNIX_TEST