cmake_minimum_required(VERSION 3.14.3)
project("android_app")
cmake_policy(SET CMP0115 OLD)

if(HEXAGON_SDK_ROOT)
    include(${HEXAGON_SDK_ROOT}/build/cmake/hexagon_fun.cmake)
else()
    include(${HEXAGON_CMAKE_ROOT}/hexagon_fun.cmake)
endif()

set(common_incs
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/
        ${CMAKE_CURRENT_BINARY_DIR}/
        ${HEXAGON_SDK_ROOT}/incs/
        ${HEXAGON_SDK_ROOT}/incs/stddef/
        ${HEXAGON_SDK_ROOT}/utils/examples
)

set(VARIANT "")
if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
    set(VARIANT "Debug")
else()
    set(VARIANT "ReleaseG")
endif()

message(STATUS "=========================================")
message(STATUS "HEXAGON_SDK_ROOT: ${HEXAGON_SDK_ROOT}")
message(STATUS "BUILD_DSP: ${BUILD_DSP}")
message(STATUS "HLOS_TYPE: ${HLOS_TYPE}")
message(STATUS "DSP_VERSION: ${DSP_VERSION}")

function(build_hexagon target version)
    if (${version} MATCHES "v75")
        set(TOOL_VERSION "toolv87_v75")
    elseif (${version} MATCHES "v73")
        set(TOOL_VERSION "toolv87_v73")
    elseif (${version} MATCHES "v69")
        set(TOOL_VERSION "toolv87_v69")
    elseif (${version} MATCHES "v68")
        set(TOOL_VERSION "toolv87_v68")
    endif()

    set(DSP_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../dsp)
    set(DSP_OUTPUT_DIR ${DSP_SOURCE_DIR}/hexagon_${VARIANT}_${TOOL_VERSION})

    ExternalProject_Add(dsp_build_${version}
            SOURCE_DIR ${DSP_SOURCE_DIR}
            STAMP_DIR ${DSP_OUTPUT_DIR}
            BINARY_DIR ${DSP_OUTPUT_DIR}
            CMAKE_ARGS -DHEXAGON_SDK_ROOT=${HEXAGON_SDK_ROOT} -DV=hexagon_${VARIANT}_${TOOL_VERSION} -DPREBUILT_LIB_DIR=hexagon_${TOOL_VERSION}
                -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR} -DSYSTEM_TYPE=${SYSTEM_TYPE}
                -DCMAKE_SYSTEM_NAME="QURT" -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DDEPENDENCIES=
                -DCMAKE_VERBOSE_MAKEFILE=${CMAKE_VERBOSE_MAKEFILE} -DDSP_VERSION=${version} -DHEXAGON_TOOLS_ROOT=${HEXAGON_TOOLS_ROOT}
                -DCMAKE_TOOLCHAIN_FILE=${HEXAGON_SDK_ROOT}/build/cmake/hexagon_toolchain.cmake -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                -DCMAKE_GENERATOR=${CMAKE_GENERATOR} -DADD_SYMBOLS=${ADD_SYMBOLS} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=${DSP_OUTPUT_DIR}/ship -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${DSP_OUTPUT_DIR}/ship -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${DSP_OUTPUT_DIR}/ship
            BUILD_ALWAYS 1
            INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/
            BUILD_BYPRODUCTS ${DSP_OUTPUT_DIR}/ship/libandroid_app_skel_${version}.so ${DSP_OUTPUT_DIR}/ship/libandroid_app_async_skel_${version}.so
           )
    set_property(
            TARGET dsp_build_${version}
            APPEND
            PROPERTY
            ADDITIONAL_CLEAN_FILES ${DSP_OUTPUT_DIR}/
    )
    add_dependencies(${target} dsp_build_${version})
endfunction()

if (${OS_TYPE} MATCHES "HLOS")
    include_directories(
            ${common_incs}
    )

    #libandroid_app
    add_library(android_app SHARED
            ${CMAKE_CURRENT_BINARY_DIR}/android_app_stub
            )

    #libandroid_app_async
    add_library(android_app_async SHARED
            ${CMAKE_CURRENT_BINARY_DIR}/android_app_async_stub
            )

    choose_dsprpc(${DSP_TYPE} dsprpc)
    link_custom_library(android_app ${dsprpc})
    link_custom_library(android_app_async ${dsprpc})

    build_idl(inc/android_app.idl android_app)
    build_idl(inc/android_app_defs.idl android_app android_app_async)
    build_idl(inc/android_app_async.idl android_app_async)

    if (${OS_TYPE} MATCHES "AS_HLOS")

        # Create e2e library as SHARED to be packaged with the APK.
        add_library( android_app_apk
                SHARED
                ${CMAKE_CURRENT_SOURCE_DIR}/jni.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/app_run.c
                ${CMAKE_CURRENT_BINARY_DIR}/android_app_stub.c
                ${CMAKE_CURRENT_BINARY_DIR}/android_app_async_stub.c
                )

        target_compile_definitions(
                android_app_apk
                PUBLIC VERIFY_PRINT_ERROR
        )

        target_link_options(android_app_apk PUBLIC -llog)

        link_custom_library(android_app_apk ${dsprpc})
        build_hexagon(android_app_apk v68)
        build_hexagon(android_app_apk v69)
        build_hexagon(android_app_apk v73)
        build_hexagon(android_app_apk v75)

        set_property(TARGET android_app_apk android_app android_app_async PROPERTY LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${V}/ship)
        set_property(
                TARGET android_app_apk
                APPEND
                PROPERTY
                ADDITIONAL_CLEAN_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${V}/
        )
    else()

        #android_app exec
        add_executable(android_app_device
                ${CMAKE_CURRENT_SOURCE_DIR}/main
                ${CMAKE_CURRENT_SOURCE_DIR}/app_run
                )

        target_compile_definitions(
                android_app_device
                PUBLIC VERIFY_PRINT_ERROR
        )

        target_link_options(android_app_device PUBLIC -llog)
        link_custom_library(android_app_device ${dsprpc})
        link_custom_library(android_app_device googletest)

        set_target_properties(android_app_device PROPERTIES OUTPUT_NAME "android_app")

        #test exec
        add_executable(android_app_test
                ${CMAKE_CURRENT_SOURCE_DIR}/test
                ${CMAKE_CURRENT_SOURCE_DIR}/app_run
                )

        SET(CMAKE_CXX_CREATE_SHARED_LIBRARY "${CMAKE_CXX_CREATE_SHARED_LIBRARY} -lc++")
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc++")
        target_compile_definitions(
                android_app_test
                PUBLIC VERIFY_PRINT_ERROR
        )
        target_link_options(android_app_test PUBLIC -llog)
        link_custom_library(android_app_test ${dsprpc})
        link_custom_library(android_app_test googletest)

        if (${BUILD_DSP} MATCHES "1")
            build_hexagon(android_app v68)
            build_hexagon(android_app v69)
            build_hexagon(android_app v73)
            build_hexagon(android_app v75)
        endif()

    endif()
else()
    set(ignoreVar ${DEPENDENCIES}${V}${BUILD_NAME}${BUILD_TAG_NAME}${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}${CMAKE_LIBRARY_OUTPUT_DIRECTORY}${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    add_custom_target(android_app_skel)
    build_hexagon(android_app_skel ${DSP_VERSION})
endif()


