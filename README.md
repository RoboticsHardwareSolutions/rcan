## rcan


can lib for stm32 and pcan usb adapter on linux, mac, windows 

example 
```
void start_proto_task(void const *argument) {
    /* USER CODE BEGIN start_proto_task */
    /* Infinite loop */
    rcan can = {0};
    uint32_t id = 0x112;
    //rcan_filter_preconfiguration(&can, &id, 1);

    if (!rcan_start(&can, FDCAN1_BASE, 1000000)) {
        printf("cannot start can\r\n");
    }


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
        printf("packet sended\r\n");
    }

    for (;;) {

        if (!rcan_is_ok(&can)) {
            led_red_on();
            rcan_stop(&can);
            if (!rcan_start(&can, FDCAN1_BASE, 1000000)) {
                printf("cannot start can\r\n");
            }

            if(!rcan_is_ok(&can)){
                led_green_on();
                rcan_is_ok(&can);
            }
            led_red_off();
        }


        if (!rcan_read(&can, &frame)) {

        }
        frame.id = 0x345;
        if (!rcan_write(&can, &frame)) {

        }

    }

    /* USER CODE END start_proto_task */
}
```