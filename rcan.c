#include "rcan.h"
#include "stdio.h"
#include "string.h"

#define RCAN_EXT_ID_MAX                      0x1FFFFFFFU
#define RCAN_STD_ID_MAX                      0x000007FFU

#define RCAN_MAX_FRAME_PAYLOAD_SIZE                    8

#if defined(STM32G474xx)

static bool rcan_set_filter(rcan *can);

static bool rcan_set_timing(rcan *can, uint32_t bitrate);

static bool rcan_set_data_timing(rcan *can, uint32_t data_bitrate);

static bool rcan_make_can_tx_header(rcan_frame *frame, FDCAN_TxHeaderTypeDef *tx_header);


bool rcan_filter_preconfiguration(rcan *can, uint32_t *accepted_ids, uint32_t size) {

    if (can == NULL || accepted_ids == NULL || size == 0)
        return false;

    can->use_filter = true;
    return rcan_filter_calculate(accepted_ids, size, &can->filter);

}


bool rcan_start(rcan *can, uint32_t channel, uint32_t bitrate) {

    if (can == NULL || channel == 0 || bitrate == 0)
        return false;


    can->handle.Instance = (FDCAN_GlobalTypeDef *) channel;
    can->handle.Init.ClockDivider = FDCAN_CLOCK_DIV1;
    can->handle.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    can->handle.Init.Mode = FDCAN_MODE_NORMAL;

    can->handle.Init.AutoRetransmission = ENABLE;
    can->handle.Init.TransmitPause = DISABLE;
    can->handle.Init.ProtocolException = DISABLE;

    can->handle.Init.StdFiltersNbr = 1;
    can->handle.Init.ExtFiltersNbr = 1;
    can->handle.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

    if (!rcan_set_timing(can, bitrate))
        return false;

    if (!rcan_set_data_timing(can, 0)) // FIXME data_bitrate
        return false;

    if (HAL_FDCAN_Init(&can->handle) != HAL_OK)
        return false;

    if (can->use_filter) {
        if (!rcan_set_filter(can))
            return false;
    }

    return HAL_FDCAN_Start(&can->handle) == HAL_OK;

}

bool rcan_is_ok(rcan *can) {

    if (HAL_FDCAN_GetError(&can->handle) != HAL_FDCAN_ERROR_NONE)
        return false;

    if (HAL_FDCAN_GetState(&can->handle) == HAL_FDCAN_STATE_RESET)
        return false;

    if (HAL_FDCAN_GetState(&can->handle) == HAL_FDCAN_STATE_ERROR)
        return false;

    if (__HAL_FDCAN_GET_FLAG(&can->handle, FDCAN_FLAG_RX_FIFO0_MESSAGE_LOST)) {
        __HAL_FDCAN_CLEAR_FLAG(&can->handle, FDCAN_FLAG_RX_FIFO0_MESSAGE_LOST);
        return false;
    }

    FDCAN_ProtocolStatusTypeDef status;

    if (HAL_FDCAN_GetProtocolStatus(&can->handle, &status) != HAL_OK)
        return false;

    if (status.BusOff == 1 || status.ErrorPassive == 1) {
        __HAL_FDCAN_CLEAR_FLAG(&can->handle, FDCAN_FLAG_ERROR_PASSIVE);
        __HAL_FDCAN_CLEAR_FLAG(&can->handle, FDCAN_FLAG_BUS_OFF);
        return false;
    }

    if (status.LastErrorCode == FDCAN_PROTOCOL_ERROR_NO_CHANGE)
        return true;

    if (status.LastErrorCode != FDCAN_PROTOCOL_ERROR_NONE)
        return false;

    return true;
}


bool rcan_stop(rcan *can) {
    HAL_Delay(1);

    if (HAL_OK != HAL_FDCAN_AbortTxRequest(
            &can->handle,
            FDCAN_TX_BUFFER0 | FDCAN_TX_BUFFER1 | FDCAN_TX_BUFFER2)) {
        return false;
    }

    if (HAL_OK != HAL_FDCAN_Stop(&can->handle)) {
        return false;
    }
    if (HAL_OK != HAL_FDCAN_DeInit(&can->handle)) {
        return false;
    }
    return true;
}


