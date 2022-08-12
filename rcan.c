#include "rcan.h"



#if defined(RCAN_WINDOWS) || defined (RCAN_MACOS) || defined (RCAN_UNIX)
#include "stdio.h"
#include "string.h"
#endif






#if defined(RCAN_WINDOWS) || defined (RCAN_MACOS) || defined (RCAN_UNIX)

#if defined(RCAN_UNIX)

static bool is_socket_can_iface(uint32_t channel);

static bool is_vcan_iface(uint32_t channel);

static bool translate_socket_can_name(uint32_t channel, struct ifreq *ifr);

static bool socket_can_start(rcan *can, uint32_t channel, uint32_t bitrate);

static bool socet_can_read(rcan *can, rcan_frame *frame);

static bool socet_can_write(rcan *can, rcan_frame *frame);

#endif // #endif  defined(RCAN_UNIX)

#if defined(RCAN_WINDOWS) || defined (RCAN_MACOS) || defined (RCAN_UNIX)

static bool is_pcan_iface(uint32_t channel);

static bool is_correct_bitrate_for_pcan(uint32_t bitrate);

static bool pcan_start(rcan *can, uint32_t channel, uint32_t bitrate);

static bool pcan_read(rcan *can, rcan_frame *frame);

static bool pcan_write(rcan *can, rcan_frame *frame);

#endif // endif for defined(RCAN_WINDOWS) || defined (RCAN_MACOS) || defined (RCAN_UNIX)

bool rcan_filter_preconfiguration(rcan *can, uint32_t *accepted_ids, uint32_t size) {
    //TODO make software support filter on macOS and api compactable on linux/win

    if (can == NULL || accepted_ids == NULL || size == 0)
        return false;

    can->use_filter = true;
    return rcan_filter_calculate(accepted_ids, size, &can->filter);

}

bool rcan_start(rcan *can, uint32_t channel, uint32_t bitrate) {
    //TODO create function of convert all baudrate to standart.

    if (can == NULL || can->opened)
        return false;

    bool success = false;

#if defined(RCAN_WINDOWS) || defined (RCAN_MACOS)

    success = pcan_start(can, channel, bitrate);

#endif

#if defined(RCAN_UNIX)
    if (channel == 0)
        return false;

    if (is_pcan_iface(channel)) {

        if (!is_correct_bitrate_for_pcan(bitrate))
            return false;
        success = pcan_start(can, channel, bitrate);

    } else if (is_socket_can_iface(channel))
        success = socket_can_start(can, channel, bitrate);

#endif

    if (!success)
        return false;

    can->channel = channel;
    can->opened = true;
    return true;
}

bool rcan_is_ok(rcan *can) {

    if (!can->opened || can == NULL)
        return false;

#if defined(RCAN_UNIX)

    if (is_pcan_iface(can->channel)) {
        return CAN_GetValue(can->channel, PCAN_RECEIVE_EVENT, &can->fd, sizeof(int)) == PCAN_ERROR_OK;
    } else {
        //TODO add check socet can state 
        return true;
    }

#endif

#if defined(RCAN_WINDOWS) || defined (RCAN_MACOS)

    return CAN_GetValue(can->channel, PCAN_RECEIVE_EVENT, &can->fd, sizeof(int)) == PCAN_ERROR_OK;

#endif

}

bool rcan_stop(rcan *can) {

    if (can == NULL || !can->opened)
        return false;

#if defined(RCAN_WINDOWS) || defined (RCAN_MACOS)

    CAN_Uninitialize(can->channel);

#endif

#if defined(RCAN_UNIX)

    if (is_pcan_iface(can->channel)) {

        CAN_Uninitialize(can->channel);

    } else {

        if (close(can->fd) < 0) {
            return false;
        }

    }

#endif

    can->opened = false;
    return true;

}

bool rcan_send(rcan *can, rcan_frame *frame) {

    if (can == NULL ||
        frame == NULL ||
        !can->opened ||
        frame->type == nonframe ||
        frame->len > RCAN_MAX_FRAME_PAYLOAD_SIZE)
        return false;

#if defined(RCAN_WINDOWS) || defined (RCAN_MACOS)

    return pcan_write(can, frame);

#endif

#if defined(RCAN_UNIX)

    if (is_pcan_iface(can->channel))
        return pcan_write(can, frame);
    else
        return socet_can_write(can, frame);

#endif

}

bool rcan_receive(rcan *can, rcan_frame *frame) {

    if (can == NULL || frame == NULL || !can->opened)
        return false;

#if defined(RCAN_WINDOWS) || defined (RCAN_MACOS)

    return pcan_read(can, frame);

#endif

#if defined(RCAN_UNIX)

    if (is_pcan_iface(can->channel))
        return pcan_read(can, frame);
    else
        return socet_can_read(can, frame);

#endif

}

/*********************************************  PCAN  *****************************************************************/

static bool is_pcan_iface(uint32_t channel) {

#if defined(RCAN_UNIX)
    if (channel == PCAN_USBBUS1 || channel == PCAN_USBBUS2 || channel == PCAN_USBBUS3 ||
        channel == PCAN_PCIBUS1 || channel == PCAN_PCIBUS2 || channel == PCAN_PCIBUS3)
        return true;
#endif

    return false;
}

