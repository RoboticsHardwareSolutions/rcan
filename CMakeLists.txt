cmake_minimum_required(VERSION 3.9)
project(librcan
        VERSION 0.2.0
        LANGUAGES C
        DESCRIPTION
        "cross platform lib with simple C API for work with CAN BUS")

include(rcmake_macros/rcmake_macros.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (TEST_TARGET STREQUAL "virtual_can")
    message("Compile test for virtual_can")
    add_definitions(-DRCAN_VIRTUAL_CAN_TEST=1)
elseif (TEST_TARGET STREQUAL "virtual_bus")
    message("Compile test for virtual_bus")
    add_definitions(-DRCAN_VIRTUAL_BUS_TEST=1)
elseif (TEST_TARGET STREQUAL "hardware_macos")
    message("Compile test for hardware_macos")
    add_definitions(-DRCAN_HARDWARE_MACOS_TEST=1)
elseif (TEST_TARGET STREQUAL "hardware_unix")
    message("Compile test for hardware_macos")
    add_definitions(-DCAN_HARDWARE_UNIX_TEST=1)
else ()
    message(FATAL_ERROR "TEST_TARGET should be specified")
endif ()


if (MSVC OR MSYS OR MINGW)
    set(RCAN_THIRDPARTY ${CMAKE_CURRENT_LIST_DIR}/thirdparty/PCAN-Basic-windows)
    link_directories(${RCAN_DIRECTORIES})
    find_package(nanomsg CONFIG REQUIRED)
    #set(PCBUSB_LIBRARIES PCANBasic.lib -static gcc stdc++ winpthread -dynamic)
    set(PCAN_LIBRARIES PCANBasic.lib)
    file(COPY ${CMAKE_CURRENT_LIST_DIR}/thirdparty/PCAN-Basic-windows/PCANBasic.dll DESTINATION ${PROJECT_BINARY_DIR})
    add_definitions(-DRCAN_WINDOWS)
    set(RCAN_LIBRARIES "${PCAN_LIBRARIES}")
    set(RCAN_LIBRARIES ${RCAN_LIBRARIES} nanomsg)
endif ()

if (APPLE)
    set(RCAN_THIRDPARTY ${CMAKE_CURRENT_LIST_DIR}/thirdparty/PCAN-Basic-mac)
    find_library(PCAN_LIBRARIES NAMES pcbusb)
    find_library(NANOMSG_LIBRARIES NAMES nanomsg)
    find_package(nanomsg)
    add_definitions(-DRCAN_MACOS)
    set(RCAN_LIBRARIES "${PCAN_LIBRARIES}")
    set(RCAN_LIBRARIES ${RCAN_LIBRARIES} "${NANOMSG_LIBRARIES}")
endif ()


if (UNIX AND NOT APPLE)
    set(RCAN_THIRDPARTY ${CMAKE_CURRENT_LIST_DIR}/thirdparty/PCAN-Basic-linux)
    link_directories(${CMAKE_CURRENT_LIST_DIR}/thirdparty/PCAN-Basic-linux)
    set(PCBUSB_LIBRARIES pcanbasic)
    add_definitions(-DRCAN_UNIX)
    set(RCAN_LIBRARIES ${PCBUSB_LIBRARIES})
    set(RCAN_LIBRARIES ${RCAN_LIBRARIES} socketcan)
    set(RCAN_LIBRARIES ${RCAN_LIBRARIES} nanomsg)
endif ()


set(RCAN_HEADER
        rcan.h
        rcan_timing.h
        rcan_def.h
        rcan_filter.h
        can/bx_canfd.h
        can/bx_can.h
        can/u_can.h
        )

set(RCAN_SRC_FILES
        ${RCAN_HEADER}
        rcan.c
        rcan_filter.c
        rcan_timing.c
        can/bx_can.c
        can/bx_canfd.c
        can/u_can.c
        can/bx_can.h
        can/bx_canfd.h)

set(RCAN_FILES
        ${RCAN_HEADER}
        ${RCAN_SRC_FILES}
        ${RCAN_THIRDPARTY}
        )

set(TEST_FRAMEWORK_SRC
        test/runit/runit.c
        )
set(TEST_FRAMEWORK_HEADER
        test/runit/runit.h
        )

set(TEST_SRC_FILES
        test/hardware_unix.c
        test/virtual_bus.c
        test/virtual_can.c
        test/test.c)

set(TEST_HEADER_FILES
        test/hardware_unix.h
        test/virtual_bus.h
        test/virtual_can.h
        test/test.h)

set(FILES_FOR_FORMATTING
        ${TEST_SRC_FILES}
        ${TEST_HEADER_FILES}
        ${RCAN_SRC_FILES}
        ${RCAN_HEADER}
        )


# Use -DNO_STATIC_ANALYSIS=1 to suppress static analysis.
# If not suppressed, the tools used here shall be available, otherwise the build will fail.
if (NOT NO_STATIC_ANALYSIS)
    # clang-tidy (separate config files per directory)
    find_program(clang_tidy NAMES clang-tidy)
    if (NOT clang_tidy)
        message(FATAL_ERROR "Could not locate clang-tidy")
    endif ()
    message(STATUS "Using clang-tidy: ${clang_tidy}")
    set(CMAKE_C_CLANG_TIDY ${clang_tidy})
    set(CMAKE_CXX_CLANG_TIDY ${clang_tidy})
endif ()

# clang-format
find_program(clang_format NAMES clang-format)
if (NOT clang_format)
    message(STATUS "Could not locate clang-format")
else ()
    file(GLOB format_files ${FILES_FOR_FORMATTING})
    message(STATUS "Using clang-format: ${clang_format}; files: ${format_files}")
    add_custom_target(format COMMAND ${clang_format} -i -fallback-style=none -style=file --verbose ${format_files})
endif ()


# -----------------------------------------------------------------------------
# Build targets
# -----------------------------------------------------------------------------
# Copy the library API header into the build directory.
# With this command, if the header changes, it's copied, otherwise not.
#configure_file(${LIB_HEADER} ${CMAKE_BINARY_DIR} COPYONLY)

# Static libraries (.a), compiled with all features or only some features
add_library(rcan STATIC ${RCAN_FILES})
target_include_directories(rcan
        PUBLIC ./
        PRIVATE can/
        PRIVATE ${RCAN_THIRDPARTY}
        PRIVATE ${nanomsg_INCDIR})
set_target_properties(rcan PROPERTIES
        INTERPROCEDURAL_OPTIMISATION TRUE)

# Shared library (.so / .dylib / .dll)
# Does not reuse the static library object files, as they are
# recompiled in order to have position-independent code
add_library(rcanshared SHARED ${RCAN_FILES})
target_include_directories(rcanshared
        PUBLIC ./
        PRIVATE can/
        PRIVATE ${RCAN_THIRDPARTY}
        PRIVATE ${nanomsg_INCDIR})
set_target_properties(rcanshared PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        INTERPROCEDURAL_OPTIMISATION TRUE
        # Remove any "msys-" and enforce the same lib name with all toolchains
        PREFIX lib
        OUTPUT_NAME rcan)


# Test runner executable testing the static library
add_executable(testrcan
        ${TEST_FRAMEWORK_SRC}
        ${TEST_SRC_FILES})
target_include_directories(testrcan
        PUBLIC ./
        PRIVATE can/
        PRIVATE test/
        PRIVATE test/runit/
        PRIVATE ${RCAN_THIRDPARTY}
        PRIVATE ${nanomsg_INCDIR})
add_dependencies(testrcan  rcan nanomsg pcbusb )
target_link_libraries(testrcan
        PUBLIC rcan
        PUBLIC pcbusb)
set_target_properties(testrcan PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        INTERPROCEDURAL_OPTIMISATION TRUE)

# Test runner executable testing the shared library
add_executable(testrcanshared
        ${TEST_FRAMEWORK}
        ${TEST_SRC_FILES})
target_include_directories(testrcanshared
        PUBLIC can/
        PRIVATE test/
        PRIVATE test/runit/)
add_dependencies(testrcanshared rcan)
target_link_libraries(testrcanshared PUBLIC rcan)
set_target_properties(testrcanshared PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        INTERPROCEDURAL_OPTIMISATION TRUE)

# Doxygen documentation builder
find_package(Doxygen OPTIONAL_COMPONENTS dot)
if (DOXYGEN_FOUND)
    # Cmake's wrapper of Doxygen, constructing a doxyfile from the
    # DOXYGEN_* variables, which are mapped to the Doxygen variables.

    # Parts of the source documentation to work on
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_EXTRACT_PRIVATE NO)
    set(DOXYGEN_EXTRACT_PRIV_VIRTUAL NO)
    set(DOXYGEN_EXTRACT_PACKAGE NO)
    set(DOXYGEN_EXTRACT_STATIC NO)
    set(DOXYGEN_EXTRACT_LOCAL_CLASSES NO)
    set(DOXYGEN_EXTRACT_LOCAL_METHODS NO)
    set(DOXYGEN_EXTRACT_ANON_NSPACES NO)
    set(DOXYGEN_INTERNAL_DOCS NO)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
    set(DOXYGEN_IMAGE_PATH ${PROJECT_SOURCE_DIR}/images)

    # How to process the source code
    set(DOXYGEN_INPUT_ENCODING UTF-8)
    set(DOXYGEN_BRIEF_MEMBER_DESC YES)
    set(DOXYGEN_REPEAT_BRIEF YES)
    set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
    set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
    set(DOXYGEN_MARKDOWN_SUPPORT YES)
    set(DOXYGEN_TAB_SIZE 4)

    # Components and look of the output
    set(DOXYGEN_OUTPUT_LANGUAGE English)
    set(DOXYGEN_TOC_INCLUDE_HEADINGS 5)
    set(DOXYGEN_AUTOLINK_SUPPORT YES)
    set(DOXYGEN_HIDE_UNDOC_MEMBERS NO)
    set(DOXYGEN_HIDE_UNDOC_CLASSES NO)
    set(DOXYGEN_HIDE_IN_BODY_DOCS NO)
    set(DOXYGEN_SORT_MEMBER_DOCS NO)
    set(DOXYGEN_SORT_BRIEF_DOCS NO)
    set(DOXYGEN_MAX_INITIALIZER_LINES 30)
    #set(DOXYGEN_PROJECT_LOGO )

    # Format of the output
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_MAN YES)

    # Processing
    set(DOXYGEN_NUM_PROC_THREADS 0)  # As many as CPU cores
    set(DOXYGEN_QUIET YES)
    set(DOXYGEN_WARNINGS YES)
    set(DOXYGEN_WARN_IF_UNDOCUMENTED YES)
    set(DOXYGEN_WARN_IF_DOC_ERROR YES)
    set(DOXYGEN_WARN_NO_PARAMDOC YES)
    set(DOXYGEN_WARN_AS_ERROR YES)
    if (DOT_FOUND)
        set(DOXYGEN_DOT_PATH) # Empty = find it in PATH
        set(DOXYGEN_DOT_NUM_THREADS 0)  # As many as CPU cores
        set(DOXYGEN_CALL_GRAPH YES)
        set(DOXYGEN_CALLER_GRAPH YES)
        set(DOXYGEN_DIRECTORY_GRAPH YES)
    endif ()

    # Customisations
    set(DOXYGEN_ALIASES license="**License:**")
    set(DOXYGEN_PREDEFINED WIN32 ASCON_INPUT_ASSERTS)

    doxygen_add_docs(rcan_doxygen
            # Do NOT build doxygen on make-all, to avoid polluting the stdout
            # List of input files for Doxygen
            ${PROJECT_SOURCE_DIR}/rcan.h
            ${PROJECT_SOURCE_DIR}/LICENSE.md
            ${PROJECT_SOURCE_DIR}/AUTHORS.md
            ${PROJECT_SOURCE_DIR}/README.md
            ${PROJECT_SOURCE_DIR}/CHANGELOG.md
            COMMENT "Generating Doxygen documentation...")
else (DOXYGEN_FOUND)
    message(WARNING "Doxygen not found. Cannot generate documentation.")
endif (DOXYGEN_FOUND)

#print_all_variables()
enable_testing()
add_test(NAME testrcan COMMAND ./testrcan)
add_test(NAME testrcanshared COMMAND ./testrcanshared)
