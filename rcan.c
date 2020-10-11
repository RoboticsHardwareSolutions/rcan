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


void rcan_stop(rcan *can) {
    HAL_Delay(1);
    HAL_FDCAN_AbortTxRequest(&can->handle, FDCAN_TX_BUFFER0 | FDCAN_TX_BUFFER1 | FDCAN_TX_BUFFER2);
    HAL_FDCAN_Stop(&can->handle);
    HAL_FDCAN_DeInit(&can->handle);
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
            frame->type = ext;
        else if (rx_header.IdType == FDCAN_STANDARD_ID)
            frame->type = std;

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

    if (frame->type == std) {

        tx_header->IdType = FDCAN_STANDARD_ID;

        if (frame->id > RCAN_STD_ID_MAX)
            return false;

    } else if (frame->type == ext) {
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

    printf("ID : %8lx | %s | LEN : %2d | DATA : ", frame->id, frame->type == std ? "STD" : "EXT", frame->len);
    for (uint8_t i = 0; i < frame->len; i++) {
        printf("%02x ", frame->payload[i]);
    }
    printf("\n");
}
#endif


#if defined(WINDOWS) || defined(UNIX) || defined (APPLE)


bool rcan_filter_preconfiguration(rcan *can, uint32_t *accepted_ids, uint32_t size) {
    //TODO make software support filter on macOS and api compactable on linux/win

    if (can == NULL || accepted_ids == NULL || size == 0)
        return false;

    can->use_filter = true;
    return rcan_filter_calculate(accepted_ids, size, &can->filter);

}

bool rcan_start(rcan *can, uint32_t channel, uint32_t bitrate) {

    if (can == NULL || channel == 0 || bitrate == 0)
        return false;

    TPCANStatus status;

    status = CAN_Initialize(channel, bitrate, 0, 0, 0);

    if (status != PCAN_ERROR_OK)
        return false;

    status = CAN_GetValue(channel, PCAN_RECEIVE_EVENT, &can->fd, sizeof(int));
    if (status != PCAN_ERROR_OK)
        return false;

    can->channel = channel;
    can->opened = true;
    return true;
}

bool rcan_is_ok(rcan *can) {

    if (!can->opened || can == NULL)
        return false;

    if (CAN_GetValue(can->channel, PCAN_RECEIVE_EVENT, &can->fd, sizeof(int)) != PCAN_ERROR_OK)
        return false;

    return true;
}

void rcan_stop(rcan *can) {

    if (can == NULL)
        return;

    CAN_Uninitialize(can->channel);
    can->opened = false;
}

bool rcan_send(rcan *can, rcan_frame *frame) {

    if (can == NULL ||
        frame == NULL ||
        frame->type == nonframe ||
        frame->len > RCAN_MAX_FRAME_PAYLOAD_SIZE)
        return false;


    TPCANMsg message;

    if (frame->rtr) {

        message.MSGTYPE = PCAN_MESSAGE_RTR;

    } else if (frame->type == ext) {

        message.MSGTYPE = PCAN_MESSAGE_EXTENDED;

    } else if (frame->type == std) {

        message.MSGTYPE = PCAN_MESSAGE_STANDARD;
        if (frame->id > RCAN_STD_ID_MAX)
            return false;

    } else {

        return false;
    }

    message.ID = frame->id;
    message.LEN = frame->len;
    memcpy(message.DATA, frame->payload, frame->len);
    return CAN_Write(can->channel, &message) == PCAN_ERROR_OK ? true : false;
}

bool rcan_receive(rcan *can, rcan_frame *frame) {

    if (can == NULL || frame == NULL)
        return false;

    TPCANMsg message;
    TPCANStatus status;

    status = CAN_Read(can->channel, &message, NULL);

    if (status == PCAN_ERROR_QRCVEMPTY)
        return false;
    else if (status != PCAN_ERROR_OK)
        return false;


    frame->len = message.LEN;
    frame->id = message.ID;

    if (message.MSGTYPE == PCAN_MESSAGE_EXTENDED)
        frame->type = ext;
    else if (message.MSGTYPE == PCAN_MESSAGE_STANDARD)
        frame->type = std;
    else if (message.MSGTYPE == PCAN_MESSAGE_RTR)
        frame->rtr = true; // TODO get real rtr frame and look payload size


    memcpy(frame->payload, &message.DATA, message.LEN);
    return true;

}

void rcan_view_frame(rcan_frame *frame) {

    if (frame == NULL)
        return;

    if (frame->rtr) {
        printf("ID : %8x RTR ", frame->id);
        return;
    }

    printf("ID : %8x | %s | LEN : %2d | DATA : ", frame->id, frame->type == std ? "STD" : "EXT", frame->len);
    for (uint8_t i = 0; i < frame->len; i++) {
        printf("%02x ", frame->payload[i]);
    }
    printf("\n");
}

#endif


