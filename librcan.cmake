message("Hello from rcan!")
message("Full path to module: ${CMAKE_CURRENT_LIST_FILE}")
message("Module located in directory: ${CMAKE_CURRENT_LIST_DIR}")

if (MSVC OR MSYS OR MINGW)
    set(RCAN_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/thirdparty/PCAN-Basic-windows)
    link_directories(${RCAN_DIRECTORIES})
    #set(PCBUSB_LIBRARIES PCANBasic.lib -static gcc stdc++ winpthread -dynamic)
    set(PCAN_LIBRARIES PCANBasic.lib)

    file(COPY ${CMAKE_CURRENT_LIST_DIR}/thirdparty/PCAN-Basic-windows/PCANBasic.dll DESTINATION ${PROJECT_BINARY_DIR})
    add_definitions(-DRCAN_WINDOWS)
    set(RCAN_LIBRARIES "${PCAN_LIBRARIES}")
endif ()

if (APPLE)
    set(RCAN_DIRECTORIES rcan/thirdparty/PCAN-Basic-mac)
    find_library(PCAN_LIBRARIES NAMES pcbusb)
    add_definitions(-DRCAN_MACOS)
    set(RCAN_LIBRARIES "${PCAN_LIBRARIES}")
endif ()


if (UNIX AND NOT APPLE)
    set(RCAN_DIRECTORIES rcan/thirdparty/PCAN-Basic-linux)
    link_directories(thirdparty/PCAN-Basic-linux)
    set(PCBUSB_LIBRARIES pcanbasic)
    add_definitions(-DRCAN_UNIX)
    set(RCAN_LIBRARIES "${PCBUSB_LIBRARIES}")
    set(RCAN_LIBRARIES ${RCAN_LIBRARIES} socketcan)
endif ()

set(RCAN_DIRECTORIES ${RCAN_DIRECTORIES} ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/can)
# set(RCAN_DIRECTORIES ${RCAN_DIRECTORIES}  ${CMAKE_CURRENT_LIST_DIR})
set(RCAN_EXECUTABLE_FILES
        ${CMAKE_CURRENT_LIST_DIR}/rcan.c
        ${CMAKE_CURRENT_LIST_DIR}/rcan_filter.c
        ${CMAKE_CURRENT_LIST_DIR}/rcan_timing.c
        ${CMAKE_CURRENT_LIST_DIR}/can/bx_can.c
        ${CMAKE_CURRENT_LIST_DIR}/can/bx_canfd.c
        ${CMAKE_CURRENT_LIST_DIR}/can/u_can.c)






