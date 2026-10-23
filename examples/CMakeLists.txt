if (NOT ${CMAKE_SYSTEM_NAME} MATCHES Generic)
    add_executable(rcan_example_pcan_echo pcan_echo/main.c)
    target_link_libraries(rcan_example_pcan_echo rcan)

    add_executable(rcan_example_virtual_bus virtual_bus/main.c)
    target_link_libraries(rcan_example_virtual_bus rcan)
endif ()
