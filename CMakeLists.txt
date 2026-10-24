cmake_minimum_required(VERSION 3.24)
project(rcan
        VERSION 1.0.1
        LANGUAGES C
        DESCRIPTION
        "cross platform lib with simple C API for work with CAN BUS")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#################### MAIN SECTION #######################

set(RCAN_HEADER
        rcan.h
        rcan_backend.h
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
)

if (APPLE OR (UNIX AND NOT APPLE))
    if (NOT TARGET rnode)
        include(FetchContent)
        FetchContent_Declare(
                rnode
                GIT_REPOSITORY https://github.com/RoboticsHardwareSolutions/rnode.git
                GIT_TAG main
        )
        FetchContent_MakeAvailable(rnode)
    endif ()
endif ()

if (APPLE)
    # PCBUSB is closed-source (PEAK only ships source for the Linux driver), but the
    # official mac-can/PCBUSB-Library release already ships a prebuilt *static* archive
    # alongside the dylib - link that directly instead of `find_library()`-ing whatever
    # (possibly differently-versioned, possibly unsigned) copy happens to be installed
    # system-wide.
    if (NOT pcbusb_POPULATED)
        include(FetchContent)
        FetchContent_Declare(
                pcbusb
                URL https://github.com/mac-can/PCBUSB-Library/releases/download/v0.13/macOS_Library_for_PCANUSB_v0.13.tar.gz
                URL_HASH SHA256=e06e5d650868b11ecb4c27f03749e61ab43825580c644ec593dd3e7ac7be0dd5
        )
        FetchContent_MakeAvailable(pcbusb)
    endif ()

    set(RCAN_THIRDPARTY ${pcbusb_SOURCE_DIR})
    set(PCAN_LIBRARIES ${pcbusb_SOURCE_DIR}/libPCBUSB.a)
    set(RCAN_LIBRARIES ${PCAN_LIBRARIES} rnode)
endif ()

