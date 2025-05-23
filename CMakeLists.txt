cmake_minimum_required(VERSION 3.24)
project(rcan
        VERSION 0.2.0
        LANGUAGES C
        DESCRIPTION
        "cross platform lib with simple C API for work with CAN BUS")

include(rcmake_macros/rcmake_macros.cmake)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#################### MAIN SECTION #######################

set(RCAN_HEADER
        rcan.h
        rcan_timing.h
        rcan_def.h
        rcan_filter.h
        can/bx_canfd.h
        can/bx_can.h
        can/u_can.h
        can/u_can_urls.h
)

set(RCAN_SOURCES
        rcan.c
        rcan_filter.c
        rcan_timing.c
        can/bx_can.c
        can/bx_canfd.c
        can/u_can.c
        can/bx_can.c
        can/bx_canfd.h)

set(RCAN_FILES
        ${RCAN_HEADER}
        ${RCAN_SOURCES}
)

if (MSVC OR MSYS OR MINGW)
    set(RCAN_THIRDPARTY ${CMAKE_CURRENT_LIST_DIR}/thirdparty/PCAN-Basic-windows)
    link_directories(${RCAN_DIRECTORIES})
    find_package(nanomsg CONFIG REQUIRED)
    set(PCBUSB_LIBRARIES PCANBasic.lib -static gcc stdc++ winpthread -dynamic)
    set(PCAN_LIBRARIES PCANBasic.lib)
    file(COPY ${CMAKE_CURRENT_LIST_DIR}/thirdparty/PCAN-Basic-windows/PCANBasic.dll DESTINATION ${PROJECT_BINARY_DIR})
    set(RCAN_LIBRARIES "${PCAN_LIBRARIES}")
    set(RCAN_LIBRARIES ${RCAN_LIBRARIES} nanomsg)
endif ()

if (APPLE)
    set(RCAN_THIRDPARTY ${CMAKE_CURRENT_LIST_DIR}/thirdparty/PCAN-Basic-mac)
    find_library(PCAN_LIBRARIES NAMES pcbusb)
    add_subdirectory(rnode)
    set(RCAN_LIBRARIES ${PCAN_LIBRARIES} rnode)
endif ()

if (UNIX AND NOT APPLE)
    set(RCAN_THIRDPARTY ${CMAKE_CURRENT_LIST_DIR}/thirdparty/PCAN-Basic-linux)
    link_directories(${CMAKE_CURRENT_LIST_DIR}/thirdparty/PCAN-Basic-linux)
    set(PCBUSB_LIBRARIES pcanbasic)
    set(RCAN_LIBRARIES ${PCBUSB_LIBRARIES})
    set(RCAN_LIBRARIES ${RCAN_LIBRARIES} socketcan rnode)
    add_subdirectory(rnode)
endif ()

add_library(${PROJECT_NAME} STATIC ${RCAN_SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${RCAN_LIBRARIES})
target_include_directories(${PROJECT_NAME}
        PUBLIC
        ./
        can/
        ${RCAN_THIRDPARTY})

if (MSVC OR MSYS OR MINGW)
    target_compile_definitions(${PROJECT_NAME} PUBLIC -DRCAN_WINDOWS)
endif ()

if (APPLE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC -DRCAN_MACOS)
endif ()

if (UNIX AND NOT APPLE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC -DRCAN_UNIX)
endif ()


#################### TEST SECTION #######################
set(TEST_SRC_FILES
        test/hardware_unix.c
        test/hardware_macos.c
        test/virtual_bus.c
        test/virtual_can.c
        test/test.c)

set(TEST_HEADER_FILES
        test/hardware_unix.h
        test/virtual_bus.h
        test/virtual_can.h
        test/test.h)

if (TEST_TARGET STREQUAL "virtual_can")
    message("Compile test for virtual_can")
    add_definitions(-DRCAN_UNIX_TEST_UNIX_VIRTUAL_CAN=1)
elseif (TEST_TARGET STREQUAL "virtual_bus")
    message("Compile test for virtual_bus")
    add_definitions(-DRCAN_VIRTUAL_BUS_TEST)
elseif (TEST_TARGET STREQUAL "hardware_macos")
    message("Compile test for hardware_macos")
    add_definitions(-DRCAN_HARDWARE_MACOS_TEST=1)
elseif (TEST_TARGET STREQUAL "hardware_unix")
    message("Compile test for hardware_unix")
    add_definitions(-DRCAN_HARDWARE_UNIX_TEST=1)
endif ()

# Test runner executable testing the static library
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES Generic)
    add_executable(${PROJECT_NAME}_test
            ${TEST_FRAMEWORK_SRC}
            ${TEST_SRC_FILES})
    target_include_directories(${PROJECT_NAME}_test
            PUBLIC ./
            PRIVATE can/
            PRIVATE test/
            PRIVATE test/runit/
            PRIVATE ${RCAN_THIRDPARTY})

    target_link_libraries(${PROJECT_NAME}_test
            PUBLIC rcan
            PUBLIC ${RCAN_LIBRARIES} runit)

    enable_testing()
    add_test(NAME ${PROJECT_NAME}_test COMMAND ./testrcan)
    add_test(NAME ${PROJECT_NAME}_testshared COMMAND ./testrcanshared)
endif ()


#################### FORMAT SECTION #######################

set(FILES_FOR_FORMATTING
        ${TEST_SRC_FILES}
        ${TEST_HEADER_FILES}
        ${RCAN_SOURCES}
        ${RCAN_HEADER}
)

include(rcmake_macros/clang.cmake)
