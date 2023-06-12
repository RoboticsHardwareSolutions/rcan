#include "../runit/src/runit.h"
#include "rcan.h"

#if defined(RCAN_HARDWARE_UNIX_TEST)

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