if (UNIX AND NOT APPLE)
    # PEAK ships libpcanbasic's source as a plain Makefile-based tarball (not a CMake
    # or autotools project), fetched straight from their own site rather than vendored,
    # so the version in use is always visible from the URL/hash below.
    if (NOT pcanbasic_POPULATED)
        include(FetchContent)
        FetchContent_Declare(
                pcanbasic
                URL "https://www.peak-system.com/produktcd/Develop/PC%20interfaces/Linux/PCAN-Basic_API_for_Linux/PCAN-Basic_Linux-5.0.1.8.tar.gz"
                URL_HASH SHA256=a7e3b50945201c28e7075d344cd85a877725d9e22945d180d1fbc3ed8fc984f4
        )
        FetchContent_MakeAvailable(pcanbasic)
    endif ()

    set(PCANBASIC_SRC_DIR ${pcanbasic_SOURCE_DIR}/libpcanbasic/pcanbasic)
    set(PCANBASIC_OUT_DIR ${PCANBASIC_SRC_DIR}/out)
    set(PCANBASIC_LIB ${PCANBASIC_SRC_DIR}/lib/libpcanbasic.a)

    # PEAK's own Makefile only links a shared .so; reuse the .o files it already builds
    # and archive them ourselves instead of patching their build - static, so no rpath
    # dance is needed at all (unlike the old vendored libpcanbasic.so).
    file(GLOB_RECURSE PCANBASIC_SOURCES ${PCANBASIC_SRC_DIR}/src/*.c ${PCANBASIC_SRC_DIR}/src/*.h)
    add_custom_command(
            OUTPUT ${PCANBASIC_LIB}
            COMMAND make -C ${PCANBASIC_SRC_DIR}
                    PCANFD_HEADER=${PCANBASIC_SRC_DIR}/src/pcan/driver/pcanxl.h
                    all
            COMMAND sh -c "ar rcs ${PCANBASIC_LIB} ${PCANBASIC_OUT_DIR}/*.o"
            DEPENDS ${PCANBASIC_SOURCES}
            COMMENT "Building libpcanbasic from source (static)"
            VERBATIM
    )
    add_custom_target(pcanbasic_build DEPENDS ${PCANBASIC_LIB})

    # libsocketcan isn't a CMake project (autotools), so FetchContent only fetches its
    # source; built as a static lib so there's no .so to find at runtime (no rpath needed,
    # unlike libpcanbasic above).
    if (NOT TARGET libsocketcan_build)
        include(FetchContent)
        FetchContent_Declare(
                libsocketcan
                GIT_REPOSITORY https://github.com/linux-can/libsocketcan.git
                GIT_TAG v0.0.12
        )
        FetchContent_MakeAvailable(libsocketcan)
    endif ()

    set(LIBSOCKETCAN_LIB ${libsocketcan_SOURCE_DIR}/src/.libs/libsocketcan.a)
    file(GLOB_RECURSE LIBSOCKETCAN_SOURCES ${libsocketcan_SOURCE_DIR}/src/*.c ${libsocketcan_SOURCE_DIR}/include/*.h)
    add_custom_command(
            OUTPUT ${LIBSOCKETCAN_LIB}
            COMMAND sh -c "./autogen.sh && ./configure --disable-shared --enable-static && make"
            WORKING_DIRECTORY ${libsocketcan_SOURCE_DIR}
            DEPENDS ${LIBSOCKETCAN_SOURCES}
            COMMENT "Building libsocketcan from source (static)"
            VERBATIM
    )
    add_custom_target(libsocketcan_build DEPENDS ${LIBSOCKETCAN_LIB})

    set(RCAN_THIRDPARTY ${PCANBASIC_SRC_DIR}/include ${PCANBASIC_SRC_DIR}/src/pcan/driver
            ${libsocketcan_SOURCE_DIR}/include)
    # libpcanbasic's pcaninfo.c calls truncf() (math.h) - needs an explicit libm on toolchains
    # that don't implicitly pull it in via libc.
    set(RCAN_LIBRARIES ${PCANBASIC_LIB} ${LIBSOCKETCAN_LIB} rnode m)
endif ()

add_library(${PROJECT_NAME} STATIC ${RCAN_SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${RCAN_LIBRARIES})
target_include_directories(${PROJECT_NAME}
        PUBLIC
        ./
        can/
        ${RCAN_THIRDPARTY})

if (UNIX AND NOT APPLE)
    # Both PCAN libs are static archives now, so there's no .so to find at runtime -
    # no target_link_directories/rpath needed, just make sure they're built first.
    add_dependencies(${PROJECT_NAME} pcanbasic_build libsocketcan_build)
endif ()

if (APPLE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC -DRCAN_MACOS)
endif ()

if (UNIX AND NOT APPLE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC -DRCAN_UNIX)
endif ()

#################### TEST SECTION #######################
set(TEST_SRC_FILES
        test/hardware_two_pcan.c
        test/hardware_linux_pcan_driver.c
        test/virtual_bus.c
        test/virtual_can.c
        test/test.c)

set(TEST_HEADER_FILES
        test/hardware_two_pcan.h
        test/hardware_linux_pcan_driver.h
        test/virtual_bus.h
        test/virtual_can.h
        test/test.h)

# Every variant whose body can actually compile on this host is built into one binary;
# which one runs is picked at runtime via argv (see test/test.c) rather than at configure
# time, so `ctest -N` can list each variant by name instead of hiding behind -DTEST_TARGET=....
# virtual_can needs Linux-only SocketCAN headers (pulled in transitively only under RCAN_UNIX).
# hardware_two_pcan needs the two real PCAN-USB (FD) adapters this project assumes - it picks
# its channel macros (PEAK_CAN_USBBUS1/2 vs SOCKET_CAN0/1) from RCAN_MACOS/RCAN_UNIX itself
# (see test/hardware_two_pcan.c), so the same file/flag covers both host platforms.
# hardware_linux_pcan_driver is Linux-only: same two adapters, reached through PEAK's own
# proprietary chardev driver (PEAK_CAN_USBBUS1/2 via libpcanbasic) instead of mainline
# peak_usb/SocketCAN - see README.md, "Switching Linux CAN drivers". Both it and
# hardware_two_pcan share their actual test bodies via test/hardware_pcan_scenarios.inc
# (textually #include'd, not compiled standalone - deliberately not listed in TEST_SRC_FILES).
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES Generic)
    if (NOT TARGET runit)
        include(FetchContent)
        FetchContent_Declare(
                runit
                GIT_REPOSITORY https://github.com/RoboticsHardwareSolutions/runit.git
                GIT_TAG main
        )
        FetchContent_MakeAvailable(runit)
    endif ()

    add_executable(${PROJECT_NAME}_test ${TEST_SRC_FILES})
    target_include_directories(${PROJECT_NAME}_test PRIVATE test/)
    target_link_libraries(${PROJECT_NAME}_test ${PROJECT_NAME} runit)
    target_compile_definitions(${PROJECT_NAME}_test PRIVATE RCAN_VIRTUAL_BUS_TEST)

    if (UNIX AND NOT APPLE)
        target_compile_definitions(${PROJECT_NAME}_test PRIVATE
                RCAN_UNIX_TEST_UNIX_VIRTUAL_CAN
                RCAN_HARDWARE_TWO_PCAN_TEST
                RCAN_HARDWARE_LINUX_PCAN_DRIVER_TEST)
    endif ()

    if (APPLE)
        target_compile_definitions(${PROJECT_NAME}_test PRIVATE RCAN_HARDWARE_TWO_PCAN_TEST)
    endif ()

    if (UNIX AND NOT APPLE)
        # virtual_can/hardware_unix call libsocketcan (can_do_start/can_do_stop/...), which
        # talks to the kernel over netlink directly - that needs CAP_NET_ADMIN on the process
        # itself, not just "some earlier command happened to run under sudo". Grant it via a
        # file capability so the test binary runs unprivileged otherwise, instead of needing
        # the whole test run wrapped in `sudo`.
        add_custom_command(TARGET ${PROJECT_NAME}_test POST_BUILD
                COMMAND sudo setcap cap_net_admin+ep $<TARGET_FILE:${PROJECT_NAME}_test>
                COMMENT "Granting rcan_test CAP_NET_ADMIN for SocketCAN netlink calls")
    endif ()

    enable_testing()
    add_test(NAME ${PROJECT_NAME}_virtual_bus COMMAND ${PROJECT_NAME}_test virtual_bus)
    add_test(NAME ${PROJECT_NAME}_virtual_can COMMAND ${PROJECT_NAME}_test virtual_can)
    add_test(NAME ${PROJECT_NAME}_hardware_two_pcan COMMAND ${PROJECT_NAME}_test hardware_two_pcan)
    add_test(NAME ${PROJECT_NAME}_hardware_linux_pcan_driver COMMAND ${PROJECT_NAME}_test hardware_linux_pcan_driver)
    # virtual_bus is pure host-side (rnode) and needs nothing special; the rest need either root
    # (Linux SocketCAN vcan) or the two real PCAN-USB adapters, so exclude them from a plain
    # `ctest` run with `ctest -LE "root|hardware"`.
    set_tests_properties(${PROJECT_NAME}_virtual_can PROPERTIES LABELS root)
    set_tests_properties(${PROJECT_NAME}_hardware_two_pcan ${PROJECT_NAME}_hardware_linux_pcan_driver
            PROPERTIES LABELS hardware)
    # Every receive wait in test/hardware_pcan_scenarios.inc is itself bounded
    # (RCAN_TEST_RECEIVE_TIMEOUT_SEC), so a missing/unwired adapter fails fast instead of hanging -
    # this is just a safety net in case that assumption is ever violated (e.g. a future test adds
    # an unbounded wait).
    set_tests_properties(${PROJECT_NAME}_hardware_two_pcan ${PROJECT_NAME}_hardware_linux_pcan_driver
            PROPERTIES TIMEOUT 90)

    if (UNIX AND NOT APPLE)
        # Bridges vcan0<->vcan1 via the Linux CAN_GW kernel gateway (cangw, from can-utils)
        # before rcan_virtual_can runs, and tears it down after - same FIXTURES_SETUP /
        # FIXTURES_CLEANUP / FIXTURES_REQUIRED pattern rserial uses for its socat ptys.
        add_test(NAME ${PROJECT_NAME}_can_gateway_start COMMAND ${CMAKE_CURRENT_LIST_DIR}/test/canfortests.sh start)
        add_test(NAME ${PROJECT_NAME}_can_gateway_stop COMMAND ${CMAKE_CURRENT_LIST_DIR}/test/canfortests.sh stop)
        set_tests_properties(${PROJECT_NAME}_can_gateway_start PROPERTIES FIXTURES_SETUP can_gateway)
        set_tests_properties(${PROJECT_NAME}_can_gateway_stop PROPERTIES FIXTURES_CLEANUP can_gateway)
        set_tests_properties(${PROJECT_NAME}_virtual_can PROPERTIES FIXTURES_REQUIRED can_gateway)
    endif ()
endif ()

#################### EXAMPLES SECTION #######################
add_subdirectory(examples)

#################### FORMAT SECTION #######################
set(FILES_FOR_FORMATTING
        ${TEST_SRC_FILES}
        ${TEST_HEADER_FILES}
        ${RCAN_SOURCES}
        ${RCAN_HEADER}
        examples/pcan_echo/main.c
        examples/virtual_bus/main.c
        test/hardware_pcan_scenarios.inc
)

if (NOT TARGET rlibhelper)
    include(FetchContent)
    FetchContent_Declare(
            rlibhelper
            GIT_REPOSITORY https://github.com/RoboticsHardwareSolutions/rlibhelper.git
            GIT_TAG main
    )
    FetchContent_MakeAvailable(rlibhelper)
endif ()

if (EXISTS "${rlibhelper_SOURCE_DIR}/format.cmake")
    include(${rlibhelper_SOURCE_DIR}/format.cmake)
else ()
    message(WARNING "format.cmake file not found")
endif ()
format_files(${PROJECT_NAME} SOURCES ${FILES_FOR_FORMATTING})
