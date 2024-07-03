# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "/home/mclark/StudioProjects/hexagon_6x_android_app/app/src/main/cpp/android_Debug_aarch64"
  "/home/mclark/StudioProjects/hexagon_6x_android_app/dsp/hexagon_Debug_toolv87_v68"
  "/home/mclark/StudioProjects/hexagon_6x_android_app/dsp/hexagon_Debug_toolv87_v69"
  "/home/mclark/StudioProjects/hexagon_6x_android_app/dsp/hexagon_Debug_toolv87_v73"
  "/home/mclark/StudioProjects/hexagon_6x_android_app/dsp/hexagon_Debug_toolv87_v75"
  )
endif()
