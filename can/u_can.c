#include "rcan_backend.h"

#if defined(RCAN_BACKEND_HOST)

#    include "u_can.h"
#    include "stdio.h"
#    include "string.h"
#    include "unistd.h"
#    include "u_can_urls.h"

#    if defined(RCAN_UNIX)

static bool translate_socket_can_name(uint32_t channel, struct ifreq* ifr);

static bool socket_can_start(rcan* can, uint32_t channel, uint32_t bitrate);

static bool socet_can_read(rcan* can, rcan_frame* frame);

static bool socet_can_write(rcan* can, rcan_frame* frame);

#    endif  // #endif  defined(RCAN_UNIX)

static bool is_peak_can_iface(uint32_t channel);

static bool is_socket_can_iface(uint32_t channel);

static bool is_vcan_iface(uint32_t channel);

static bool is_virtual_can_iface(uint32_t channel);

static bool virtual_can_start(rcan* can, uint32_t channel, uint32_t bitrate);

static bool virtual_can_read(rcan* can, rcan_frame* frame);

static bool virtual_can_write(rcan* can, rcan_frame* frame);

static bool virtual_can_stop(rcan* can);

static bool peak_can_start(rcan* can, uint32_t channel, uint32_t bitrate);

static bool peak_can_read(rcan* can, rcan_frame* frame);

static bool peak_can_write(rcan* can, rcan_frame* frame);

static uint32_t convert_to_peak_can_bitrate(uint32_t bitrate);

bool u_can_filter_preconfiguration(rcan* can, uint32_t* accepted_ids, uint32_t size)
{
    // TODO make software support filter on macOS and api compactable on linux/win
    can->use_filter = true;
    return rcan_filter_calculate(accepted_ids, size, &can->filter);
}

bool u_can_start(rcan* can, uint32_t channel, uint32_t bitrate)
{
    // TODO create function of convert all baudrate to standart.
    bool virtual = false, socket = false, peak = false;

    virtual = is_virtual_can_iface(channel);

    if (can->can_ready)
    {
        return false;
    }

    bool opened = false;

    if (virtual)
    {
        opened = virtual_can_start(can, channel, bitrate);

        if (!opened)
        {
            return false;
        }
        usleep(1000);
        can->channel   = channel;
        can->can_ready = true;
        return true;
    }

#    if defined(RCAN_MACOS)

    uint32_t pcan_bitrate = convert_to_peak_can_bitrate(bitrate);

    if (!pcan_bitrate)
        return false;

    opened = peak_can_start(can, channel, pcan_bitrate);

#    endif  // defined(RCAN_MACOS)

#    if defined(RCAN_UNIX)
    socket = is_socket_can_iface(channel);
    peak   = is_peak_can_iface(channel);
    if (peak)
    {
        uint32_t peak_can_bitrate = convert_to_peak_can_bitrate(bitrate);

        if (!peak_can_bitrate)
        {
            return false;
        }
        opened = peak_can_start(can, channel, peak_can_bitrate);
    }
    if (socket)
    {
        opened = socket_can_start(can, channel, bitrate);
    }

#    endif  // defined(RCAN_UNIX)

    if (!opened)
    {
        return false;
    }

    can->channel   = channel;
    can->can_ready = true;
    return true;
}

bool u_can_is_ok(rcan* can)
{
    if (!can->can_ready)
    {
        return false;
    }

#    if defined(RCAN_UNIX)

    if (is_peak_can_iface(can->channel))
    {
        return CAN_GetValue(can->channel, PCAN_RECEIVE_EVENT, &can->fd, sizeof(int)) == PCAN_ERROR_OK;
    }
    else
    {
        // TODO add check socet can state
        return true;
    }

#    endif

#    if defined(RCAN_MACOS)

    return CAN_GetValue((unsigned short) can->channel, PCAN_RECEIVE_EVENT, &can->fd, sizeof(int)) == PCAN_ERROR_OK;

#    endif
}

