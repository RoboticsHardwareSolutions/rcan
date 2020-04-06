#include "rcan.h"
#include "stdio.h"

#define RCAN_EXT_ID_MAX                      0x1FFFFFFFU
#define RCAN_STD_ID_MAX                      0x000007FFU

#define RCAN_MAX_FRAME_PAYLOAD_SIZE                    8

bool rcan_filter_preconfiguration(rcan *can, const uint32_t *source_sequence, uint32_t size) {

    if (can == NULL || source_sequence == NULL || size == 0)
        return false;

    can->use_filter = true;
    return rcan_filter_calculate(source_sequence, size, &can->filter);

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

    uint32_t clock = SystemCoreClock;
    uint32_t div = can->handle.Init.ClockDivider + 1;
    if (!rcan_calculate_timing(clock / div, bitrate, &can->timing))
        return false;


    can->handle.Init.NominalPrescaler = can->timing.bit_rate_prescaler;
    can->handle.Init.NominalSyncJumpWidth = can->timing.max_resynchronization_jump_width;
    can->handle.Init.NominalTimeSeg1 = can->timing.bit_segment_1;
    can->handle.Init.NominalTimeSeg2 = can->timing.bit_segment_2;

    can->handle.Init.DataPrescaler = 1;
    can->handle.Init.DataSyncJumpWidth = 1;
    can->handle.Init.DataTimeSeg1 = 1;
    can->handle.Init.DataTimeSeg2 = 1;
    can->handle.Init.StdFiltersNbr = 1;
    can->handle.Init.ExtFiltersNbr = 1;
    can->handle.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

    if (HAL_FDCAN_Init(&can->handle) != HAL_OK)
        return false;

    if (can->use_filter) {
        // TODO separate and add case when filter not configured;

        FDCAN_FilterTypeDef sFilterConfig = {0};

        if (can->filter.is_extended) {
            sFilterConfig.IdType = FDCAN_EXTENDED_ID;
        } else {
            sFilterConfig.IdType = FDCAN_STANDARD_ID;
        }

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
    if( __HAL_FDCAN_GET_FLAG(&can->handle, FDCAN_FLAG_RX_FIFO0_MESSAGE_LOST) == FDCAN_FLAG_RX_FIFO0_MESSAGE_LOST )
        return false;

    return true;
}


void rcan_stop(rcan *can) {
    HAL_FDCAN_DeInit(&can->handle);
}


bool rcan_write(rcan *can, rcan_frame *frame) {

    if (can == NULL || frame == NULL || frame->payload == NULL || frame->len > RCAN_MAX_FRAME_PAYLOAD_SIZE)
        return false;

    if (HAL_FDCAN_GetTxFifoFreeLevel(&can->handle) == 0)
        return false;

    FDCAN_TxHeaderTypeDef tx_header = {0};

    if (frame->type == std) {
        tx_header.IdType = FDCAN_STANDARD_ID;
        if (frame->id > RCAN_STD_ID_MAX) {
            return false;
        }
    } else if (frame->type == ext) {
        tx_header.IdType = FDCAN_EXTENDED_ID;
    } else {
        return false;
    }

    tx_header.TxFrameType = FDCAN_DATA_FRAME;
    tx_header.DataLength = frame->len << 16U;
    tx_header.ErrorStateIndicator = FDCAN_ESI_PASSIVE;
    tx_header.BitRateSwitch = FDCAN_BRS_OFF;
    tx_header.FDFormat = FDCAN_CLASSIC_CAN;
    tx_header.Identifier = frame->id;

    return HAL_FDCAN_AddMessageToTxFifoQ(&can->handle, &tx_header, frame->payload) == HAL_OK;
}


bool rcan_read(rcan *can, rcan_frame *frame) {

    if (can == NULL || frame == NULL || frame->payload == NULL)
        return false;

    bool success = false;
    uint32_t fifo = 0;
    FDCAN_RxHeaderTypeDef rx_header = {0};


    if (HAL_FDCAN_GetRxFifoFillLevel(&can->handle, FDCAN_RX_FIFO1) != 0)
        fifo = FDCAN_RX_FIFO1;
    else if (HAL_FDCAN_GetRxFifoFillLevel(&can->handle, FDCAN_RX_FIFO0) != 0)
        fifo = FDCAN_RX_FIFO0;
    else
        return false;

    success = HAL_FDCAN_GetRxMessage(&can->handle, fifo, &rx_header, frame->payload) == HAL_OK;
    if (success) {
        frame->id = rx_header.Identifier;
        frame->len = rx_header.DataLength >> 16U;
        if (rx_header.IdType == FDCAN_EXTENDED_ID)
            frame->type = ext;
        else if (rx_header.IdType == FDCAN_STANDARD_ID)
            frame->type = std;
    }
    return success;
}

void rcan_view_frame(rcan_frame *frame) {

    if (frame == NULL || frame->payload == NULL)
        return;

    printf("ID : %8x | %s | LEN : %2d | DATA : ", frame->id, frame->type == std ? "STD" : "EXT", frame->len);
    for (uint8_t i = 0; i < frame->len; i++) {
        printf("%02x ", frame->payload[i]);
    }
    printf("\n");
}





