#if defined(RCAN_UNIX_TEST_UNIX_VIRTUAL_CAN)

#    include "pthread.h"
#    include "stdlib.h"
#    include "unistd.h"
#    include "rcan.h"
#    include "../runit/src/runit.h"

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

pthread_t candump_thread;
char      frame1[] = "  vcan0  123   [8]  00 01 02 03 04 05 06 07";
char      frame2[] = "  vcan1  00012345   [8]  07 06 05 04 03 02 01 00";
char      frame3[] = "  vcan2  123   [8]  remote request";
char      frame4[] = "  vcan0  00000123   [0]  remote request";

void* candump_vcan0_loop(void* arg)
{
    system("candump vcan0 -n 1 > frame.txt");
    return NULL;
}

void* candump_vcan1_loop(void* arg)
{
    system("candump vcan1 -n 1 > frame.txt");
    return NULL;
}

void* candump_vcan2_loop(void* arg)
{
    system("candump vcan2 -n 1 > frame.txt");
    return NULL;
}

void test_virtual_can_start_stop(void)
{
    rcan vcan0 = {0}, vcan1 = {0}, vcan2 = {0};
    runit_true(rcan_start(&vcan0, SOCKET_VCAN0, 1000000));
    runit_true(rcan_start(&vcan1, SOCKET_VCAN1, 1000000));
    runit_true(rcan_start(&vcan2, SOCKET_VCAN2, 1000000));
    runit_true(rcan_stop(&vcan0));
    runit_true(rcan_stop(&vcan1));
    runit_true(rcan_stop(&vcan2));
}

void test_virtual_can_start_stop_twice(void)
{
    rcan vcan0 = {0};
    runit_true(rcan_start(&vcan0, SOCKET_VCAN0, 1000000));
    runit_false(rcan_start(&vcan0, SOCKET_VCAN0, 1000000));
    runit_true(rcan_stop(&vcan0));
    runit_false(rcan_stop(&vcan0));
}

void test_virtual_can_invalid_arg(void)
{
    rcan vcan0 = {0};
    runit_false(rcan_start(NULL, SOCKET_VCAN0, 0));
    runit_false(rcan_start(&vcan0, 0, 0));
    runit_false(rcan_start(&vcan0, 495868907U, 0));
    runit_false(rcan_stop(NULL));
}

void test_virtual_can_send_frame(void)
{
    rcan       vcan0                                = {0};
    uint8_t    payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7};
    rcan_frame frame;

    frame.id   = 0x123;
    frame.type = std_id;
    frame.rtr  = false;
    frame.len  = 8;
    memcpy(frame.payload, payload, sizeof(payload));

    runit_true(rcan_start(&vcan0, SOCKET_VCAN0, 1000000));

    pthread_create(&candump_thread, NULL, &candump_vcan0_loop, NULL);
    usleep(10000);
    runit_true(rcan_send(&vcan0, &frame));
    pthread_join(candump_thread, NULL);

    FILE*   fp;
    char*   line = NULL;
    size_t  len  = 0;
    ssize_t read;
    fp = fopen("frame.txt", "r");
    runit_true(fp != NULL);

    read = getline(&line, &len, fp);
    runit_true(read != -1);
    fclose(fp);
    runit_true(line != NULL);

    runit_true(strncmp(line, frame1, sizeof(frame1) - 1) == 0);
    runit_true(rcan_stop(&vcan0));
    free(line);
}