bool u_can_stop(rcan* can)
{
    if (!can->can_ready)
    {
        return false;
    }

    if (is_virtual_can_iface(can->channel))
    {
        virtual_can_stop(can);
        can->can_ready = false;
        return true;
    }

#    if defined(RCAN_MACOS)

    CAN_Uninitialize((unsigned short) can->channel);

#    endif

#    if defined(RCAN_UNIX)

    if (is_peak_can_iface(can->channel))
    {
        CAN_Uninitialize(can->channel);
    }
    else
    {
        if (close(can->fd) < 0)
            return false;
    }

#    endif

    can->can_ready = false;
    return true;
}

bool u_can_send(rcan* can, rcan_frame* frame)
{
    if (!can->can_ready)
    {
        return false;
    }

    if (is_virtual_can_iface(can->channel))
    {
        return virtual_can_write(can, frame);
    }

#    if defined(RCAN_MACOS)

    return peak_can_write(can, frame);

#    endif

#    if defined(RCAN_UNIX)

    if (is_peak_can_iface(can->channel))
        return peak_can_write(can, frame);
    else
        return socet_can_write(can, frame);

#    endif
}

bool u_can_receive(rcan* can, rcan_frame* frame)
{
    if (!can->can_ready)
    {
        return false;
    }

    if (is_virtual_can_iface(can->channel))
    {
        return virtual_can_read(can, frame);
    }

#    if defined(RCAN_MACOS)

    return peak_can_read(can, frame);

#    endif

#    if defined(RCAN_UNIX)

    if (is_peak_can_iface(can->channel))
        return peak_can_read(can, frame);
    else
        return socet_can_read(can, frame);

#    endif
    return false;
}

/*********************************************  PCAN  *****************************************************************/

static bool is_peak_can_iface(uint32_t channel)
{
    if (channel == PEAK_CAN_USBBUS1 || channel == PEAK_CAN_USBBUS2 || channel == PEAK_CAN_USBBUS3 ||
        channel == PEAK_CAN_PCIBUS1 || channel == PEAK_CAN_PCIBUS2 || channel == PEAK_CAN_PCIBUS3)
    {
        return true;
    }
    return false;
}

static uint32_t convert_to_peak_can_bitrate(uint32_t bitrate)
{
    switch (bitrate)
    {
    case 1000000:
        return PCAN_BAUD_1M;
    case 800000:
        return PCAN_BAUD_800K;
    case 500000:
        return PCAN_BAUD_500K;
    case 250000:
        return PCAN_BAUD_250K;
    case 125000:
        return PCAN_BAUD_125K;
    case 100000:
        return PCAN_BAUD_100K;
    case 95000:
        return PCAN_BAUD_95K;
    case 83000:
        return PCAN_BAUD_83K;
    case 50000:
        return PCAN_BAUD_50K;
    case 47000:
        return PCAN_BAUD_47K;
    case 33000:
        return PCAN_BAUD_33K;
    case 20000:
        return PCAN_BAUD_20K;
    case 10000:
        return PCAN_BAUD_10K;
    case 5000:
        return PCAN_BAUD_5K;
    default:
        return 0;
    }
}

static bool peak_can_start(rcan* can, uint32_t channel, uint32_t bitrate)
{
    // TODO add  filter configuration
    TPCANStatus status = 0;

    status = CAN_Initialize((unsigned short) channel, (unsigned short) bitrate, 0, 0, 0);
    if (status != PCAN_ERROR_OK)
    {
        return false;
    }

    status = CAN_GetValue((unsigned short) channel, PCAN_RECEIVE_EVENT, &can->fd, sizeof(int));
    if (status != PCAN_ERROR_OK)
    {
        return false;
    }

    // Best-effort: lets CAN_Read() surface PCAN_MESSAGE_ERRFRAME events (see peak_can_read()).
    // Not fatal if an older driver/hardware combo doesn't support this parameter.
    DWORD allow_error_frames = 1;
    CAN_SetValue((unsigned short) channel, PCAN_ALLOW_ERROR_FRAMES, &allow_error_frames, sizeof(allow_error_frames));

    return true;
}