bool rcan_send(rcan *can, rcan_frame *frame) {

    if (can == NULL ||
        frame == NULL ||
        frame->type == nonframe ||
        frame->payload == NULL ||
        frame->len > RCAN_MAX_FRAME_PAYLOAD_SIZE)
        return false;

    if (HAL_FDCAN_GetTxFifoFreeLevel(&can->handle) == 0)
        return false;

    FDCAN_TxHeaderTypeDef tx_header = {0};

    if (!rcan_make_can_tx_header(frame, &tx_header))
        return false;

    return HAL_FDCAN_AddMessageToTxFifoQ(&can->handle, &tx_header, frame->payload) == HAL_OK;
}


bool rcan_receive(rcan *can, rcan_frame *frame) {
    if (can == NULL || frame == NULL || frame->payload == NULL)
        return false;

    uint32_t fifo = 0;
    if (HAL_FDCAN_GetRxFifoFillLevel(&can->handle, FDCAN_RX_FIFO0) != 0)
        fifo = FDCAN_RX_FIFO0;
    else if (HAL_FDCAN_GetRxFifoFillLevel(&can->handle, FDCAN_RX_FIFO1) != 0)
        fifo = FDCAN_RX_FIFO1;
    else
        return false;

    bool success = false;
    FDCAN_RxHeaderTypeDef rx_header = {0};

    success = HAL_FDCAN_GetRxMessage(&can->handle, fifo, &rx_header, frame->payload) == HAL_OK;
    if (success) {
        frame->id = rx_header.Identifier;
        frame->len = (uint32_t) rx_header.DataLength >> 16U;
        if (rx_header.IdType == FDCAN_EXTENDED_ID)
            frame->type = ext_id;
        else if (rx_header.IdType == FDCAN_STANDARD_ID)
            frame->type = std_id;

        frame->rtr = rx_header.RxFrameType == FDCAN_REMOTE_FRAME ? true : false;
    }
    return success;
}

static bool rcan_set_filter(rcan *can) {

    FDCAN_FilterTypeDef sFilterConfig = {0};

    if (can->filter.is_extended)
        sFilterConfig.IdType = FDCAN_EXTENDED_ID;
    else
        sFilterConfig.IdType = FDCAN_STANDARD_ID;


    sFilterConfig.FilterIndex = 0;
    sFilterConfig.FilterType = FDCAN_FILTER_MASK;
    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    sFilterConfig.FilterID1 = can->filter.mask_filter.id;
    sFilterConfig.FilterID2 = can->filter.mask_filter.mask;

    if (HAL_FDCAN_ConfigFilter(&can->handle, &sFilterConfig) != HAL_OK)
        return false;

    if (HAL_FDCAN_ConfigGlobalFilter(&can->handle, FDCAN_REJECT, FDCAN_REJECT,
                                     FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE) != HAL_OK)
        return false;

    return true;
}


static bool rcan_set_timing(rcan *can, uint32_t bitrate) {

    //TODO check source of system tick for can !!! if not PCLK1 set error or some other sheet
    //TODO add  uint32_t clock = HAL_RCC_GetPCLK1Freq();
    uint32_t clock = SystemCoreClock;
    //TODO add Divider + convert DIV to value real
    if (!rcan_calculate_timing(clock, bitrate, &can->timing))
        return false;

    can->handle.Init.NominalPrescaler = can->timing.bit_rate_prescaler;
    can->handle.Init.NominalSyncJumpWidth = can->timing.max_resynchronization_jump_width;
    can->handle.Init.NominalTimeSeg1 = can->timing.bit_segment_1;
    can->handle.Init.NominalTimeSeg2 = can->timing.bit_segment_2;
    return true;
}


