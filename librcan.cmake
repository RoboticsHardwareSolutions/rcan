project(rcan_project)

if (MSVC OR MSYS OR MINGW)
    add_definitions(-m64 -Wa,-mbig-obj)
    include_directories(rcan/thirdparty/PCAN-Basic-windows)
    link_directories(rcan/thirdparty/PCAN-Basic-windows)
    set(PCBUSB_LIBRARIES PCANBasic.lib -static gcc stdc++ winpthread -dynamic)
    file(COPY ${PROJECT_SOURCE_DIR}/thirdparty/PCAN-Basic-windows/PCANBasic.dll DESTINATION ${PROJECT_BINARY_DIR})
    add_definitions(-DWINDOWS)
    set(LIBRARIES "${PCBUSB_LIBRARIES}")
endif ()

if (APPLE)
    include_directories(rcan/thirdparty/PCAN-Basic-mac)
    find_library(PCBUSB_LIBRARIES NAMES pcbusb)
    add_definitions(-DAPPLE)
    set(LIBRARIES "${PCBUSB_LIBRARIES}")
endif ()

if (UNIX AND NOT APPLE)
    include_directories(rcan/thirdparty/PCAN-Basic-linux)
    link_directories(thirdparty/PCAN-Basic-linux)
    set(PCBUSB_LIBRARIES pcanbasic)
    add_definitions(-DUNIX)
    set(LIBRARIES "${PCBUSB_LIBRARIES}")
endif ()