static bool is_correct_bitrate_for_pcan(uint32_t bitrate) {

    if (bitrate == PCAN_BAUD_1M || bitrate == PCAN_BAUD_800K || bitrate == PCAN_BAUD_500K ||
        bitrate == PCAN_BAUD_250K || bitrate == PCAN_BAUD_125K || bitrate == PCAN_BAUD_100K ||
        bitrate == PCAN_BAUD_95K || bitrate == PCAN_BAUD_83K || bitrate == PCAN_BAUD_50K ||
        bitrate == PCAN_BAUD_47K || bitrate == PCAN_BAUD_33K || bitrate == PCAN_BAUD_20K ||
        bitrate == PCAN_BAUD_10K || bitrate == PCAN_BAUD_5K)
        return true;

    return false;
}

static bool pcan_start(rcan *can, uint32_t channel, uint32_t bitrate) {

    //TODO create function of convert all baudrate to standart.
    //TODO add  filter configuration
    TPCANStatus status;

    status = CAN_Initialize(channel, bitrate, 0, 0, 0);
    if (status != PCAN_ERROR_OK)
        return false;

    status = CAN_GetValue(channel, PCAN_RECEIVE_EVENT, &can->fd, sizeof(int));
    if (status != PCAN_ERROR_OK)
        return false;

    return true;
}

static bool pcan_read(rcan *can, rcan_frame *frame) {

    TPCANMsg message = {0};
    TPCANStatus status;

    status = CAN_Read(can->channel, &message, NULL);

    if (status & PCAN_ERROR_QRCVEMPTY)
        return false;
    if (status != PCAN_ERROR_OK || message.MSGTYPE & PCAN_MESSAGE_STATUS)
        return false;

    // TODO read error message after you must return false

    frame->len = message.LEN;
    frame->id = message.ID;

    if (message.MSGTYPE & PCAN_MESSAGE_EXTENDED)
        frame->type = ext_id;
    else
        frame->type = std_id;


    if (message.MSGTYPE & PCAN_MESSAGE_RTR)
        frame->rtr = true;
    else
        memcpy(frame->payload, &message.DATA, message.LEN);

    return true;
}

static bool pcan_write(rcan *can, rcan_frame *frame) {

    TPCANMsg message = {0};

    if (frame->type == std_id) {

        if (frame->id > RCAN_STD_ID_MAX)
            return false;

        message.MSGTYPE |= PCAN_MESSAGE_STANDARD;


    } else if (frame->type == ext_id) {

        if (frame->id > RCAN_EXT_ID_MAX)
            return false;

        message.MSGTYPE |= PCAN_MESSAGE_EXTENDED;
    }


    message.ID = frame->id;
    message.LEN = frame->len;

    if (frame->rtr)
        message.MSGTYPE |= PCAN_MESSAGE_RTR;
    else
        memcpy(message.DATA, frame->payload, frame->len);

    return CAN_Write(can->channel, &message) == PCAN_ERROR_OK ? true : false;

}


/***************************************  SOCET CAN   *****************************************************************/

#if defined(RCAN_UNIX)

static bool is_socket_can_iface(uint32_t channel) {

    if (channel == SOCKET_VCAN0 || channel == SOCKET_VCAN1 || channel == SOCKET_VCAN2 ||
        channel == SOCKET_CAN0 || channel == SOCKET_CAN1 || channel == SOCKET_CAN2)
        return true;

    return false;
}

static bool is_vcan_iface(uint32_t channel) {
    return channel == SOCKET_VCAN0 || channel == SOCKET_VCAN1 || channel == SOCKET_VCAN2 ? true : false;
}


static void create_vcan(const char *name) {

    char cli_enter[50];
    char *add = "add";
    char *delete = "delete";

    memset(cli_enter, '\0', sizeof(cli_enter));
    sprintf(cli_enter, "sudo ip link %s dev %s", delete, name);
    system(cli_enter);

    memset(cli_enter, '\0', sizeof(cli_enter));
    sprintf(cli_enter, "sudo ip link %s dev %s type vcan", add, name);
    system(cli_enter);

}


static bool translate_socket_can_name(uint32_t channel, struct ifreq *ifr) {

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

static bool socket_can_start(rcan *can, uint32_t channel, uint32_t bitrate) {

    //TODO add bitrate and filter configuration
    struct sockaddr_can addr;
    struct ifreq ifr;

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

    if (!is_vcan_iface(channel)) {

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
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(can->fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
        return false;

    return true;
}

static bool socet_can_read(rcan *can, rcan_frame *frame) {

    int nbytes;
    struct can_frame socet_can_frame;

    nbytes = read(can->fd, &socet_can_frame, sizeof(struct can_frame));

    if (nbytes < 0)
        return false;

    if (socet_can_frame.can_id & CAN_ERR_FLAG)
        return false;

    if (socet_can_frame.can_id & CAN_EFF_FLAG) {
        frame->id = socet_can_frame.can_id & RCAN_EXT_ID_MAX;
        frame->type = ext_id;
    } else {
        frame->id = socet_can_frame.can_id & RCAN_STD_ID_MAX;
        frame->type = std_id;
    }

    frame->rtr = socet_can_frame.can_id & CAN_RTR_FLAG ? true : false;
    frame->len = socet_can_frame.can_dlc;

    if (!frame->rtr)
        memcpy(frame->payload, socet_can_frame.data, frame->len);

    return true;
}

static bool socet_can_write(rcan *can, rcan_frame *frame) {

    struct can_frame socet_can_frame;

    socet_can_frame.can_id = frame->id;

    if (frame->rtr)
        socet_can_frame.can_id |= CAN_RTR_FLAG;

    if (frame->type == ext_id) {

        if (frame->id > RCAN_EXT_ID_MAX)
            return false;

        socet_can_frame.can_id |= CAN_EFF_FLAG;


    } else if (frame->type == std_id) {

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

#endif

#endif //  endif for !defined(STM32G474xx || STM32F767xx)