static bool rcan_set_data_timing(rcan *can, uint32_t data_bitrate) {
    // TODO in future add support can FD check recomended CAN open speeds
    can->handle.Init.DataPrescaler = 1;
    can->handle.Init.DataSyncJumpWidth = 1;
    can->handle.Init.DataTimeSeg1 = 1;
    can->handle.Init.DataTimeSeg2 = 1;
    return true;
}

static bool rcan_make_can_tx_header(rcan_frame *frame, FDCAN_TxHeaderTypeDef *tx_header) {

    if (frame->type == std_id) {

        tx_header->IdType = FDCAN_STANDARD_ID;

        if (frame->id > RCAN_STD_ID_MAX)
            return false;

    } else if (frame->type == ext_id) {
        tx_header->IdType = FDCAN_EXTENDED_ID;
    } else {
        return false;
    }

    if (!frame->rtr) {
        tx_header->TxFrameType = FDCAN_DATA_FRAME;
        tx_header->DataLength = frame->len << 16U;
    } else {
        tx_header->TxFrameType = FDCAN_REMOTE_FRAME;
        tx_header->DataLength = 0 << 16U;
    }

    tx_header->ErrorStateIndicator = FDCAN_ESI_PASSIVE;
    tx_header->BitRateSwitch = FDCAN_BRS_OFF;
    tx_header->FDFormat = FDCAN_CLASSIC_CAN;
    tx_header->Identifier = frame->id;
    return true;
}

void rcan_view_frame(rcan_frame *frame) {

    if (frame == NULL)
        return;

    if (frame->rtr) {
        printf("ID : %8lx RTR ", frame->id);
        return;
    }

    printf("ID : %8lx | %s | LEN : %2d | DATA : ", frame->id, frame->type == std_id ? "STD" : "EXT", frame->len);
    for (uint8_t i = 0; i < frame->len; i++) {
        printf("%02x ", frame->payload[i]);
    }
    printf("\n");
}

#endif // endif rcan STM32G474xx


#if  !defined(STM32G474xx)

#if defined(RCAN_UNIX)

static bool is_socket_can_iface(uint32_t channel);

static bool is_vcan_iface(uint32_t channel);

static bool translate_socket_can_name(uint32_t channel, struct ifreq *ifr);

static bool socket_can_start(rcan *can, uint32_t channel, uint32_t bitrate);

static bool socet_can_read(rcan *can, rcan_frame *frame);

static bool socet_can_write(rcan *can, rcan_frame *frame);

#endif

#if defined(RCAN_WINDOWS) || defined (RCAN_MACOS) || defined (RCAN_UNIX)

static bool is_pcan_iface(uint32_t channel);

static bool is_correct_bitrate_for_pcan(uint32_t bitrate);

static bool pcan_start(rcan *can, uint32_t channel, uint32_t bitrate);

static bool pcan_read(rcan *can, rcan_frame *frame);

static bool pcan_write(rcan *can, rcan_frame *frame);

#endif

bool rcan_filter_preconfiguration(rcan *can, uint32_t *accepted_ids, uint32_t size) {
    //TODO make software support filter on macOS and api compactable on linux/win

    if (can == NULL || accepted_ids == NULL || size == 0)
        return false;

    can->use_filter = true;
    return rcan_filter_calculate(accepted_ids, size, &can->filter);

}

bool rcan_start(rcan *can, uint32_t channel, uint32_t bitrate) {


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

void rcan_view_frame(rcan_frame *frame) {

    if (frame == NULL)
        return;

    if (frame->rtr) {
        printf("ID : %8x | %s | RTR \n", frame->id, frame->type == std_id ? "STD" : "EXT");
        return;
    }

    printf("ID : %8x | %s | LEN : %2d | DATA : ", frame->id, frame->type == std_id ? "STD" : "EXT", frame->len);
    for (uint8_t i = 0; i < frame->len; i++) {
        printf("%02x ", frame->payload[i]);
    }
    printf("\n");
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
    else if (status != PCAN_ERROR_OK || message.MSGTYPE & PCAN_MESSAGE_STATUS)
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

#endif // #ifndef (STM32....)
