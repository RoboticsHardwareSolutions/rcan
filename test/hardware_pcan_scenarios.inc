// Shared body for every "two real PCAN-USB (FD) adapters" hardware test variant. Not compiled on
// its own - #include this from a thin front-end .c file that first defines:
//
//   RCAN_TEST_CHANNEL1 / RCAN_TEST_CHANNEL2  - the two rcan channel macros to use
//   RCAN_TEST_ENTRYPOINT                     - the name of the exported aggregator function
//   RCAN_TEST_USES_PEAK_API                  - (optional) define it iff RCAN_TEST_CHANNEL1/2 are
//                                               PEAK_CAN_USBBUS* (PCBUSB on macOS, or the
//                                               proprietary chardev driver on Linux) rather than
//                                               SOCKET_CAN* - see heal_bus_after_bitrate_mismatch()
//
// Every test function below is `static`: several front-ends can #include this file into separate
// translation units of the same rcan_test binary (e.g. on Linux, both the SocketCAN variant and
// the proprietary-driver variant compile into one executable) without symbol collisions - only
// RCAN_TEST_ENTRYPOINT's name needs to be unique per variant, and the front-end controls that.

// Generous for real hardware (frames arrive in microseconds-to-ms), but bounds every receive
// wait in this file so a missing/unwired adapter fails the test instead of hanging ctest forever.
#define RCAN_TEST_RECEIVE_TIMEOUT_SEC 0.5

// Fills *frame with the given fields; payload is only copied for non-RTR frames with len > 0.
static void build_frame(rcan_frame*     frame,
                        uint32_t        id,
                        rcan_frame_type type,
                        bool            rtr,
                        uint8_t         len,
                        const uint8_t*  payload)
{
    memset(frame, 0, sizeof(rcan_frame));
    frame->id   = id;
    frame->type = type;
    frame->rtr  = rtr;
    frame->len  = len;

    if (!rtr && payload != NULL && len > 0)
    {
        memcpy(frame->payload, payload, len);
    }
}

// Low-level primitive: waits up to timeout_sec for ANY rcan_receive() event - a data frame
// (std_id/ext_id) or a bus error/status event (error_frame, see rcan.h). Callers that care which
// kind they got use receive_data_frame()/receive_error_frame() below instead of this directly.
static bool poll_receive(rcan* can, rcan_frame* frame, double timeout_sec)
{
    clock_t start = clock();

    while (((double) (clock() - start)) / CLOCKS_PER_SEC < timeout_sec)
    {
        if (rcan_receive(can, frame))
        {
            return true;
        }
    }
    return false;
}

// Waits up to timeout_sec for a data frame (std_id/ext_id). rcan_start() enables error-frame
// reporting by default (see rcan.h), so a real bus fault can legitimately surface an
// error_frame event here instead of the data this caller is waiting for - log it (so it's not
// silently lost) and keep waiting out the same timeout budget rather than treating it as "the"
// result.
static bool receive_data_frame(rcan* can, rcan_frame* frame, double timeout_sec)
{
    clock_t start = clock();

    while (((double) (clock() - start)) / CLOCKS_PER_SEC < timeout_sec)
    {
        if (rcan_receive(can, frame))
        {
            if (frame->type == error_frame)
            {
                rcan_view_frame(frame);
                continue;
            }
            return true;
        }
    }
    return false;
}

// Symmetric counterpart to receive_data_frame(): waits up to timeout_sec specifically for an
// error_frame event, logging and skipping any real data frames encountered along the way. Used
// by error-frame-focused tests so a stray valid data frame can never be mistaken for the error
// event under test.
static bool receive_error_frame(rcan* can, rcan_frame* frame, double timeout_sec)
{
    clock_t start = clock();

    while (((double) (clock() - start)) / CLOCKS_PER_SEC < timeout_sec)
    {
        if (rcan_receive(can, frame))
        {
            if (frame->type != error_frame)
            {
                rcan_view_frame(frame);
                continue;
            }
            return true;
        }
    }
    return false;
}

