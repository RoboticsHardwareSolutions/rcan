## rcan


can & can fd  lib for stm32 

example 

```
    rcan can = {0};
    uint32_t id = 0x112;
    rcan_filter_preconfiguration(&can, &id, 1);
    rcan_start(&can, FDCAN1_BASE, 1000000);

    if (rcan_is_ok(&can)) {
        printf("hello rcan\r\n");
    }

    rcan_frame frame = {0};
    uint8_t load[8] = {0x01, 0x02, 0x03, 0x04, 0x05};
    frame.len = sizeof(load);
    frame.type = std;
    frame.id = 0x123;
    frame.payload = load;

    if (rcan_write(&can, &frame)) {
        printf("yes\r\n");
    }
    /* Infinite loop */
    for (;;) {
        if (rcan_read(&can, &frame)) {
            led_red_on();
            for(uint32_t i = 0; i < 400; i++){
                __NOP();
            }
            led_red_off();
            //rcan_view_frame(&frame);
        }
        if (!rcan_is_ok(&can)) {
            led_red_on();
            while(1){
                __NOP();
            }
            printf("fuck'\r\n");
        }
    }
```