static rcan_bus_error_type peak_status_to_bus_error_type(TPCANStatus status)
{
    if (status & PCAN_ERROR_BUSOFF)
    {
        return rcan_err_busoff;
    }
    // PCAN_ERROR_OVERRUN isn't part of PEAK's own PCAN_ERROR_ANYBUSERR mask (it's a software-side
    // "read too late" condition, not a wire-level bus error), but rcan_err_controller's own
    // contract (rcan_def.h) explicitly covers "RX/TX overflow" - so it belongs here too.
    if (status & (PCAN_ERROR_BUSHEAVY | PCAN_ERROR_BUSLIGHT | PCAN_ERROR_BUSPASSIVE | PCAN_ERROR_OVERRUN))
    {
        return rcan_err_controller;
    }
    return rcan_err_unspecified;
}

// Fills *frame as an error_frame from a CAN_Read() status code that is itself a bus-error code
// (e.g. PCAN_ERROR_BUSHEAVY) rather than PCAN_ERROR_OK. Confirmed empirically (this session, real
// PCAN-USB FD hardware): PCBUSB (macOS) surfaces bus errors exactly this way - it never queues a
// PCAN_MESSAGE_STATUS message, and CAN_SetValue(..., PCAN_ALLOW_ERROR_FRAMES, ...) returns
// PCAN_ERROR_ILLOPERATION (unsupported) - so no detailed ECC breakdown (direction/segment/error
// counters) is available on this backend; those fields are filled with 0xFF ("not provided").
static void peak_fill_coarse_error_frame(rcan_frame* frame, TPCANStatus status)
{
    memset(frame, 0, sizeof(rcan_frame));
    frame->type       = error_frame;
    frame->len        = 5;
    frame->id         = status;
    frame->payload[0] = (uint8_t) peak_status_to_bus_error_type(status);
    frame->payload[1] = RCAN_ERR_FIELD_UNDEF;  // direction: not provided by this backend
    frame->payload[2] = RCAN_ERR_FIELD_UNDEF;  // rx error counter: not provided
    frame->payload[3] = RCAN_ERR_FIELD_UNDEF;  // tx error counter: not provided
    frame->payload[4] = 0;
}

