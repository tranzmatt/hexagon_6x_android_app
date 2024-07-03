cmake_minimum_required(VERSION 3.14.3)
project("android_app_dsp" C CXX ASM)
enable_language(ASM)
cmake_policy(SET CMP0115 OLD)

if(HEXAGON_SDK_ROOT)
    include(${HEXAGON_SDK_ROOT}/build/cmake/hexagon_fun.cmake)
else()
    include(${HEXAGON_CMAKE_ROOT}/hexagon_fun.cmake)
endif()
set(ignoreVar ${DEPENDENCIES}${V})

set(common_incs
        ${CMAKE_CURRENT_BINARY_DIR}/
        ${HEXAGON_SDK_ROOT}/incs/
        ${HEXAGON_SDK_ROOT}/incs/stddef/
        ${HEXAGON_SDK_ROOT}/utils/examples
        )

include_directories(${common_incs})

#libandroid_app_skel
add_library(android_app_skel_noasync SHARED
        ${CMAKE_CURRENT_BINARY_DIR}/android_app_skel
        android_app_imp
        task.S
        )

set(IDL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../app/src/main/cpp/inc)
build_idl(${IDL_DIR}/android_app.idl android_app_skel_noasync)
build_idl(${IDL_DIR}/android_app_defs.idl android_app_skel_noasync)
set_target_properties(android_app_skel_noasync PROPERTIES OUTPUT_NAME "android_app_skel_${DSP_VERSION}")

# Link worker_pool as static to accommodate multiple versions of libraries
link_custom_library(android_app_skel_noasync worker_pool_static)

#async skel
add_library(android_app_skel SHARED
        ${CMAKE_CURRENT_BINARY_DIR}/android_app_async_skel
        android_app_async_imp
        )

build_idl(${IDL_DIR}/android_app_async.idl android_app_skel)
set_target_properties(android_app_skel PROPERTIES OUTPUT_NAME "android_app_async_skel_${DSP_VERSION}")

#add dependencies
add_dependencies(android_app_skel android_app_skel_noasync)
#Link Libraries
target_link_libraries(android_app_skel android_app_skel_noasync)
#Copy binaries to CMAKE_CURRENT_BINARY_DIR
copy_binaries(android_app_skel android_app_skel_noasync)

install(TARGETS android_app_skel DESTINATION .)
install(TARGETS android_app_skel_noasync DESTINATION .)