// Sends frame_sent on tx, waits up to RCAN_TEST_RECEIVE_TIMEOUT_SEC for rx to receive it as data,
// then checks the whole struct matches.
static void send_and_verify(rcan* tx, rcan* rx, rcan_frame* frame_sent)
{
    rcan_frame frame_received = {0};

    runit_true(rcan_send(tx, frame_sent));
    runit_true(receive_data_frame(rx, &frame_received, RCAN_TEST_RECEIVE_TIMEOUT_SEC));
    runit_true(memcmp(&frame_received, frame_sent, sizeof(rcan_frame)) == 0);
}

// A mismatched bitrate between the two nodes on a real CAN bus drives the transceivers' hardware
// error counters (TEC/REC) up - confirmed via CAN_GetStatus()/PCAN_ERROR_BUSHEAVY on macOS and via
// CAN_ERR_CRTL_* error frames on Linux/SocketCAN - and rcan_start() does not reset them. Left
// alone, this corrupts every later run of this test variant (on any process) until the adapters
// are physically replugged. A burst of successful matched-bitrate round trips reliably drains the
// counters back down (empirically ~40 were enough; this uses 60 for margin). Best-effort: not
// asserted, since this is cleanup, not part of the test under verification.
static void heal_bus_after_bitrate_mismatch(void)
{
    rcan pcan1 = {0}, pcan2 = {0};

#if defined(RCAN_TEST_USES_PEAK_API)
    // If an earlier test in this same process failed a runit_assert mid-test, it returned before
    // reaching its own rcan_stop(), leaving the PEAK channels claimed at the driver level for the
    // rest of this process - a plain rcan_start() below would then fail immediately ("already
    // initialized") and this whole healing pass would silently no-op. Force-release both channels
    // directly (bypassing rcan's can_ready tracking, which only knows about *this* fresh struct)
    // before trying to reopen them. This applies to every PEAK/libpcanbasic-API variant (PCBUSB on
    // macOS, and the proprietary chardev driver on Linux - same CAN_Initialize/
    // PCAN_ERROR_INITIALIZE contract either way). SocketCAN has no equivalent problem:
    // rcan_start() always does its own stop/reconfigure/start over netlink regardless of any
    // leaked file descriptor.
    CAN_Uninitialize(PCAN_NONEBUS);
#endif

    if (!rcan_start(&pcan1, RCAN_TEST_CHANNEL1, 1000000) || !rcan_start(&pcan2, RCAN_TEST_CHANNEL2, 1000000))
    {
        rcan_stop(&pcan1);
        rcan_stop(&pcan2);
        return;
    }

    for (int i = 0; i < 60; i++)
    {
        rcan_frame frame = {0}, received = {0};
        uint8_t    payload[1] = {(uint8_t) i};
        build_frame(&frame, 0x100, std_id, false, 1, payload);
        rcan_send(&pcan1, &frame);
        receive_data_frame(&pcan2, &received, RCAN_TEST_RECEIVE_TIMEOUT_SEC);
    }

    rcan_stop(&pcan1);
    rcan_stop(&pcan2);
}

static void test_start_stop_pcan(void)
{
    rcan pcan1 = {0}, pcan2 = {0};
    runit_true(rcan_start(&pcan1, RCAN_TEST_CHANNEL1, 1000000));
    runit_true(rcan_start(&pcan2, RCAN_TEST_CHANNEL2, 500000));
    runit_true(rcan_stop(&pcan1));
    runit_true(rcan_stop(&pcan2));
}

static void test_start_stop_pcan_twice(void)
{
    rcan pcan1 = {0};
    runit_true(rcan_start(&pcan1, RCAN_TEST_CHANNEL1, 100000));
    runit_false(rcan_start(&pcan1, RCAN_TEST_CHANNEL2, 100000));
    runit_true(rcan_stop(&pcan1));
    runit_false(rcan_stop(&pcan1));
}