// Fills *frame as an error_frame from a successfully-read TPCANMsg flagged as
// PCAN_MESSAGE_ERRFRAME or PCAN_MESSAGE_STATUS. The ERRFRAME field layout (error type in ID,
// direction/segment/error-counters in DATA[0..3]) follows PEAK's documented PCAN-Basic error
// frame format; the ID&0xFF switch below (1=bit, 2=form, 4=stuff) is verified CORRECT by reading
// the real driver/library sources (see below) - but the PCAN_MESSAGE_ERRFRAME branch itself is
// UNREACHABLE on every real backend tried so far, for two independent reasons:
//
// - PCBUSB (macOS): never sets PCAN_MESSAGE_ERRFRAME at all - CAN_SetValue(PCAN_ALLOW_ERROR_FRAMES)
//   returns PCAN_ERROR_ILLOPERATION (unsupported). Real traffic goes through
//   peak_fill_coarse_error_frame() above instead.
//
// - Linux, PEAK's own proprietary chardev driver + official libpcanbasic (confirmed on real
//   PCAN-USB FD hardware, driver v9.3.1 - the latest available, md5-verified byte-identical to
//   PEAK's current download at the time of writing - with libpcanbasic 5.0.1.8, also their
//   current latest): PCAN_ALLOW_ERROR_FRAMES *is* supported and confirmed ON (CAN_GetValue reads
//   back 1), and the driver's pcan_handle_error_msg() (driver/src/pcan_main.c) genuinely decodes
//   BIT_ERR/FRM_ERR/STF_ERR/OTH_ERR - including from a hardware error GENERATOR
//   (test/pcanerrgen, ioctl PCANFD_OPT_ERR_GEN, confirmed PCANFD_FEATURE_ERR_GEN on both
//   adapters) that deterministically corrupts a chosen bit of a chosen frame on the wire. Tried
//   bitrate mismatches (5 pairs, the full available canonical range) AND pcanerrgen (single and
//   periodic, several bit positions, 3500+ induced events) - every single one surfaced only as
//   PCAN_MESSAGE_STATUS (coarse controller state change), never PCAN_MESSAGE_ERRFRAME.
//   Root cause, confirmed straight from the driver's own headers:
//     /usr/include/pcanfd.h: "#define PCANFD_TYPE_ERROR_MSG 4 /* Deprecated in v9 (see
//                             PCANXL_TYPE_ERROR) */"
//     /usr/include/pcanxl.h: "PCANXL_TYPE_ERROR supersedes PCANFD_TYPE_ERROR_MSG. ...
//                             PCANFD_TYPE_ERROR is not posted anymore."
//   i.e. driver v9+ only ever emits the newer PCANXL_TYPE_ERROR (id 6), and libpcanbasic 5.0.1.8's
//   pcbcore.c has zero handling for PCANXL_TYPE_ERROR anywhere in the file (grep comes up empty) -
//   it still only recognizes the deprecated PCANFD_TYPE_ERROR_MSG (id 4). This is a gap in PEAK's
//   own current release (libpcanbasic's 5.0.0 changelog claims "Added support to CAN XL features"
//   but evidently never wired PCANXL_TYPE_ERROR into the classic TPCANMsg read path used here),
//   not something fixable from rcan's side short of talking to /dev/pcanusbfdN via
//   libpcanfd/libpcanxl directly instead of PCAN-Basic.
//
// FIXME: the PCAN_MESSAGE_ERRFRAME branch below is effectively dead code with every
// driver/library combination tried so far - its correctness is verified by source reading only,
// never by an actual message flowing through it on real hardware.
// TODO: re-check whether a future libpcanbasic release wires PCANXL_TYPE_ERROR into the classic
// TPCANMsg/CAN_Read() path (watch its changelog.md), or switch this backend to libpcanfd/libpcanxl
// directly to read PCANXL_TYPE_ERROR ourselves - whichever happens first.
static void peak_fill_message_error_frame(rcan_frame* frame, const TPCANMsg* message)
{
    memset(frame, 0, sizeof(rcan_frame));
    frame->type = error_frame;
    frame->len  = 5;
    frame->id   = message->ID;

    if (message->MSGTYPE & PCAN_MESSAGE_ERRFRAME)
    {
        switch (message->ID & 0xFF)
        {
        case 1:
            frame->payload[0] = (uint8_t) rcan_err_bit;
            break;
        case 2:
            frame->payload[0] = (uint8_t) rcan_err_form;
            break;
        case 4:
            frame->payload[0] = (uint8_t) rcan_err_stuff;
            break;
        default:
            frame->payload[0] = (uint8_t) rcan_err_unspecified;  // 0 = counter update, 8 = other
            break;
        }
        frame->payload[1] = message->DATA[0] ? 1 : 0;  // direction bit
        frame->payload[2] = message->DATA[2];          // rx error counter
        frame->payload[3] = message->DATA[3];          // tx error counter
        frame->payload[4] = message->DATA[1];          // segment
    }
    else
    {
        // Plain PCAN_MESSAGE_STATUS, no ECC detail available - but NOT a dead branch: this is the
        // path real bus errors (bitrate mismatch, pcanerrgen) actually take on the Linux
        // proprietary driver (confirmed on real PCAN-USB FD hardware - see the comment above).
        // libpcanbasic's pcbcore.c packs the condition into DATA[0..3] as a big-endian 32-bit word
        // (its own TRC_DATA_STATUS_FRAME_* constants are literally #defined to the same numeric
        // values as the TPCANStatus PCAN_ERROR_* bits - e.g. BUSOFF=0x10, BUSHEAVY=0x8,
        // BUSPASSIVE=0x40000 - confirmed by reading pcbcore.c directly), so reassembling it and
        // reusing peak_status_to_bus_error_type() recovers the real classification (busoff vs.
        // controller) instead of hardcoding controller for every status, including BUSOFF.
        TPCANStatus status_data = ((TPCANStatus) message->DATA[0] << 24) | ((TPCANStatus) message->DATA[1] << 16) |
                                   ((TPCANStatus) message->DATA[2] << 8) | (TPCANStatus) message->DATA[3];
        frame->payload[0] = (uint8_t) peak_status_to_bus_error_type(status_data);
        frame->payload[1] = RCAN_ERR_FIELD_UNDEF;
        frame->payload[2] = RCAN_ERR_FIELD_UNDEF;
        frame->payload[3] = RCAN_ERR_FIELD_UNDEF;
    }
}

