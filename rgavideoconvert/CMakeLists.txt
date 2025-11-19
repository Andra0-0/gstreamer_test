cmake_minimum_required(VERSION 3.10)
project(rgavideoconvert)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-video-1.0)

set(
  RGA_VIDEO_CONVERT_HEADERS
  gstrgavideoconvert.h
)

set(
  RGA_VIDEO_CONVERT_SOURCES
  gstrgavideoconvert.cpp
)

add_library(gstrgavideoconvert SHARED
  ${RGA_VIDEO_CONVERT_SOURCES}
  ${RGA_VIDEO_CONVERT_HEADERS}
)
target_compile_features(gstrgavideoconvert PRIVATE cxx_std_11)
target_compile_options(gstrgavideoconvert PRIVATE
  -Wall
  -Werror
  -fPIC
)
target_include_directories(gstrgavideoconvert PRIVATE ${GSTREAMER_INCLUDE_DIRS})
target_link_libraries(gstrgavideoconvert ${GSTREAMER_LIBRARIES})

# Install the plugin
if(DEFINED ENV{GST_PLUGIN_PATH_1_0})
  set(INSTALL_PLUGIN_PATH $ENV{GST_PLUGIN_PATH_1_0})
  install(TARGETS gstrgavideoconvert
    LIBRARY DESTINATION ${INSTALL_PLUGIN_PATH}
  )
else()
  message(WARNING "Not found GST_PLUGIN_PATH_1_0")
endif()