static void test_invalid_arg(void)
{
    rcan pcan1 = {0};
    runit_false(rcan_start(NULL, RCAN_TEST_CHANNEL1, 0));
    // 0 bps, not a non-preset-but-plausible rate: PEAK rejects anything off its fixed whitelist
    // (convert_to_peak_can_bitrate()), but SocketCAN computes bit-timing directly from the
    // requested rate via netlink and can silently accept/clamp an odd-but-nonzero value (verified
    // empirically: 4359843 was accepted on Linux/SocketCAN even though it's not a standard rate) -
    // 0 is invalid on both, since neither backend can derive bit-timing for a 0 Hz target.
    runit_false(rcan_start(&pcan1, RCAN_TEST_CHANNEL2, 0));
    runit_false(rcan_start(&pcan1, 0, 0));
    runit_false(rcan_start(&pcan1, 495868907U, PCAN_BAUD_100K));
    runit_false(rcan_stop(NULL));
}

static void test_send_std_frame(void)
{
    rcan       pcan1 = {0}, pcan2 = {0};
    uint8_t    payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7};
    rcan_frame frame_sended = {0}, frame_received = {0};
    frame_sended.id   = 0x123;
    frame_sended.type = std_id;
    frame_sended.rtr  = false;
    frame_sended.len  = 8;
    memcpy(frame_sended.payload, payload, sizeof(payload));

    runit_true(rcan_start(&pcan1, RCAN_TEST_CHANNEL1, 10000));
    runit_true(rcan_start(&pcan2, RCAN_TEST_CHANNEL2, 10000));
    runit_true(rcan_send(&pcan1, &frame_sended));
    runit_true(receive_data_frame(&pcan2, &frame_received, RCAN_TEST_RECEIVE_TIMEOUT_SEC));
    runit_true(rcan_stop(&pcan1));
    runit_true(rcan_stop(&pcan2));
    runit_true(memcmp(&frame_received, &frame_sended, sizeof(rcan_frame)) == 0);
}

static void test_send_ext_frame(void)
{
    rcan       pcan1 = {0}, pcan2 = {0};
    uint8_t    payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {7, 6, 5, 4, 3, 2, 1, 0};
    rcan_frame frame_sended = {0}, frame_received = {0};
    frame_sended.id   = 0x12345;
    frame_sended.type = ext_id;
    frame_sended.rtr  = false;
    frame_sended.len  = 8;
    memcpy(frame_sended.payload, payload, sizeof(payload));

    runit_true(rcan_start(&pcan1, RCAN_TEST_CHANNEL1, 125000));
    runit_true(rcan_start(&pcan2, RCAN_TEST_CHANNEL2, 125000));
    runit_true(rcan_send(&pcan1, &frame_sended));
    runit_true(receive_data_frame(&pcan2, &frame_received, RCAN_TEST_RECEIVE_TIMEOUT_SEC));
    runit_true(rcan_stop(&pcan1) == true);
    runit_true(rcan_stop(&pcan2) == true);
    runit_true(memcmp(&frame_received, &frame_sended, sizeof(rcan_frame)) == 0);
}

static void test_send_rtr_frame(void)
{
    rcan       pcan1 = {0}, pcan2 = {0};
    rcan_frame frame_sended = {0}, frame_received = {0};

    frame_sended.id   = 0x123;
    frame_sended.type = std_id;
    frame_sended.rtr  = true;

    runit_true(rcan_start(&pcan1, RCAN_TEST_CHANNEL1, 1000000));
    runit_true(rcan_start(&pcan2, RCAN_TEST_CHANNEL2, 1000000));
    runit_true(rcan_send(&pcan1, &frame_sended));
    runit_true(receive_data_frame(&pcan2, &frame_received, RCAN_TEST_RECEIVE_TIMEOUT_SEC));
    runit_true(rcan_stop(&pcan1));
    runit_true(rcan_stop(&pcan2));
    runit_true(memcmp(&frame_received, &frame_sended, sizeof(rcan_frame)) == 0);
}