static bool peak_can_read(rcan* can, rcan_frame* frame)
{
    TPCANMsg    message = {0};
    TPCANStatus status  = 0;

    status = CAN_Read((unsigned short) can->channel, &message, NULL);

    // A successful data-frame read reports status == PCAN_ERROR_OK (0), which has neither bit
    // below set, so this must only treat the read as empty when QRCVEMPTY is actually set - not
    // whenever no bus-error bit happens to be present (that would discard every real frame).
    // PCAN_ERROR_* bits combine (e.g. QRCVEMPTY sticks alongside a bus-error bit while the bus is
    // degraded - see peak_fill_coarse_error_frame()'s comment), so testing for QRCVEMPTY alone
    // would still miss a real bus error reported in the same status word; a status of
    // QRCVEMPTY|XMTFULL (XMTFULL can linger from an earlier CAN_Write(), unrelated to this read)
    // must still count as empty though, since XMTFULL isn't one of the bits
    // peak_status_to_bus_error_type() classifies.
    if ((status & PCAN_ERROR_QRCVEMPTY) && !(status & (PCAN_ERROR_ANYBUSERR | PCAN_ERROR_OVERRUN)))
    {
        return false;
    }

    if (status != PCAN_ERROR_OK)
    {
        // The only error-diagnostic path confirmed to work on PCBUSB/macOS - see
        // peak_fill_coarse_error_frame()'s comment.
        peak_fill_coarse_error_frame(frame, status);
        return true;
    }

    if (message.MSGTYPE & (PCAN_MESSAGE_ERRFRAME | PCAN_MESSAGE_STATUS))
    {
        peak_fill_message_error_frame(frame, &message);
        return true;
    }

    frame->len = message.LEN;
    frame->id  = message.ID;

    if (message.MSGTYPE & PCAN_MESSAGE_EXTENDED)
    {
        frame->type = ext_id;
    }
    else
    {
        frame->type = std_id;
    }

    if (message.MSGTYPE & PCAN_MESSAGE_RTR)
    {
        frame->rtr = true;
    }
    else
    {
        memcpy(frame->payload, &message.DATA, message.LEN);
    }

    return true;
}

static bool peak_can_write(rcan* can, rcan_frame* frame)
{
    TPCANMsg message = {0};

    if (frame->type == std_id)
    {
        if (frame->id > RCAN_STD_ID_MAX)
        {
            return false;
        }

        message.MSGTYPE |= PCAN_MESSAGE_STANDARD;
    }
    else if (frame->type == ext_id)
    {
        if (frame->id > RCAN_EXT_ID_MAX)
        {
            return false;
        }

        message.MSGTYPE |= PCAN_MESSAGE_EXTENDED;
    }

    message.ID  = frame->id;
    message.LEN = frame->len;

    if (frame->rtr)
    {
        message.MSGTYPE |= PCAN_MESSAGE_RTR;
    }
    else
    {
        memcpy(message.DATA, frame->payload, frame->len);
    }

    return CAN_Write((unsigned short) can->channel, &message) == PCAN_ERROR_OK ? true : false;
}

/***************************************  VIRTUAL CAN  ****************************************************************/

static bool is_virtual_can_iface(uint32_t channel)
{
    if (channel == VIRTUAL_INPROC_CAN_BUS0 || channel == VIRTUAL_INPROC_CAN_BUS1 ||
        channel == VIRTUAL_INPROC_CAN_BUS2 || channel == VIRTUAL_IPC_CAN_BUS0 || channel == VIRTUAL_IPC_CAN_BUS1 ||
        channel == VIRTUAL_IPC_CAN_BUS2)
    {
        return true;
    }
    return false;
}

