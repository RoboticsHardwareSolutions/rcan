//#include "rcan_bus.h"
//
//
////FDCAN_HandleTypeDef hfdcan1;
//
////void rcan_bus_add(rcan_bus *bus, FDCAN_HandleTypeDef *fdcan, rcan_speed) {
////
////
////}
//
//
////void rcan_bus_config(rcan_bus *bus, rcan_speed speed) {
////
////}
//
////
////bool rcan_iface_init(FDCAN_HandleTypeDef *fdcan, ) {
////
////    fdcan->Instance = FDCAN1;
////    fdcan->Init.ClockDivider = FDCAN_CLOCK_DIV1;
////    fdcan->Init.FrameFormat = FDCAN_FRAME_FD_NO_BRS;
////    fdcan->Init.Mode = FDCAN_MODE_NORMAL;
////
////
////    fdcan->Init.AutoRetransmission = ENABLE;
////    fdcan->Init.TransmitPause = DISABLE;
////    fdcan->Init.ProtocolException = DISABLE;
////
////    fdcan->Init.NominalPrescaler = 1;
////    fdcan->Init.NominalSyncJumpWidth = 1;
////    fdcan->Init.NominalTimeSeg1 = 2;
////    fdcan->Init.NominalTimeSeg2 = 2;
////
////    fdcan->Init.DataPrescaler = 1;
////    fdcan->Init.DataSyncJumpWidth = 1;
////    fdcan->Init.DataTimeSeg1 = 1;
////    fdcan->Init.DataTimeSeg2 = 1;
////    fdcan->Init.StdFiltersNbr = 0;
////    fdcan->Init.ExtFiltersNbr = 0;
////    fdcan->Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
////
////    return HAL_FDCAN_Init(fdcan) == HAL_OK ? true : false;
////}
//
//
//void rcan_fd_filter(void){
//    FDCAN_FilterTypeDef sFilterConfig;
//
//    /* Configure Rx filter */
//    sFilterConfig.IdType = FDCAN_STANDARD_ID;
//    sFilterConfig.FilterIndex = 0;
//    sFilterConfig.FilterType = FDCAN_FILTER_MASK;
//    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
//    sFilterConfig.FilterID1 = 0x321;
//    sFilterConfig.FilterID2 = 0x7FF;
////    if (HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig) != HAL_OK)
////    {
////        Error_Handler();
////    }
////
////    /* Configure global filter:
////       Filter all remote frames with STD and EXT ID
////       Reject non matching frames with STD ID and EXT ID */
////    if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_REJECT, FDCAN_REJECT, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE) != HAL_OK)
////    {
////        Error_Handler();
////    }
//}
//
//
//
//
//
//
//