static void test_send_ext_rtr_frame(void)
{
    rcan       pcan1 = {0}, pcan2 = {0};
    rcan_frame frame_sended = {0}, frame_received = {0};

    frame_sended.id   = 0x12345;
    frame_sended.type = ext_id;
    frame_sended.rtr  = true;
    frame_sended.len  = 0;

    runit_true(rcan_start(&pcan1, RCAN_TEST_CHANNEL1, 500000));
    runit_true(rcan_start(&pcan2, RCAN_TEST_CHANNEL2, 500000));
    runit_true(rcan_send(&pcan1, &frame_sended));
    runit_true(receive_data_frame(&pcan2, &frame_received, RCAN_TEST_RECEIVE_TIMEOUT_SEC));
    runit_true(rcan_stop(&pcan1));
    runit_true(rcan_stop(&pcan2));
    runit_true(memcmp(&frame_received, &frame_sended, sizeof(rcan_frame)) == 0);
}

static void test_send_invalid(void)
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
    runit_true(rcan_start(&pcan1, RCAN_TEST_CHANNEL1, 10000));
    runit_true(rcan_send(&pcan1, &frame_sended));

    frame_sended.len = 9;
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

static void test_is_ok_lifecycle(void)
{
    rcan pcan1 = {0};

    runit_false(rcan_is_ok(&pcan1));
    runit_false(rcan_is_ok(NULL));

    runit_true(rcan_start(&pcan1, RCAN_TEST_CHANNEL1, 500000));
    runit_true(rcan_is_ok(&pcan1));

    runit_true(rcan_stop(&pcan1));
    runit_false(rcan_is_ok(&pcan1));

    runit_true(rcan_start(&pcan1, RCAN_TEST_CHANNEL1, 500000));
    runit_true(rcan_is_ok(&pcan1));

    runit_true(rcan_stop(&pcan1));
}

static void test_send_after_stop(void)
{
    rcan       pcan1                                = {0};
    uint8_t    payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7};
    rcan_frame frame                                = {0};
    build_frame(&frame, 0x123, std_id, false, 8, payload);

    runit_true(rcan_start(&pcan1, RCAN_TEST_CHANNEL1, 1000000));
    runit_true(rcan_send(&pcan1, &frame));

    runit_true(rcan_stop(&pcan1));
    runit_false(rcan_send(&pcan1, &frame));
    runit_false(rcan_is_ok(&pcan1));
}

static void test_burst_send_receive(void)
{
    rcan       pcan1 = {0}, pcan2 = {0};
    rcan_frame frame = {0};

    runit_true(rcan_start(&pcan1, RCAN_TEST_CHANNEL1, 1000000));
    runit_true(rcan_start(&pcan2, RCAN_TEST_CHANNEL2, 1000000));

    for (uint32_t i = 0; i < 50; i++)
    {
        uint8_t payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {(uint8_t) i,
                                                        (uint8_t) (i + 1),
                                                        (uint8_t) (i + 2),
                                                        (uint8_t) (i + 3),
                                                        (uint8_t) (i + 4),
                                                        (uint8_t) (i + 5),
                                                        (uint8_t) (i + 6),
                                                        (uint8_t) (i + 7)};
        build_frame(&frame, 0x100 + i, std_id, false, 8, payload);
        send_and_verify(&pcan1, &pcan2, &frame);
    }

    runit_true(rcan_stop(&pcan1));
    runit_true(rcan_stop(&pcan2));
}

static void test_mixed_type_burst(void)
{
    rcan       pcan1 = {0}, pcan2 = {0};
    rcan_frame frame                                = {0};
    uint8_t    payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7};

    runit_true(rcan_start(&pcan1, RCAN_TEST_CHANNEL1, 500000));
    runit_true(rcan_start(&pcan2, RCAN_TEST_CHANNEL2, 500000));

    build_frame(&frame, 0x123, std_id, false, 8, payload);
    send_and_verify(&pcan1, &pcan2, &frame);

    build_frame(&frame, 0x12345, ext_id, false, 8, payload);
    send_and_verify(&pcan1, &pcan2, &frame);

    build_frame(&frame, 0x123, std_id, true, 0, NULL);
    send_and_verify(&pcan1, &pcan2, &frame);

    build_frame(&frame, 0x12345, ext_id, true, 0, NULL);
    send_and_verify(&pcan1, &pcan2, &frame);

    build_frame(&frame, 0x321, std_id, false, 8, payload);
    send_and_verify(&pcan1, &pcan2, &frame);

    runit_true(rcan_stop(&pcan1));
    runit_true(rcan_stop(&pcan2));
}

