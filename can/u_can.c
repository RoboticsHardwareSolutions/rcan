
#if defined(RCAN_WINDOWS) || defined(RCAN_MACOS) || defined(RCAN_UNIX)

#    include "u_can.h"
#    include "stdio.h"
#    include "unistd.h"
#    include "string.h"
#    include <nanomsg/nn.h>
#    include <nanomsg/bus.h>
#    include <uuid/uuid.h>

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

static int virtual_can_bind(rcan* can, uint32_t channel);

static bool virtual_can_start(rcan* can, uint32_t channel, uint32_t bitrate);

static bool virtual_can_read(rcan* can, rcan_frame* frame);

static bool virtual_can_write(rcan* can, rcan_frame* frame);

static bool virtual_can_stop(rcan* can);

static bool is_correct_bitrate_for_peak_can(uint32_t bitrate);

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

        can->channel   = channel;
        can->can_ready = true;
        return true;
    }

#    if defined(RCAN_WINDOWS) || defined(RCAN_MACOS)

    if (!is_correct_bitrate_for_peak_can(bitrate))
    {
        return false;
    }

    uint32_t peak_can_bitrate = convert_to_peak_can_bitrate(bitrate);

    opened = peak_can_start(can, channel, peak_can_bitrate);

#    endif  // defined(RCAN_WINDOWS) || defined (RCAN_MACOS)

#    if defined(RCAN_UNIX)
    socket = is_socket_can_iface(channel);
    peak   = is_peak_can_iface(channel);
    if (peak)
    {
        if (!is_correct_bitrate_for_peak_can(bitrate))
        {
            return false;
        }

        uint32_t peak_can_bitrate = convert_to_peak_can_bitrate(bitrate);
        opened                    = peak_can_start(can, channel, peak_can_bitrate);
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

#    if defined(RCAN_WINDOWS) || defined(RCAN_MACOS)

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

#    if defined(RCAN_WINDOWS) || defined(RCAN_MACOS)

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

#    if defined(RCAN_WINDOWS) || defined(RCAN_MACOS)

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

#    if defined(RCAN_WINDOWS) || defined(RCAN_MACOS)

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

static bool is_correct_bitrate_for_peak_can(uint32_t bitrate)
{
    if (bitrate == PCAN_BAUD_1M || bitrate == PCAN_BAUD_800K || bitrate == PCAN_BAUD_500K ||
        bitrate == PCAN_BAUD_250K || bitrate == PCAN_BAUD_125K || bitrate == PCAN_BAUD_100K ||
        bitrate == PCAN_BAUD_95K || bitrate == PCAN_BAUD_83K || bitrate == PCAN_BAUD_50K || bitrate == PCAN_BAUD_47K ||
        bitrate == PCAN_BAUD_33K || bitrate == PCAN_BAUD_20K || bitrate == PCAN_BAUD_10K || bitrate == PCAN_BAUD_5K)
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

    return true;
}

static bool peak_can_read(rcan* can, rcan_frame* frame)
{
    TPCANMsg    message = {0};
    TPCANStatus status  = 0;

    status = CAN_Read((unsigned short) can->channel, &message, NULL);

    if (status & PCAN_ERROR_QRCVEMPTY)
    {
        return false;
    }

    if (status != PCAN_ERROR_OK || message.MSGTYPE & PCAN_MESSAGE_STATUS)
    {
        return false;
    }

    // TODO read error message after you must return false

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
    if (channel == VIRTUAL_CAN_BUS0 || channel == VIRTUAL_CAN_BUS1 || channel == VIRTUAL_CAN_BUS2)
    {
        return true;
    }

    return false;
}

static int virtual_can_bind(rcan* can, uint32_t channel)
{
    char ipc[30];
    int  bus  = (int) (channel - VIRTUAL_CAN_BUS0);
    int  node = 0;
    for (int i = 0; i < RCAN_MAX_NODE_QUANTITY; i++)
    {
        sprintf(ipc, "ipc:///tmp/bus%dnode%d.ipc", bus, node++);
        if (nn_bind(can->fd, ipc) >= 0)
        {
            return node - 1;
        }
    }
    return -1;
}

static bool virtual_can_start(rcan* can, uint32_t channel, uint32_t bitrate)
{
    (void) bitrate;
    bool uuid = false;
    char ipc[30];
    int  bus = (int) (channel - VIRTUAL_CAN_BUS0);

    can->fd = nn_socket(AF_SP, NN_BUS);
    if (can->fd < 0)
    {
        return false;
    }

    int node = virtual_can_bind(can, channel);

    if (node < 0)
    {
        return false;
    }

    for (; node > 0; node--)
    {
        sprintf(ipc, "ipc:///tmp/bus%dnode%d.ipc", bus, node - 1);
        if (nn_connect(can->fd, ipc) < 0)
        {
            return false;
        }
    }
    return true;
}

static bool virtual_can_read(rcan* can, rcan_frame* frame)
{
    int recv = nn_recv(can->fd, frame, sizeof(rcan_frame), NN_DONTWAIT);

    if (recv == sizeof(rcan_frame))
    {
        return true;
    }

    return false;
}

static bool virtual_can_write(rcan* can, rcan_frame* frame)
{
    if (nn_send(can->fd, frame, sizeof(rcan_frame), 0) < 0)
    {
        return false;
    }

    return true;
}

static bool virtual_can_stop(rcan* can)
{
    nn_shutdown(can->fd, 0);
    return true;
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

static void create_vcan(const char* name)
{
    char  cli_enter[50];
    char* add    = "add";
    char* delete = "delete";

    memset(cli_enter, '\0', sizeof(cli_enter));
    sprintf(cli_enter, "sudo ip link %s dev %s", delete, name);
    system(cli_enter);

    memset(cli_enter, '\0', sizeof(cli_enter));
    sprintf(cli_enter, "sudo ip link %s dev %s type vcan", add, name);
    system(cli_enter);
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

    if (is_vcan_iface(channel))
        create_vcan(ifr.ifr_ifrn.ifrn_name);

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

static bool socet_can_read(rcan* can, rcan_frame* frame)
{
    int              nbytes;
    struct can_frame socet_can_frame;

    nbytes = read(can->fd, &socet_can_frame, sizeof(struct can_frame));

    if (nbytes < 0)
        return false;

    if (socet_can_frame.can_id & CAN_ERR_FLAG)
        return false;

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

#endif  // endif for defined(RCAN_WINDOWS) || defined (RCAN_MACOS) || defined (RCAN_UNIX)
