#define CATCH_CONFIG_MAIN

#include <catch/catch.hpp>
#include "rcan.h"

#define EXT_ID_MAX 0x1FFFFFFFU
#define STD_ID_MAX 0x000007FFU

#if defined(RCAN_UNIX)

#    include "pthread.h"
#    include "stdlib.h"
#    include "unistd.h"

#endif

#if defined(RCAN_WINDOWS)
#    include "stdlib.h"
#    include <string>
#    include <iostream>
#endif

#if defined(RCAN_UNIX)
/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

pthread_t candump_thread;

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

TEST_CASE("virtual can")
{
    /**********************************************************************************************************************/
    SECTION("start stop")
    {
        SECTION("start stop  iface vcan 0 - 2")
        {
            rcan vcan0 = {0}, vcan1 = {0}, vcan2 = {0};
            REQUIRE(rcan_start(&vcan0, SOCKET_VCAN0, 1000000) == true);
            REQUIRE(rcan_start(&vcan1, SOCKET_VCAN1, 1000000) == true);
            REQUIRE(rcan_start(&vcan2, SOCKET_VCAN2, 1000000) == true);
            REQUIRE(rcan_stop(&vcan0) == true);
            REQUIRE(rcan_stop(&vcan1) == true);
            REQUIRE(rcan_stop(&vcan2) == true);
        }

        SECTION("start / stop twice")
        {
            rcan vcan0 = {0};
            REQUIRE(rcan_start(&vcan0, SOCKET_VCAN0, 1000000) == true);
            REQUIRE(rcan_start(&vcan0, SOCKET_VCAN0, 1000000) == false);
            REQUIRE(rcan_stop(&vcan0) == true);
            REQUIRE(rcan_stop(&vcan0) == false);
        }

        SECTION("invalid arg")
        {
            rcan vcan0 = {0};
            REQUIRE(rcan_start(NULL, SOCKET_VCAN0, 0) == false);
            REQUIRE(rcan_start(&vcan0, 0, 0) == false);
            REQUIRE(rcan_start(&vcan0, 495868907U, 0) == false);
            REQUIRE(rcan_stop(NULL) == false);
        }
    }

    /**********************************************************************************************************************/

    SECTION("send frames")
    {
        SECTION("send std frame vcan0")
        {
            rcan        vcan0                                = {0};
            uint8_t     payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7};
            rcan_frame  frame;
            std::string line;

            frame.id   = 0x123;
            frame.type = std_id;
            frame.rtr  = false;
            frame.len  = 8;
            memcpy(frame.payload, payload, sizeof(payload));

            REQUIRE(rcan_start(&vcan0, SOCKET_VCAN0, 1000000) == true);

            pthread_create(&candump_thread, NULL, &candump_vcan0_loop, NULL);
            usleep(10000);
            REQUIRE(rcan_send(&vcan0, &frame) == true);
            pthread_join(candump_thread, NULL);

            std::ifstream in("frame.txt");
            if (in.is_open())
            {
                getline(in, line);
                // std::cout << line << std::endl;
                in.close();
            }
            REQUIRE(line.find("vcan0  123   [8]  00 01 02 03 04 05 06 07") == 2);
            REQUIRE(rcan_stop(&vcan0) == true);
        }

        SECTION("send extended frame vcan1")
        {
            rcan        vcan1                                = {0};
            uint8_t     payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {7, 6, 5, 4, 3, 2, 1, 0};
            rcan_frame  frame;
            std::string line;

            frame.id   = 0x12345;
            frame.type = ext_id;
            frame.rtr  = false;
            frame.len  = 8;
            memcpy(frame.payload, payload, sizeof(payload));

            REQUIRE(rcan_start(&vcan1, SOCKET_VCAN1, 1000000) == true);

            pthread_create(&candump_thread, NULL, &candump_vcan1_loop, NULL);
            usleep(10000);

            REQUIRE(rcan_send(&vcan1, &frame) == true);
            pthread_join(candump_thread, NULL);

            std::ifstream in("frame.txt");
            if (in.is_open())
            {
                getline(in, line);
                // std::cout << line << std::endl;
                in.close();
            }
            REQUIRE(line.find("vcan1  00012345   [8]  07 06 05 04 03 02 01 00") == 2);
            REQUIRE(rcan_stop(&vcan1) == true);
        }

        SECTION("send std rtr frame vcan2")
        {
            rcan        vcan2 = {0};
            rcan_frame  frame;
            std::string line;

            frame.id   = 0x123;
            frame.type = std_id;
            frame.rtr  = true;
            frame.len  = 8;

            REQUIRE(rcan_start(&vcan2, SOCKET_VCAN2, 1000000) == true);

            pthread_create(&candump_thread, NULL, &candump_vcan2_loop, NULL);
            usleep(10000);

            REQUIRE(rcan_send(&vcan2, &frame) == true);
            pthread_join(candump_thread, NULL);

            std::ifstream in("frame.txt");
            if (in.is_open())
            {
                getline(in, line);
                in.close();
            }
            REQUIRE(line.find("vcan2  123   [8]  remote request") == 2);
            REQUIRE(rcan_stop(&vcan2) == true);
        }

        SECTION("send ext rtr frame vcan0")
        {
            rcan        vcan0 = {0};
            rcan_frame  frame;
            std::string line;

            frame.id   = 0x123;
            frame.type = ext_id;
            frame.rtr  = true;
            frame.len  = 0;

            REQUIRE(rcan_start(&vcan0, SOCKET_VCAN0, 1000000) == true);

            pthread_create(&candump_thread, NULL, &candump_vcan0_loop, NULL);
            usleep(10000);
            REQUIRE(rcan_send(&vcan0, &frame) == true);
            pthread_join(candump_thread, NULL);

            std::ifstream in("frame.txt");
            if (in.is_open())
            {
                getline(in, line);
                // std::cout << line << std::endl;
                in.close();
            }
            REQUIRE(line.find("vcan0  00000123   [0]  remote request") == 2);
            REQUIRE(rcan_stop(&vcan0) == true);
        }

        SECTION("send invalid frame_sended and send in invalid can")
        {
            rcan        vcan0                                = {0};
            uint8_t     payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7};
            rcan_frame  frame;
            std::string line;

            frame.id   = 0x123;
            frame.type = std_id;
            frame.rtr  = false;
            frame.len  = 8;
            memcpy(frame.payload, payload, sizeof(payload));

            REQUIRE(rcan_send(NULL, &frame) == false);
            REQUIRE(rcan_send(&vcan0, &frame) == false);
            REQUIRE(rcan_start(&vcan0, SOCKET_VCAN0, 1000000) == true);
            REQUIRE(rcan_send(&vcan0, &frame) == true);

            frame.len = CAN_MAX_DLC + 1;
            REQUIRE(rcan_send(&vcan0, &frame) == false);
            frame.len = 8;

            frame.type = nonframe;
            REQUIRE(rcan_send(&vcan0, &frame) == false);
            frame.type = std_id;

            frame.id = STD_ID_MAX + 1;
            REQUIRE(rcan_send(&vcan0, &frame) == false);
            frame.id = 0x123;

            frame.type = ext_id;
            frame.id   = EXT_ID_MAX + 1;
            REQUIRE(rcan_send(&vcan0, &frame) == false);
            frame.id = 123;

            REQUIRE(rcan_stop(&vcan0) == true);
            REQUIRE(rcan_send(&vcan0, &frame) == false);
        }
    }

    /**********************************************************************************************************************/

    SECTION("receive frames")
    {
        SECTION("receive std frame_sended vcan0")
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

            REQUIRE(rcan_start(&vcan0, SOCKET_VCAN0, 1000000) == true);
            system("cansend vcan0 5A1#11.2233.44556677.88");
            REQUIRE(rcan_receive(&vcan0, &frame_sended) == true);
            REQUIRE(memcmp(&received_frame, &frame_sended, sizeof(rcan_frame)) == 0);
            REQUIRE(rcan_stop(&vcan0) == true);
        }

        SECTION("receive extended frame_sended vcan1")
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
            REQUIRE(rcan_start(&vcan1, SOCKET_VCAN1, 1000000) == true);
            system("cansend vcan1 80012345#33.33.33");
            REQUIRE(rcan_receive(&vcan1, &frame_sended) == true);
            REQUIRE(memcmp(&received_frame, &frame_sended, sizeof(rcan_frame)) == 0);
            REQUIRE(rcan_stop(&vcan1) == true);
        }

        SECTION("receive std rtr frame_sended vcan1")
        {
            rcan       vcan1          = {0};
            rcan_frame frame_sended   = {0};
            rcan_frame received_frame = {0};
            received_frame.id         = 0x100;
            received_frame.type       = std_id;
            received_frame.rtr        = true;
            REQUIRE(rcan_start(&vcan1, SOCKET_VCAN1, 1000000) == true);
            system("cansend vcan1 100#R");
            REQUIRE(rcan_receive(&vcan1, &frame_sended) == true);
            REQUIRE(memcmp(&received_frame, &frame_sended, sizeof(rcan_frame) - RCAN_MAX_FRAME_PAYLOAD_SIZE) == 0);
            REQUIRE(rcan_stop(&vcan1) == true);
        }

        SECTION("receive extended rtr frame_sended vcan0")
        {
            rcan       vcan0          = {0};
            rcan_frame frame_sended   = {0};
            rcan_frame received_frame = {0};
            received_frame.id         = 0x100;
            received_frame.type       = ext_id;
            received_frame.rtr        = true;
            REQUIRE(rcan_start(&vcan0, SOCKET_VCAN0, 1000000) == true);
            system("cansend vcan0 80000100#R");
            REQUIRE(rcan_receive(&vcan0, &frame_sended) == true);
            REQUIRE(memcmp(&received_frame, &frame_sended, sizeof(rcan_frame) - RCAN_MAX_FRAME_PAYLOAD_SIZE) == 0);
            REQUIRE(rcan_stop(&vcan0) == true);
        }
    }
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

