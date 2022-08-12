#ifndef  __BX_CANFD_H_
#define  __BX_CANFD_H_

#if defined(STM32G474xx)

bool bx_canfd_filter_preconfiguration(rcan *can, uint32_t *accepted_ids, uint32_t size);

bool bx_canfd_start(rcan *can, uint32_t channel, uint32_t bitrate);

#endif // defined(STM32G474xx)

#endif
