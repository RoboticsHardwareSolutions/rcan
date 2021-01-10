if (MSVC OR MSYS OR MINGW)
    add_definitions(-m64 -Wa,-mbig-obj)
    set(RCAN_DIRECTORIES rcan/thirdparty/PCAN-Basic-windows)
    link_directories(${RCAN_DIRECTORIES})
    set(PCBUSB_LIBRARIES PCANBasic.lib -static gcc stdc++ winpthread -dynamic)
    file(COPY ${PROJECT_SOURCE_DIR}/thirdparty/PCAN-Basic-windows/PCANBasic.dll DESTINATION ${PROJECT_BINARY_DIR})
    add_definitions(-DRCAN_WINDOWS)
    set(LIBRARIES "${PCBUSB_LIBRARIES}")
endif ()

if (APPLE)
    set(RCAN_DIRECTORIES rcan/thirdparty/PCAN-Basic-mac)
    find_library(PCBUSB_LIBRARIES NAMES pcbusb)
    add_definitions(-DRCAN_MACOS)
    set(LIBRARIES "${PCBUSB_LIBRARIES}")
endif ()

if (UNIX AND NOT APPLE)
    set(RCAN_DIRECTORIES rcan/thirdparty/PCAN-Basic-linux)
    link_directories(thirdparty/PCAN-Basic-linux)
    set(PCBUSB_LIBRARIES pcanbasic)
    add_definitions(-DRCAN_UNIX)
    set(LIBRARIES "${PCBUSB_LIBRARIES}")
    set(LIBRARIES ${LIBRARIES} socketcan)
endif ()

set(RCAN_DIRECTORIES ${RCAN_DIRECTORIES} rcan)
set(RCAN_EXECUTABLE_FILES rcan/rcan.c rcan/rcan_filter.c rcan/rcan_timing.c)