static bool is_ipc(uint32_t channel)
{
    if (channel == VIRTUAL_IPC_CAN_BUS0 || channel == VIRTUAL_IPC_CAN_BUS1 || channel == VIRTUAL_IPC_CAN_BUS2)
    {
        return true;
    }
    return false;
}

static bool virtual_can_start(rcan* can, uint32_t channel, uint32_t bitrate)
{
    char url[30];
    int  bus = is_ipc(channel) ? (channel - VIRTUAL_IPC_CAN_BUS0) : (channel - VIRTUAL_INPROC_CAN_BUS0);

    int written = snprintf(url, sizeof(url), "%sbus%dnode", is_ipc(channel) ? URL_CAN_INTER_PROC : URL_CAN_INPROC, bus);
    if (written < 0 || (size_t) written >= sizeof(url))
        return false;

    return rnode_create(&can->node, url);
}

static bool virtual_can_read(rcan* can, rcan_frame* frame)
{
    return rnode_receive(&can->node, frame, sizeof(rcan_frame));
}

static bool virtual_can_write(rcan* can, rcan_frame* frame)
{
    return rnode_send(&can->node, frame, sizeof(rcan_frame));
}

static bool virtual_can_stop(rcan* can)
{
    return rnode_delete(&can->node);
}

/***************************************  SOCET CAN   *****************************************************************/

static bool is_socket_can_iface(uint32_t channel)
{
    if (channel == SOCKET_VCAN0 || channel == SOCKET_VCAN1 || channel == SOCKET_VCAN2 || channel == SOCKET_CAN0 ||
        channel == SOCKET_CAN1 || channel == SOCKET_CAN2)
    {
        return true;
    }

    return false;
}

static bool is_vcan_iface(uint32_t channel)
{
    return channel == SOCKET_VCAN0 || channel == SOCKET_VCAN1 || channel == SOCKET_VCAN2 ? true : false;
}

#    if defined(RCAN_UNIX)

static bool create_vcan(const char* name)
{
    char cli_enter[64];

    // Only create the interface if it doesn't exist yet: unconditionally deleting and
    // re-adding it on every rcan_start() would assign it a new ifindex each time, which
    // silently breaks any `cangw`/tc rule a test fixture bound to the previous one.
    snprintf(cli_enter, sizeof(cli_enter), "ip link show dev %s > /dev/null 2>&1", name);
    if (system(cli_enter) == 0)
        return true;

    snprintf(cli_enter, sizeof(cli_enter), "sudo ip link add dev %s type vcan", name);
    return system(cli_enter) == 0;
}

static bool translate_socket_can_name(uint32_t channel, struct ifreq* ifr)
{
    if (channel == SOCKET_VCAN0)
        strcpy(ifr->ifr_ifrn.ifrn_name, "vcan0");
    else if (channel == SOCKET_VCAN1)
        strcpy(ifr->ifr_ifrn.ifrn_name, "vcan1");
    else if (channel == SOCKET_VCAN2)
        strcpy(ifr->ifr_ifrn.ifrn_name, "vcan2");
    else if (channel == SOCKET_CAN0)
        strcpy(ifr->ifr_ifrn.ifrn_name, "can0");
    else if (channel == SOCKET_CAN1)
        strcpy(ifr->ifr_ifrn.ifrn_name, "can1");
    else if (channel == SOCKET_CAN2)
        strcpy(ifr->ifr_ifrn.ifrn_name, "can2");
    else
        return false;

    return true;
}

