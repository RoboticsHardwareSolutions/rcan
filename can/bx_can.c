
#if defined(STM32F767xx) || defined(STM32F765xx) || defined(STM32F072xB) || defined(STM32F091xC) || defined(STM32F103xB)

static bool rcan_set_filter(rcan *can);

static bool rcan_set_timing(rcan *can, uint32_t bitrate);

static bool st_prop_bs1_and_bs2_convert(rcan_timing *source, rcan_timing *result);

static bool rcan_make_can_tx_header(rcan_frame *frame, CAN_TxHeaderTypeDef *tx_header);

bool rcan_filter_preconfiguration(rcan *can, uint32_t *accepted_ids, uint32_t size) {

    if (can == NULL || accepted_ids == NULL || size == 0)
        return false;

    can->use_filter = true;
    return rcan_filter_calculate(accepted_ids, size, &can->filter);

}


bool rcan_start(rcan *can, uint32_t channel, uint32_t bitrate) {

    if (can == NULL || channel == 0 || bitrate == 0)
        return false;

    can->handle.Instance = (CAN_TypeDef *) channel;
    can->handle.Init.Mode = CAN_MODE_NORMAL;
    can->handle.Init.TimeTriggeredMode = DISABLE;
    can->handle.Init.AutoBusOff = DISABLE;
    can->handle.Init.AutoWakeUp = DISABLE;
    can->handle.Init.AutoRetransmission = ENABLE;
    can->handle.Init.ReceiveFifoLocked = DISABLE;
    can->handle.Init.TransmitFifoPriority = DISABLE;

    if (!rcan_set_timing(can, bitrate))
        return false;

    if (HAL_CAN_Init(&can->handle) != HAL_OK)
        return false;

    if (!rcan_set_filter(can))
        return false;

    return HAL_CAN_Start(&can->handle) == HAL_OK;

}

bool rcan_is_ok(rcan *can) {

    if (HAL_CAN_GetError(&can->handle) != HAL_CAN_ERROR_NONE) {
        printf("HAL_CAN_ERROR_NONE\n");
        return false;
    }
    if (HAL_CAN_GetState(&can->handle) == HAL_CAN_STATE_RESET) {
        printf("HAL_CAN_STATE_RESET\n");
        return false;
    }
    if (HAL_CAN_GetState(&can->handle) == HAL_CAN_STATE_ERROR) {
        printf("HAL_CAN_STATE_ERROR\n");
        return false;
    }
    if (__HAL_CAN_GET_FLAG(&can->handle, CAN_FLAG_TERR0)) {
        printf("CAN_FLAG_TERR0\n");
        return false;
    }
    if (__HAL_CAN_GET_FLAG(&can->handle, CAN_FLAG_TERR1)) {
        printf("CAN_FLAG_TERR1\n");
        return false;
    }
    if (__HAL_CAN_GET_FLAG(&can->handle, CAN_FLAG_TERR2)) {
        printf("CAN_FLAG_TERR2\n");
        return false;
    }
    if (__HAL_CAN_GET_FLAG(&can->handle, CAN_FLAG_FOV0)) {
        printf("CAN_FLAG_FOV0\n");
        return false;
    }
    if (__HAL_CAN_GET_FLAG(&can->handle, CAN_FLAG_FOV1)) {
        printf("CAN_FLAG_FOV1\n");
        return false;
    }
    // (__HAL_CAN_CLEAR_FLAG(&can->handle, CAN_FLAG_RX_FIFO0_MESSAGE_LOST);
    // TODO check any variants of error !!!!

    return true;
}


