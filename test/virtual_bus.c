#include "../runit/src/runit.h"
#include "rcan.h"

#if defined(RCAN_VIRTUAL_BUS_TEST)

void test_start_stop_bus(void)
{
    rcan bus0_can1 = {0}, bus0_can2 = {0}, bus0_can3 = {0};
    rcan bus1_can1 = {0}, bus1_can2 = {0}, bus1_can3 = {0};
    rcan bus2_can1 = {0}, bus2_can2 = {0}, bus2_can3 = {0};

    runit_true(rcan_start(&bus0_can1, VIRTUAL_IPC_CAN_BUS0, 0));
    runit_true(rcan_start(&bus0_can2, VIRTUAL_IPC_CAN_BUS0, 0));
    runit_true(rcan_start(&bus0_can3, VIRTUAL_IPC_CAN_BUS0, 0));

    runit_true(rcan_start(&bus1_can1, VIRTUAL_IPC_CAN_BUS1, 0));
    runit_true(rcan_start(&bus1_can2, VIRTUAL_IPC_CAN_BUS1, 0));
    runit_true(rcan_start(&bus1_can3, VIRTUAL_IPC_CAN_BUS1, 0));

    runit_true(rcan_start(&bus2_can1, VIRTUAL_IPC_CAN_BUS2, 0));
    runit_true(rcan_start(&bus2_can2, VIRTUAL_IPC_CAN_BUS2, 0));
    runit_true(rcan_start(&bus2_can3, VIRTUAL_IPC_CAN_BUS2, 0));

    runit_true(rcan_stop(&bus0_can1));
    runit_true(rcan_stop(&bus0_can2));
    runit_true(rcan_stop(&bus0_can3));

    runit_true(rcan_stop(&bus1_can1));
    runit_true(rcan_stop(&bus1_can2));
    runit_true(rcan_stop(&bus1_can3));

    runit_true(rcan_stop(&bus2_can1));
    runit_true(rcan_stop(&bus2_can2));
    runit_true(rcan_stop(&bus2_can3));
}

void test_start_stop_twice(void)
{
    rcan bus0_can1 = {0};
    runit_true(rcan_start(&bus0_can1, VIRTUAL_IPC_CAN_BUS0, 0));
    runit_false(rcan_start(&bus0_can1, VIRTUAL_IPC_CAN_BUS0, 0));
    runit_true(rcan_stop(&bus0_can1));
    runit_false(rcan_stop(&bus0_can1));
}

void test_invalid_arguments(void)
{
    rcan can = {0};
    runit_false(rcan_start(NULL, PEAK_CAN_USBBUS1, 0));
    runit_false(rcan_start(&can, PEAK_CAN_USBBUS1, 4359843));
    runit_false(rcan_start(&can, 0, 0));
    runit_false(rcan_start(&can, 495868907U, PCAN_BAUD_100K));
    runit_false(rcan_stop(NULL));
}

void test_send_std_frame(void)
{
    rcan       pcan1 = {0}, pcan2 = {0};
    uint8_t    payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7};
    rcan_frame frame_sended = {0}, frame_received = {0};
    frame_sended.id   = 0x123;
    frame_sended.type = std_id;
    frame_sended.rtr  = false;
    frame_sended.len  = 8;
    memcpy(frame_sended.payload, payload, sizeof(payload));

    runit_true(rcan_start(&pcan1, VIRTUAL_IPC_CAN_BUS0, 1000000));
    runit_true(rcan_start(&pcan2, VIRTUAL_IPC_CAN_BUS0, 1000000));
    runit_true(rcan_send(&pcan1, &frame_sended));
    runit_true(rcan_receive(&pcan2, &frame_received));
    runit_true(rcan_stop(&pcan1));
    runit_true(rcan_stop(&pcan2));
    runit_true(memcmp(&frame_received, &frame_sended, sizeof(rcan_frame)) == 0);
}