#endif  // endif RCAN_UNIX

#if defined(RCAN_UNIX_HARDWARE_EQUIPMENT_TEST)

TEST_CASE("pcan basic pci")
{
    /** For this test need hardware !
     * PC or SBC or Laptop with installed PCAN PCI wtih two CAN module where both CAN-out connected ih one bus
     * with terminator.
     * IPEH-002065
     * IPEH-002067
     * IPEH-003027
     * IPEH-003021
     * IPEH-003027
     * IPEH-003045
     * IPEH-003049
     * https://www.peak-system.com/PC-Interfaces.196.0.html?&L=1
     */
    /**********************************************************************************************************************/
    SECTION("start stop")
    {
        SECTION("start stop  iface pcan pci 1,2")
        {
            rcan pcan1 = {0}, pcan2 = {0};
            REQUIRE(rcan_start(&pcan1, PCAN_PCIBUS1, PCAN_BAUD_1M) == true);
            REQUIRE(rcan_start(&pcan2, PCAN_PCIBUS2, PCAN_BAUD_800K) == true);
            REQUIRE(rcan_stop(&pcan1) == true);
            REQUIRE(rcan_stop(&pcan2) == true);
        }

        SECTION("start / stop twice")
        {
            rcan pcan1 = {0};
            REQUIRE(rcan_start(&pcan1, PCAN_PCIBUS1, PCAN_BAUD_100K) == true);
            REQUIRE(rcan_start(&pcan1, PCAN_PCIBUS1, PCAN_BAUD_100K) == false);
            REQUIRE(rcan_stop(&pcan1) == true);
            REQUIRE(rcan_stop(&pcan1) == false);
        }

        SECTION("invalid arg")
        {
            rcan pcan1 = {0};
            REQUIRE(rcan_start(NULL, PCAN_PCIBUS1, 0) == false);
            REQUIRE(rcan_start(&pcan1, PCAN_PCIBUS1, 4359843) == false);
            REQUIRE(rcan_start(&pcan1, 0, 0) == false);
            REQUIRE(rcan_start(&pcan1, 495868907U, PCAN_BAUD_100K) == false);
            REQUIRE(rcan_stop(NULL) == false);
        }
    }
    /**********************************************************************************************************************/

    SECTION("send frames from pcan_pci1 to pcan_pci1")
    {
        SECTION("send std frame")
        {
            rcan       pcan1 = {0}, pcan2 = {0};
            uint8_t    payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7};
            rcan_frame frame_sended = {0}, frame_received = {0};
            frame_sended.id   = 0x123;
            frame_sended.type = std_id;
            frame_sended.rtr  = false;
            frame_sended.len  = 8;
            memcpy(frame_sended.payload, payload, sizeof(payload));

            REQUIRE(rcan_start(&pcan1, PCAN_PCIBUS1, PCAN_BAUD_10K) == true);
            REQUIRE(rcan_start(&pcan2, PCAN_PCIBUS2, PCAN_BAUD_10K) == true);
            REQUIRE(rcan_send(&pcan1, &frame_sended) == true);
            while (!rcan_receive(&pcan2, &frame_received))
            {}
            REQUIRE(rcan_stop(&pcan1) == true);
            REQUIRE(rcan_stop(&pcan2) == true);
            REQUIRE(memcmp(&frame_received, &frame_sended, sizeof(rcan_frame)) == 0);
        }

        SECTION("send extended frame ")
        {
            rcan       pcan1 = {0}, pcan2 = {0};
            uint8_t    payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {7, 6, 5, 4, 3, 2, 1, 0};
            rcan_frame frame_sended = {0}, frame_received = {0};
            frame_sended.id   = 0x12345;
            frame_sended.type = ext_id;
            frame_sended.rtr  = false;
            frame_sended.len  = 8;
            memcpy(frame_sended.payload, payload, sizeof(payload));

            REQUIRE(rcan_start(&pcan1, PCAN_PCIBUS1, PCAN_BAUD_125K) == true);
            REQUIRE(rcan_start(&pcan2, PCAN_PCIBUS2, PCAN_BAUD_125K) == true);
            REQUIRE(rcan_send(&pcan1, &frame_sended) == true);
            while (!rcan_receive(&pcan2, &frame_received))
            {}
            REQUIRE(rcan_stop(&pcan1) == true);
            REQUIRE(rcan_stop(&pcan2) == true);
            REQUIRE(memcmp(&frame_received, &frame_sended, sizeof(rcan_frame)) == 0);
        }

        SECTION("send std rtr frame")
        {
            rcan       pcan1 = {0}, pcan2 = {0};
            rcan_frame frame_sended = {0}, frame_received = {0};

            frame_sended.id   = 0x123;
            frame_sended.type = std_id;
            frame_sended.rtr  = true;

            REQUIRE(rcan_start(&pcan1, PCAN_PCIBUS1, PCAN_BAUD_1M) == true);
            REQUIRE(rcan_start(&pcan2, PCAN_PCIBUS2, PCAN_BAUD_1M) == true);
            REQUIRE(rcan_send(&pcan1, &frame_sended) == true);
            while (!rcan_receive(&pcan2, &frame_received))
            {}
            REQUIRE(rcan_stop(&pcan1) == true);
            REQUIRE(rcan_stop(&pcan2) == true);
            REQUIRE(memcmp(&frame_received, &frame_sended, sizeof(rcan_frame)) == 0);
        }

        SECTION("send ext rtr")
        {
            rcan       pcan1 = {0}, pcan2 = {0};
            rcan_frame frame_sended = {0}, frame_received = {0};

            frame_sended.id   = 0x12345;
            frame_sended.type = ext_id;
            frame_sended.rtr  = true;
            frame_sended.len  = 0;

            REQUIRE(rcan_start(&pcan1, PCAN_PCIBUS1, PCAN_BAUD_500K) == true);
            REQUIRE(rcan_start(&pcan2, PCAN_PCIBUS2, PCAN_BAUD_500K) == true);
            REQUIRE(rcan_send(&pcan1, &frame_sended) == true);
            while (!rcan_receive(&pcan2, &frame_received))
            {}
            REQUIRE(rcan_stop(&pcan1) == true);
            REQUIRE(rcan_stop(&pcan2) == true);

            REQUIRE(memcmp(&frame_received, &frame_sended, sizeof(rcan_frame)) == 0);
        }

        SECTION("send invalid frame_sended and send in invalid can")
        {
            rcan       pcan1                                = {0};
            uint8_t    payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7};
            rcan_frame frame_sended                         = {0};

            frame_sended.id   = 0x123;
            frame_sended.type = std_id;
            frame_sended.rtr  = false;
            frame_sended.len  = 8;
            memcpy(frame_sended.payload, payload, sizeof(payload));

            REQUIRE(rcan_send(NULL, &frame_sended) == false);
            REQUIRE(rcan_send(&pcan1, &frame_sended) == false);
            REQUIRE(rcan_start(&pcan1, PCAN_PCIBUS1, PCAN_BAUD_10K) == true);
            REQUIRE(rcan_send(&pcan1, &frame_sended) == true);

            frame_sended.len = CAN_MAX_DLC + 1;
            REQUIRE(rcan_send(&pcan1, &frame_sended) == false);
            frame_sended.len = 8;

            frame_sended.type = nonframe;
            REQUIRE(rcan_send(&pcan1, &frame_sended) == false);
            frame_sended.type = std_id;

            frame_sended.id = STD_ID_MAX + 1;
            REQUIRE(rcan_send(&pcan1, &frame_sended) == false);
            frame_sended.id = 0x123;

            frame_sended.type = ext_id;
            frame_sended.id   = EXT_ID_MAX + 1;
            REQUIRE(rcan_send(&pcan1, &frame_sended) == false);
            frame_sended.id = 123;

            REQUIRE(rcan_stop(&pcan1) == true);
            REQUIRE(rcan_send(&pcan1, &frame_sended) == false);
        }
    }

    /**********************************************************************************************************************/
}

#endif  // define(RCAN_UNIX_HARDWARE_EQUIPMENT_TEST)
