if (MSVC OR MSYS OR MINGW)
#    include(Findlibzmq.cmake)
    set(RCAN_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/thirdparty/PCAN-Basic-windows)
    link_directories(${RCAN_DIRECTORIES})
    #set(PCBUSB_LIBRARIES PCANBasic.lib -static gcc stdc++ winpthread -dynamic)
    set(PCAN_LIBRARIES PCANBasic.lib)

    file(COPY ${CMAKE_CURRENT_LIST_DIR}/thirdparty/PCAN-Basic-windows/PCANBasic.dll DESTINATION ${PROJECT_BINARY_DIR})
    add_definitions(-DRCAN_WINDOWS)
    set(RCAN_LIBRARIES "${PCAN_LIBRARIES}")
    set(RCAN_LIBRARIES ${RCAN_LIBRARIES} ${PC_LIBZMQ_LINK_LIBRARIES})
endif ()

if (APPLE)
#    include(Findlibzmq.cmake)
    set(RCAN_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/thirdparty/PCAN-Basic-mac)
    find_library(PCAN_LIBRARIES NAMES pcbusb)
    add_definitions(-DRCAN_MACOS)
    set(RCAN_LIBRARIES "${PCAN_LIBRARIES}")
    set(RCAN_LIBRARIES ${RCAN_LIBRARIES} ${PC_LIBZMQ_LINK_LIBRARIES})

endif ()


if (UNIX AND NOT APPLE)
#    include(Findlibzmq.cmake)
    set(RCAN_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/thirdparty/PCAN-Basic-linux)
    link_directories(${CMAKE_CURRENT_LIST_DIR}/thirdparty/PCAN-Basic-linux)
    set(PCBUSB_LIBRARIES pcanbasic)
    add_definitions(-DRCAN_UNIX)
    set(RCAN_LIBRARIES ${PCBUSB_LIBRARIES})
    set(RCAN_LIBRARIES ${RCAN_LIBRARIES} socketcan)
    set(RCAN_LIBRARIES ${RCAN_LIBRARIES} ${PC_LIBZMQ_LINK_LIBRARIES})
endif ()

set(RCAN_DIRECTORIES ${RCAN_DIRECTORIES}  ${CMAKE_CURRENT_LIST_DIR})
set(RCAN_DIRECTORIES ${RCAN_DIRECTORIES}  ${CMAKE_CURRENT_LIST_DIR}/can )
set(RCAN_DIRECTORIES ${RCAN_DIRECTORIES}  ${PC_LIBZMQ_LIBRARY_DIRS})
set(RCAN_EXECUTABLE_FILES
        ${CMAKE_CURRENT_LIST_DIR}/rcan.c
        ${CMAKE_CURRENT_LIST_DIR}/rcan_filter.c
        ${CMAKE_CURRENT_LIST_DIR}/rcan_timing.c
        ${CMAKE_CURRENT_LIST_DIR}/can/bx_can.c
        ${CMAKE_CURRENT_LIST_DIR}/can/bx_canfd.c
        ${CMAKE_CURRENT_LIST_DIR}/can/u_can.c
        ${CMAKE_CURRENT_LIST_DIR}/can/zmq_can.c)