static bool socket_can_start(rcan* can, uint32_t channel, uint32_t bitrate)
{
    // TODO add bitrate and filter configuration
    struct sockaddr_can addr;
    struct ifreq        ifr;

    //    struct can_device_stats can_status = {0};
    //    struct can_ctrlmode can_ctrl_mode = {0};
    //    int can_state = 0;

    if (!translate_socket_can_name(channel, &ifr))
        return false;

    //    if (can_get_ctrlmode(ifr.ifr_ifrn.ifrn_name, &can_ctrl_mode) != 0)
    //        printf("get ctrl mode error\r\n");
    //
    //    if (can_get_state(ifr.ifr_ifrn.ifrn_name, &can_state) != 0)
    //        printf("get state error\r\n");
    //
    //    if (can_get_device_stats(ifr.ifr_ifrn.ifrn_name, &can_status) != 0)
    //        printf("get device state error\r\n");

    if (is_vcan_iface(channel) && !create_vcan(ifr.ifr_ifrn.ifrn_name))
        return false;

    if (can_do_stop(ifr.ifr_ifrn.ifrn_name) != 0)
        return false;

    if (!is_vcan_iface(channel))
    {
        if (can_set_bitrate(ifr.ifr_ifrn.ifrn_name, bitrate) != 0)
            return false;
    }

    if (can_do_start(ifr.ifr_ifrn.ifrn_name) != 0)
        return false;

    if ((can->fd = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
        return false;

    // Best-effort: without this, the kernel's default error mask is empty and error frames
    // (CAN_ERR_FLAG) are never delivered to this socket at all (see socet_can_read()). Not fatal
    // if it fails - the socket just won't receive error frames.
    can_err_mask_t err_mask = CAN_ERR_MASK;
    setsockopt(can->fd, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &err_mask, sizeof(err_mask));

    ioctl(can->fd, SIOCGIFINDEX, &ifr);

    int flags = fcntl(can->fd, F_GETFL, 0);
    fcntl(can->fd, F_SETFL, flags | O_NONBLOCK);

    memset(&addr, 0, sizeof(addr));
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(can->fd, (struct sockaddr*) &addr, sizeof(addr)) < 0)
        return false;

    return true;
}

// Classifies and fills *frame from a SocketCAN error frame (can_id & CAN_ERR_FLAG), per the
// layout documented in linux/can/error.h (verified against the kernel header directly).
static void socketcan_fill_error_frame(rcan_frame* frame, const struct can_frame* socet_can_frame)
{
    canid_t class = socet_can_frame->can_id;

    memset(frame, 0, sizeof(rcan_frame));
    frame->type = error_frame;
    frame->len  = 5;
    frame->id   = class & CAN_ERR_MASK;

    if (class & CAN_ERR_BUSOFF)
    {
        frame->payload[0] = (uint8_t) rcan_err_busoff;
    }
    // BIT0/BIT1 (unable to send dominant/recessive bit) are bit-level protocol violations just
    // like BIT (single-bit error) - group them under the same rcan_err_bit so a controller that
    // only ever reports BIT0/BIT1 (common on some chips) doesn't fall through to "unspecified".
    else if (class & CAN_ERR_PROT &&
             socet_can_frame->data[2] & (CAN_ERR_PROT_BIT | CAN_ERR_PROT_BIT0 | CAN_ERR_PROT_BIT1))
    {
        frame->payload[0] = (uint8_t) rcan_err_bit;
    }
    else if (class & CAN_ERR_PROT && socet_can_frame->data[2] & CAN_ERR_PROT_FORM)
    {
        frame->payload[0] = (uint8_t) rcan_err_form;
    }
    else if (class & CAN_ERR_PROT && socet_can_frame->data[2] & CAN_ERR_PROT_STUFF)
    {
        frame->payload[0] = (uint8_t) rcan_err_stuff;
    }
    // CAN_ERR_PROT_OVERLOAD, _ACTIVE and _TX deliberately fall through to rcan_err_unspecified
    // below: none of the existing rcan_bus_error_type buckets describes them accurately (ACTIVE/TX
    // are modifier flags, not error kinds, and there's no "bus overload" category), and the raw
    // byte stays available in payload[4] for diagnostics. Mirrors the PCAN ERRFRAME path
    // (peak_fill_message_error_frame above), which has the same honest gap for its own "other"
    // code (8) - matching it here keeps unspecified's meaning the same on both backends instead of
    // inventing a mapping neither side can verify.
    else if (class & CAN_ERR_CRTL)
    {
        frame->payload[0] = (uint8_t) rcan_err_controller;
    }
    else if (class & CAN_ERR_ACK)
    {
        frame->payload[0] = (uint8_t) rcan_err_ack;
    }
    else if (class & CAN_ERR_LOSTARB)
    {
        frame->payload[0] = (uint8_t) rcan_err_arbitration_lost;
    }
    else if (class & CAN_ERR_TRX)
    {
        frame->payload[0] = (uint8_t) rcan_err_transceiver;
    }
    else
    {
        frame->payload[0] = (uint8_t) rcan_err_unspecified;
    }

    frame->payload[1] =
        (class & CAN_ERR_PROT) ? ((socet_can_frame->data[2] & CAN_ERR_PROT_TX) ? 0 : 1) : RCAN_ERR_FIELD_UNDEF;
    if (class & (CAN_ERR_ACK | CAN_ERR_LOSTARB))
    {
        // Neither can occur while receiving: ACK errors only happen on transmission (no peer
        // acknowledged the frame we sent), and arbitration is only contested by a node that is
        // itself trying to transmit - so direction is known here without needing any extra data.
        frame->payload[1] = 0;
    }
    // Counter order is NOT the same as PCAN: data[6]=TX, data[7]=RX (kernel header, not a typo).
    frame->payload[2] =
        (class & CAN_ERR_CNT) ? socet_can_frame->data[7] : RCAN_ERR_FIELD_UNDEF;  // rx error counter
    frame->payload[3] =
        (class & CAN_ERR_CNT) ? socet_can_frame->data[6] : RCAN_ERR_FIELD_UNDEF;  // tx error counter
    // Per linux/can/error.h: data[1]=CRTL detail, data[4]=TRX detail (CAN_ERR_TRX_CANH/L_*), data[3]
    // (CAN_ERR_PROT location) is the only meaningful fallback for the rest (busoff/ack/lostarb/
    // unspecified have no dedicated data index of their own).
    if (class & CAN_ERR_CRTL)
        frame->payload[4] = socet_can_frame->data[1];
    else if (class & CAN_ERR_TRX)
        frame->payload[4] = socet_can_frame->data[4];
    else
        frame->payload[4] = socet_can_frame->data[3];
}

static bool socet_can_read(rcan* can, rcan_frame* frame)
{
    int              nbytes;
    struct can_frame socet_can_frame;

    nbytes = read(can->fd, &socet_can_frame, sizeof(struct can_frame));

    if (nbytes < 0)
        return false;

    if (socet_can_frame.can_id & CAN_ERR_FLAG)
    {
        socketcan_fill_error_frame(frame, &socet_can_frame);
        return true;
    }

    if (socet_can_frame.can_id & CAN_EFF_FLAG)
    {
        frame->id   = socet_can_frame.can_id & RCAN_EXT_ID_MAX;
        frame->type = ext_id;
    }
    else
    {
        frame->id   = socet_can_frame.can_id & RCAN_STD_ID_MAX;
        frame->type = std_id;
    }

    frame->rtr = socet_can_frame.can_id & CAN_RTR_FLAG ? true : false;
    frame->len = socet_can_frame.can_dlc;

    if (!frame->rtr)
        memcpy(frame->payload, socet_can_frame.data, frame->len);

    return true;
}

static bool socet_can_write(rcan* can, rcan_frame* frame)
{
    struct can_frame socet_can_frame;

    socet_can_frame.can_id = frame->id;

    if (frame->rtr)
        socet_can_frame.can_id |= CAN_RTR_FLAG;

    if (frame->type == ext_id)
    {
        if (frame->id > RCAN_EXT_ID_MAX)
            return false;

        socet_can_frame.can_id |= CAN_EFF_FLAG;
    }
    else if (frame->type == std_id)
    {
        socet_can_frame.can_id &= ~CAN_EFF_FLAG;

        if (frame->id > RCAN_STD_ID_MAX)
            return false;
    }

    socet_can_frame.can_dlc = frame->len;
    memcpy(socet_can_frame.data, frame->payload, frame->len);

    if (write(can->fd, &socet_can_frame, sizeof(struct can_frame)) != sizeof(struct can_frame))
        return false;

    return true;
}

#    endif  // defined(RCAN_UNIX)

#endif  // defined(RCAN_BACKEND_HOST)
