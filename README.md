## rcan

can & can fd  lib for stm32 unix apple windows

for using in CMake project add in CMakeLists.txt
```
include(rcan/librcan.cmake)
include_directories(rcan)

add_executable(rcan/rcan_filter.c rcan/rcan_timing.c rcan/rcan.c )
target_link_libraries(....  ${LIBRARIES}) 
```


example
 
```
    rcan can = {0};
    uint32_t id = 0x112;
    rcan_filter_preconfiguration(&can, &id, 1);

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

    if (rcan_send(&can, &frame)) {
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


        if (!rcan_receive(&can, &frame)) {

        }
        frame.id = 0x345;
        if (!rcan_send(&can, &frame)) {

        }
        
    }

```