void test_virtual_can_send_ext_frame(void)
{
    rcan       vcan1                                = {0};
    uint8_t    payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {7, 6, 5, 4, 3, 2, 1, 0};
    rcan_frame frame;

    frame.id   = 0x12345;
    frame.type = ext_id;
    frame.rtr  = false;
    frame.len  = 8;
    memcpy(frame.payload, payload, sizeof(payload));

    runit_true(rcan_start(&vcan1, SOCKET_VCAN1, 1000000));

    pthread_create(&candump_thread, NULL, &candump_vcan1_loop, NULL);
    usleep(10000);

    runit_true(rcan_send(&vcan1, &frame));
    pthread_join(candump_thread, NULL);

    FILE*   fp;
    char*   line = NULL;
    size_t  len  = 0;
    ssize_t read;
    fp = fopen("frame.txt", "r");
    runit_true(fp != NULL);

    read = getline(&line, &len, fp);
    runit_true(read != -1);
    fclose(fp);
    runit_true(line != NULL);

    runit_true(strncmp(line, frame2, sizeof(frame2) - 1) == 0);
    runit_true(rcan_stop(&vcan1));
    free(line);
}

void test_virtual_can_send_rtr_frame(void)
{
    rcan       vcan2 = {0};
    rcan_frame frame;

    frame.id   = 0x123;
    frame.type = std_id;
    frame.rtr  = true;
    frame.len  = 8;

    runit_true(rcan_start(&vcan2, SOCKET_VCAN2, 1000000));

    pthread_create(&candump_thread, NULL, &candump_vcan2_loop, NULL);
    usleep(10000);

    runit_true(rcan_send(&vcan2, &frame));
    pthread_join(candump_thread, NULL);

    FILE*   fp;
    char*   line = NULL;
    size_t  len  = 0;
    ssize_t read;
    fp = fopen("frame.txt", "r");
    runit_true(fp != NULL);

    read = getline(&line, &len, fp);
    runit_true(read != -1);
    fclose(fp);
    runit_true(line != NULL);

    runit_true(strncmp(line, frame3, sizeof(frame3) - 1) == 0);
    runit_true(rcan_stop(&vcan2));
    free(line);
}

void test_virtual_can_send_ext_rtr_frame(void)
{
    rcan       vcan0 = {0};
    rcan_frame frame;

    frame.id   = 0x123;
    frame.type = ext_id;
    frame.rtr  = true;
    frame.len  = 0;

    runit_true(rcan_start(&vcan0, SOCKET_VCAN0, 1000000));

    pthread_create(&candump_thread, NULL, &candump_vcan0_loop, NULL);
    usleep(10000);
    runit_true(rcan_send(&vcan0, &frame));
    pthread_join(candump_thread, NULL);

    FILE*   fp;
    char*   line = NULL;
    size_t  len  = 0;
    ssize_t read;
    fp = fopen("frame.txt", "r");
    runit_true(fp != NULL);

    read = getline(&line, &len, fp);
    runit_true(read != -1);
    fclose(fp);
    runit_true(line != NULL);

    runit_true(strncmp(line, frame4, sizeof(frame4) - 1) == 0);
    runit_true(rcan_stop(&vcan0));
    free(line);
}

void test_virtual_can_invalid_frame(void)
{
    rcan       vcan0                                = {0};
    uint8_t    payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7};
    rcan_frame frame;

    frame.id   = 0x123;
    frame.type = std_id;
    frame.rtr  = false;
    frame.len  = 8;
    memcpy(frame.payload, payload, sizeof(payload));

    runit_false(rcan_send(NULL, &frame));
    runit_false(rcan_send(&vcan0, &frame));
    runit_true(rcan_start(&vcan0, SOCKET_VCAN0, 1000000));
    runit_true(rcan_send(&vcan0, &frame));

    frame.len = 8 + 1;
    runit_false(rcan_send(&vcan0, &frame));
    frame.len = 8;

    frame.type = nonframe;
    runit_false(rcan_send(&vcan0, &frame));
    frame.type = std_id;

    frame.id = RCAN_STD_ID_MAX + 1;
    runit_false(rcan_send(&vcan0, &frame));
    frame.id = 0x123;

    frame.type = ext_id;
    frame.id   = RCAN_EXT_ID_MAX + 1;
    runit_false(rcan_send(&vcan0, &frame));
    frame.id = 123;

    runit_true(rcan_stop(&vcan0));
    runit_false(rcan_send(&vcan0, &frame));
}