void test_send_extended_frame(void)
{
    rcan       pcan1 = {0}, pcan2 = {0};
    uint8_t    payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {7, 6, 5, 4, 3, 2, 1, 0};
    rcan_frame frame_sended = {0}, frame_received = {0};
    frame_sended.id   = 0x12345;
    frame_sended.type = ext_id;
    frame_sended.rtr  = false;
    frame_sended.len  = 8;
    memcpy(frame_sended.payload, payload, sizeof(payload));

    runit_true(rcan_start(&pcan1, VIRTUAL_IPC_CAN_BUS0, 1000000));
    runit_true(rcan_start(&pcan2, VIRTUAL_IPC_CAN_BUS0, 1000000));
    runit_true(rcan_send(&pcan1, &frame_sended));
    runit_true(rcan_receive(&pcan2, &frame_received));

    runit_true(rcan_stop(&pcan1));
    runit_true(rcan_stop(&pcan2));
    runit_true(memcmp(&frame_received, &frame_sended, sizeof(rcan_frame)) == 0);
}

void test_send_rtr_frame(void)
{
    rcan       pcan1 = {0}, pcan2 = {0};
    rcan_frame frame_sended = {0}, frame_received = {0};

    frame_sended.id   = 0x123;
    frame_sended.type = std_id;
    frame_sended.rtr  = true;

    runit_true(rcan_start(&pcan1, VIRTUAL_IPC_CAN_BUS0, 1000000));
    runit_true(rcan_start(&pcan2, VIRTUAL_IPC_CAN_BUS0, 1000000));
    runit_true(rcan_send(&pcan1, &frame_sended));
    runit_true(rcan_receive(&pcan2, &frame_received));

    runit_true(rcan_stop(&pcan1));
    runit_true(rcan_stop(&pcan2));
    runit_true(memcmp(&frame_received, &frame_sended, sizeof(rcan_frame)) == 0);
}

void test_send_extended_rtr_frame(void)
{
    rcan       pcan1 = {0}, pcan2 = {0};
    rcan_frame frame_sended = {0}, frame_received = {0};

    frame_sended.id   = 0x12345;
    frame_sended.type = ext_id;
    frame_sended.rtr  = true;
    frame_sended.len  = 0;

    runit_true(rcan_start(&pcan1, VIRTUAL_IPC_CAN_BUS0, 1000000));
    runit_true(rcan_start(&pcan2, VIRTUAL_IPC_CAN_BUS0, 1000000));
    runit_true(rcan_send(&pcan1, &frame_sended));
    runit_true(rcan_receive(&pcan2, &frame_received));
    runit_true(rcan_stop(&pcan1));
    runit_true(rcan_stop(&pcan2));
    runit_true(memcmp(&frame_received, &frame_sended, sizeof(rcan_frame)) == 0);
}

void test_send_invalid_frame_in_invalid_can(void)
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
    runit_false(rcan_start(&pcan1, 0xFFFF, 10000000));
    runit_false(rcan_send(&pcan1, &frame_sended));
    frame_sended.len = 9;
    runit_false(rcan_send(&pcan1, &frame_sended));
    frame_sended.len  = 8;
    frame_sended.type = nonframe;
    runit_false(rcan_send(&pcan1, &frame_sended));
    frame_sended.type = std_id;
    frame_sended.id   = 9;
    runit_false(rcan_send(&pcan1, &frame_sended));
    frame_sended.id   = 0x123;
    frame_sended.type = ext_id;
    frame_sended.id   = RCAN_EXT_ID_MAX + 1;
    runit_false(rcan_send(&pcan1, &frame_sended));
    frame_sended.id = 0x123;
    runit_false(rcan_stop(&pcan1));
    runit_false(rcan_send(&pcan1, &frame_sended));
}

void test_virtual_bus(void)
{
    printf("start test of virtual bus\n");
    test_start_stop_bus();
    test_start_stop_twice();
    test_invalid_arguments();
    test_send_std_frame();
    test_send_extended_frame();
    test_send_rtr_frame();
    test_send_extended_rtr_frame();
    test_send_invalid_frame_in_invalid_can();
}

#endif  // defined(RCAN_VIRTUAL_BUS_TEST)