bool rcan_stop(rcan *can) {

    if (HAL_OK != HAL_CAN_AbortTxRequest(
            &can->handle,
            CAN_TX_MAILBOX0 | CAN_TX_MAILBOX1 | CAN_TX_MAILBOX2)) {
        return false;
    }

    if (HAL_OK != HAL_CAN_Stop(&can->handle)) {
        return false;
    }
    if (HAL_OK != HAL_CAN_DeInit(&can->handle)) {
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

    CAN_TxHeaderTypeDef tx_header = {0};

    if (!rcan_make_can_tx_header(frame, &tx_header)) {
        return false;
    }

    if (HAL_CAN_GetTxMailboxesFreeLevel(&can->handle) == 0) {
        return false;
    }

    uint32_t mbox;

    if (HAL_CAN_AddTxMessage(&can->handle, &tx_header, frame->payload, &mbox) != HAL_OK)
        return false;

    return true;
}

bool rcan_receive(rcan *can, rcan_frame *frame) {

    if (can == NULL || frame == NULL || frame->payload == NULL)
        return false;

    uint32_t fifo = 0;
    bool success = false;
    CAN_RxHeaderTypeDef rx_header = {0};

    uint32_t nfifo0 = HAL_CAN_GetRxFifoFillLevel(&can->handle, CAN_RX_FIFO0), nfifo1 = HAL_CAN_GetRxFifoFillLevel(
            &can->handle, CAN_RX_FIFO1);
    if (nfifo0 < nfifo1)
        fifo = CAN_RX_FIFO1;
    else if (nfifo0 > nfifo1 || (nfifo1 == nfifo0 && nfifo1 != 0))
        fifo = CAN_RX_FIFO0;
    else
        return false;

    success = HAL_CAN_GetRxMessage(&can->handle, fifo, &rx_header, frame->payload) == HAL_OK;

    if (success) {

        if (rx_header.IDE == CAN_ID_STD) {

            frame->type = std_id;
            frame->id = rx_header.StdId;

        } else if (rx_header.IDE == CAN_ID_EXT) {

            frame->type = ext_id;
            frame->id = rx_header.ExtId;
        }

        frame->len = rx_header.DLC;
        frame->rtr = rx_header.RTR == CAN_RTR_REMOTE ? true : false;
    }
    return success;
}

static bool rcan_set_filter(rcan *can) {

    CAN_FilterTypeDef sFilterConfig = {0};

//    sFilterConfig.FilterIdHigh = ((can->filter.mask_filter.id << 3) >> 16) & 0xffff;
//    sFilterConfig.FilterIdLow = (uint16_t) (can->filter.mask_filter.id << 3) | CAN_ID_EXT;
//
//    sFilterConfig.FilterMaskIdHigh = (can->filter.mask_filter.mask >> 16) & 0xffff;
//    sFilterConfig.FilterMaskIdLow = can->filter.mask_filter.mask & 0xffff;
   //TODO need check filters for all mcu !!!

    if (can->handle.Instance == CAN2) {
        sFilterConfig.FilterBank = 1;
    }
    if (can->handle.Instance == CAN1) {
        sFilterConfig.FilterBank = 0;
    }
    if (can->handle.Instance == CAN3) {
        sFilterConfig.FilterBank = 2;
    }

    sFilterConfig.FilterIdHigh = (uint16_t) can->filter.mask_filter.id >> 13;
    sFilterConfig.FilterIdLow = (uint16_t) (can->filter.mask_filter.id << 3) | CAN_ID_EXT;

    sFilterConfig.FilterMaskIdHigh = (uint16_t) can->filter.mask_filter.mask >> 13;
    sFilterConfig.FilterMaskIdLow = (uint16_t) (can->filter.mask_filter.mask << 3) | CAN_ID_EXT;

    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;


    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0 | CAN_RX_FIFO1;

    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 0;

    if (HAL_CAN_ConfigFilter(&can->handle, &sFilterConfig) != HAL_OK)
        return false;

    return true;
}


static bool rcan_set_timing(rcan *can, uint32_t bitrate) {

    uint32_t clock = HAL_RCC_GetPCLK1Freq();
    if (!rcan_calculate_timing(clock, bitrate, &can->timing))
        return false;

    rcan_timing st_timing = {0};
    if (!st_prop_bs1_and_bs2_convert(&can->timing, &st_timing))
        return false;


    can->handle.Init.Prescaler = st_timing.bit_rate_prescaler;
    can->handle.Init.SyncJumpWidth = st_timing.max_resynchronization_jump_width;
    can->handle.Init.TimeSeg1 = st_timing.bit_segment_1;
    can->handle.Init.TimeSeg2 = st_timing.bit_segment_2;

    return true;
}


static bool rcan_make_can_tx_header(rcan_frame *frame, CAN_TxHeaderTypeDef *tx_header) {

    if (frame->type == std_id) {

        if (frame->id > RCAN_STD_ID_MAX)
            return false;

        tx_header->IDE = CAN_ID_STD;
        tx_header->StdId = frame->id;

    } else if (frame->type == ext_id) {

        tx_header->IDE = CAN_ID_EXT;
        tx_header->ExtId = frame->id;

    } else {

        return false;
    }


    if (!frame->rtr) {

        tx_header->RTR = CAN_RTR_DATA;
        tx_header->DLC = frame->len;
    } else {

        tx_header->RTR = CAN_RTR_REMOTE;
        tx_header->DLC = 0;
    }

    tx_header->TransmitGlobalTime = DISABLE;
    return true;
}


static bool st_prop_bs1_and_bs2_convert(rcan_timing *source, rcan_timing *result) {

    result->bit_rate_prescaler = source->bit_rate_prescaler;

    switch (source->bit_segment_2) {
        case 1:
            result->bit_segment_2 = CAN_BS2_1TQ;
            break;
        case 2:
            result->bit_segment_2 = CAN_BS2_2TQ;
            break;
        case 3:
            result->bit_segment_2 = CAN_BS2_3TQ;
            break;
        case 4:
            result->bit_segment_2 = CAN_BS2_4TQ;
            break;
        case 5:
            result->bit_segment_2 = CAN_BS2_5TQ;
            break;
        case 6:
            result->bit_segment_2 = CAN_BS2_6TQ;
            break;
        case 7:
            result->bit_segment_2 = CAN_BS2_7TQ;
            break;
        case 8:
            result->bit_segment_2 = CAN_BS2_8TQ;
            break;
        default:
            return false;
    }

    switch (source->bit_segment_1) {
        case 1:
            result->bit_segment_1 = CAN_BS1_1TQ;
            break;
        case 2:
            result->bit_segment_1 = CAN_BS1_2TQ;
            break;
        case 3:
            result->bit_segment_1 = CAN_BS1_3TQ;
            break;
        case 4:
            result->bit_segment_1 = CAN_BS1_4TQ;
            break;
        case 5:
            result->bit_segment_1 = CAN_BS1_5TQ;
            break;
        case 6:
            result->bit_segment_1 = CAN_BS1_6TQ;
            break;
        case 7:
            result->bit_segment_1 = CAN_BS1_7TQ;
            break;
        case 8:
            result->bit_segment_1 = CAN_BS1_8TQ;
            break;
        case 9:
            result->bit_segment_1 = CAN_BS1_9TQ;
            break;
        case 10:
            result->bit_segment_1 = CAN_BS1_10TQ;
            break;
        case 11:
            result->bit_segment_1 = CAN_BS1_11TQ;
            break;
        case 12:
            result->bit_segment_1 = CAN_BS1_12TQ;
            break;
        case 13:
            result->bit_segment_1 = CAN_BS1_13TQ;
            break;
        case 14:
            result->bit_segment_1 = CAN_BS1_14TQ;
            break;
        case 15:
            result->bit_segment_1 = CAN_BS1_15TQ;
            break;
        case 16:
            result->bit_segment_1 = CAN_BS1_16TQ;
            break;
        default:
            return false;
    }

    switch (source->max_resynchronization_jump_width) {
        case 1:
            result->max_resynchronization_jump_width = CAN_SJW_1TQ;
            break;
        case 2:
            result->max_resynchronization_jump_width = CAN_SJW_2TQ;
            break;
        case 3:
            result->max_resynchronization_jump_width = CAN_SJW_3TQ;
            break;
        case 4:
            result->max_resynchronization_jump_width = CAN_SJW_4TQ;
            break;
        default:
            return false;

    }
    return true;
}

#endif // defined(STM32F767xx) || defined(STM32F765xx) || defined(STM32F072xB) || defined(STM32F091xC) || defined(STM32F103xB)