void test_virtual_can_receive(void)
{
    rcan       vcan0                                = {0};
    uint8_t    payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    rcan_frame frame_sended                         = {0};
    rcan_frame received_frame                       = {0};
    received_frame.id                               = 0x5a1;
    received_frame.type                             = std_id;
    received_frame.rtr                              = false;
    received_frame.len                              = 8;
    memcpy(received_frame.payload, payload, sizeof(payload));

    runit_true(rcan_start(&vcan0, SOCKET_VCAN0, 1000000));
    system("cansend vcan0 5A1#11.2233.44556677.88");
    runit_true(rcan_receive(&vcan0, &frame_sended));
    runit_true(memcmp(&received_frame, &frame_sended, sizeof(rcan_frame)) == 0);
    runit_true(rcan_stop(&vcan0));
}

void test_virtual_can_receive_ext_frame(void)
{
    rcan       vcan1          = {0};
    uint8_t    payload[3]     = {0x33, 0x33, 0x33};
    rcan_frame frame_sended   = {0};
    rcan_frame received_frame = {0};
    received_frame.id         = 0x00012345;
    received_frame.type       = ext_id;
    received_frame.rtr        = false;
    received_frame.len        = 3;
    memcpy(received_frame.payload, payload, sizeof(payload));
    runit_true(rcan_start(&vcan1, SOCKET_VCAN1, 1000000));
    system("cansend vcan1 80012345#33.33.33");
    runit_true(rcan_receive(&vcan1, &frame_sended));
    runit_true(memcmp(&received_frame, &frame_sended, sizeof(rcan_frame)) == 0);
    runit_true(rcan_stop(&vcan1));
}

void test_virtual_can_receive_rtr_frame(void)
{
    rcan       vcan1          = {0};
    rcan_frame frame_sended   = {0};
    rcan_frame received_frame = {0};
    received_frame.id         = 0x100;
    received_frame.type       = std_id;
    received_frame.rtr        = true;
    runit_true(rcan_start(&vcan1, SOCKET_VCAN1, 1000000));
    system("cansend vcan1 100#R");
    runit_true(rcan_receive(&vcan1, &frame_sended));
    runit_true(memcmp(&received_frame, &frame_sended, sizeof(rcan_frame) - RCAN_MAX_FRAME_PAYLOAD_SIZE) == 0);
    runit_true(rcan_stop(&vcan1));
}

void test_virtual_can_receive_ext_rtr_frame(void)
{
    rcan       vcan0          = {0};
    rcan_frame frame_sended   = {0};
    rcan_frame received_frame = {0};
    received_frame.id         = 0x100;
    received_frame.type       = ext_id;
    received_frame.rtr        = true;
    runit_true(rcan_start(&vcan0, SOCKET_VCAN0, 1000000));
    system("cansend vcan0 80000100#R");
    runit_true(rcan_receive(&vcan0, &frame_sended));
    runit_true(memcmp(&received_frame, &frame_sended, sizeof(rcan_frame) - RCAN_MAX_FRAME_PAYLOAD_SIZE) == 0);
    runit_true(rcan_stop(&vcan0));
}

void test_virtual_can(void)
{
    printf("test virtual can\n");
    test_virtual_can_start_stop();
    test_virtual_can_start_stop_twice();
    test_virtual_can_invalid_arg();
    test_virtual_can_send_frame();
    test_virtual_can_send_ext_frame();
    test_virtual_can_send_rtr_frame();
    test_virtual_can_send_ext_rtr_frame();
    test_virtual_can_invalid_frame();
    test_virtual_can_receive();
    test_virtual_can_receive_ext_frame();
    test_virtual_can_receive_ext_rtr_frame();
}

#endif  // endif RCAN_UNIX_TEST_VIRTUAL_CAN