static void test_bidirectional_ping_pong(void)
{
    rcan       pcan1 = {0}, pcan2 = {0};
    rcan_frame frame = {0};

    runit_true(rcan_start(&pcan1, RCAN_TEST_CHANNEL1, 1000000));
    runit_true(rcan_start(&pcan2, RCAN_TEST_CHANNEL2, 1000000));

    for (uint32_t round = 0; round < 10; round++)
    {
        uint8_t forward_payload[1] = {(uint8_t) round};
        build_frame(&frame, 0x200 + round, std_id, false, 1, forward_payload);
        send_and_verify(&pcan1, &pcan2, &frame);

        uint8_t backward_payload[1] = {(uint8_t) (round + 1)};
        build_frame(&frame, 0x300 + round, std_id, false, 1, backward_payload);
        send_and_verify(&pcan2, &pcan1, &frame);
    }

    runit_true(rcan_stop(&pcan1));
    runit_true(rcan_stop(&pcan2));
}

static void test_boundary_payload_lengths(void)
{
    rcan       pcan1 = {0}, pcan2 = {0};
    rcan_frame frame                                = {0};
    uint8_t    payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7};

    runit_true(rcan_start(&pcan1, RCAN_TEST_CHANNEL1, 1000000));
    runit_true(rcan_start(&pcan2, RCAN_TEST_CHANNEL2, 1000000));

    for (uint8_t len = 0; len <= RCAN_MAX_FRAME_PAYLOAD_SIZE; len++)
    {
        build_frame(&frame, 0x400 + len, std_id, false, len, payload);
        send_and_verify(&pcan1, &pcan2, &frame);
    }

    runit_true(rcan_stop(&pcan1));
    runit_true(rcan_stop(&pcan2));
}

// The key test of the error-frame feature itself: a real bitrate mismatch between the two nodes
// on one physical bus reliably provokes a genuine bus error, which rcan_receive() must now
// surface as an error_frame event (instead of the old, weaker "no matching frame arrived" check).
// Backend-independent on purpose: on macOS/PCBUSB only the coarse classification and the raw
// backend status code are available (see u_can.c:peak_fill_coarse_error_frame); on Linux/SocketCAN
// individual CAN_ERR_CRTL_* events likewise don't always carry direction/counters; on Linux via
// the proprietary PEAK driver, PCAN_MESSAGE_ERRFRAME (detailed ECC breakdown,
// u_can.c:peak_fill_message_error_frame) might finally be exercised for the first time - whichever
// path fires, fields the backend didn't provide come back as the documented 0xFF sentinel, so this
// test accepts either a real value or 0xFF rather than requiring numbers, and logs whatever it saw
// via receive_error_frame()/rcan_view_frame() so the actual path taken is visible in ctest output.
static void test_bitrate_mismatch(void)
{
    rcan       pcan1 = {0}, pcan2 = {0};
    rcan_frame frame_sent = {0}, frame_received = {0}, error = {0};
    uint8_t    payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7};

    runit_true(rcan_start(&pcan1, RCAN_TEST_CHANNEL1, 500000));
    runit_true(rcan_start(&pcan2, RCAN_TEST_CHANNEL2, 1000000));

    build_frame(&frame_sent, 0x123, std_id, false, 8, payload);
    runit_true(rcan_send(&pcan1, &frame_sent));

    // Never a real match: a mismatched-rate receiver cannot correctly sample the sender's bits.
    bool received = receive_data_frame(&pcan2, &frame_received, RCAN_TEST_RECEIVE_TIMEOUT_SEC);
    runit_false(received && memcmp(&frame_received, &frame_sent, sizeof(rcan_frame)) == 0);

    // The real point of this test: an actual error_frame event must surface on at least one side.
    bool saw_error = receive_error_frame(&pcan1, &error, 1.0) || receive_error_frame(&pcan2, &error, 1.0);
    runit_true(saw_error);
    runit_true(error.type == error_frame);
    rcan_view_frame(&error);
    runit_eq(error.len, 5);
    // Accepted payload[0] values are deliberately backend-specific, not "any enum member" - the
    // latter would make this assert tautological (rcan_bus_error_type has exactly 9 values; see
    // rcan_def.h) and unable to catch a wrong classification.
    // PEAK (RCAN_TEST_USES_PEAK_API, either PCBUSB's coarse status path or the Linux proprietary
    // driver's ERRFRAME path - see peak_status_to_bus_error_type()/peak_fill_message_error_frame()
    // in u_can.c) can only ever produce busoff/controller/bit/form/stuff/unspecified - it has no
    // code path that sets rcan_err_ack, rcan_err_arbitration_lost or rcan_err_transceiver.
    // SocketCAN (socketcan_fill_error_frame()) can produce the full set, so it keeps all 9.
#if defined(RCAN_TEST_USES_PEAK_API)
    runit_true(error.payload[0] == (uint8_t) rcan_err_controller || error.payload[0] == (uint8_t) rcan_err_busoff ||
               error.payload[0] == (uint8_t) rcan_err_bit || error.payload[0] == (uint8_t) rcan_err_form ||
               error.payload[0] == (uint8_t) rcan_err_stuff || error.payload[0] == (uint8_t) rcan_err_unspecified);
#else
    runit_true(error.payload[0] == (uint8_t) rcan_err_controller || error.payload[0] == (uint8_t) rcan_err_busoff ||
               error.payload[0] == (uint8_t) rcan_err_bit || error.payload[0] == (uint8_t) rcan_err_form ||
               error.payload[0] == (uint8_t) rcan_err_stuff || error.payload[0] == (uint8_t) rcan_err_ack ||
               error.payload[0] == (uint8_t) rcan_err_arbitration_lost ||
               error.payload[0] == (uint8_t) rcan_err_transceiver || error.payload[0] == (uint8_t) rcan_err_unspecified);
#endif
    runit_neq(error.id, 0);

    runit_true(rcan_is_ok(&pcan1));
    runit_true(rcan_is_ok(&pcan2));

    runit_true(rcan_stop(&pcan1));
    runit_true(rcan_stop(&pcan2));
}

// Sanity check for the opposite direction: a healthy, matched-bitrate exchange must NOT produce
// spurious error_frame noise just because error-frame reporting is now enabled by default.
static void test_no_spurious_error_frames_when_healthy(void)
{
    rcan       pcan1 = {0}, pcan2 = {0};
    rcan_frame frame = {0}, error = {0};
    uint8_t    payload[RCAN_MAX_FRAME_PAYLOAD_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7};

    runit_true(rcan_start(&pcan1, RCAN_TEST_CHANNEL1, 1000000));
    runit_true(rcan_start(&pcan2, RCAN_TEST_CHANNEL2, 1000000));

    for (int i = 0; i < 10; i++)
    {
        build_frame(&frame, 0x500 + i, std_id, false, 8, payload);
        send_and_verify(&pcan1, &pcan2, &frame);
    }

    runit_false(receive_error_frame(&pcan1, &error, 0.2));
    runit_false(receive_error_frame(&pcan2, &error, 0.2));

    runit_true(rcan_stop(&pcan1));
    runit_true(rcan_stop(&pcan2));
}

void RCAN_TEST_ENTRYPOINT(void)
{
    test_start_stop_pcan();
    test_start_stop_pcan_twice();
    test_invalid_arg();
    test_send_std_frame();
    test_send_ext_frame();
    test_send_rtr_frame();
    test_send_ext_rtr_frame();
    test_send_invalid();
    test_is_ok_lifecycle();
    test_send_after_stop();
    test_burst_send_receive();
    test_mixed_type_burst();
    test_bidirectional_ping_pong();
    test_boundary_payload_lengths();
    test_no_spurious_error_frames_when_healthy();
    test_bitrate_mismatch();

    // Unconditional: bitrate_mismatch's own assertions can fail early (e.g. if the bus already
    // started this run in a degraded state), skipping its body before it gets a chance to heal -
    // run this directly from the aggregator so a dirty bus still gets cleaned up for the next run.
    heal_bus_after_bitrate_mismatch();
}
