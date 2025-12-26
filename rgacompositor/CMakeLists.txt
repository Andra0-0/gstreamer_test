cmake_minimum_required(VERSION 3.10)
project(rgavideofilter)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED
  gstreamer-1.0
  gstreamer-video-1.0
  gstreamer-allocators-1.0
)

pkg_check_modules(LIBRGA REQUIRED librga)
if(NOT LIBRGA_FOUND)
  find_library(LIBRGA_LIBRARIES
    NAMES rga
    PATHS /usr/lib/aarch64-linux-gnu
  )
  find_path(LIBRGA_INCLUDE_DIRS
    NAMES rga.h
    PATHS /usr/include /usr/include/rga
  )
  if(LIBRGA_LIBRARIES AND LIBRGA_INCLUDE_DIRS)
    set(LIBRGA_FOUND TRUE)
  else()
    message(FATAL_ERROR "librga not found")
  endif()
endif()

set(
  RGA_COMPOSITOR_HEADERS
  blend.h
  rkrga.h
  compositor.h
  compositororc-dist.h
)

set(
  RGA_COMPOSITOR_SOURCES
  blend.c
  rkrga.c
  compositor.c
  compositororc-dist.c
)

add_library(gstrgacompositor SHARED
  ${RGA_COMPOSITOR_SOURCES}
  ${RGA_COMPOSITOR_HEADERS}
)

target_compile_options(gstrgacompositor PRIVATE
  -Wall
  # -Werror
  -fPIC
)
target_include_directories(gstrgacompositor PRIVATE
  ${GSTREAMER_INCLUDE_DIRS}
  ${LIBRGA_INCLUDE_DIRS}
)
target_link_libraries(gstrgacompositor
  ${GSTREAMER_LIBRARIES}
  ${LIBRGA_LIBRARIES}
)

# Install the plugin
if(DEFINED ENV{GST_PLUGIN_PATH_1_0})
  set(INSTALL_PLUGIN_PATH $ENV{GST_PLUGIN_PATH_1_0})
  install(TARGETS gstrgacompositor
    LIBRARY DESTINATION ${INSTALL_PLUGIN_PATH}
  )
else()
  message(WARNING "Not found GST_PLUGIN_PATH_1_0")
endif()




add_executable(test_rgacompositor test_rgacompositor.cpp)
target_include_directories(gstrgacompositor PRIVATE
  ${GSTREAMER_INCLUDE_DIRS}
)
target_link_libraries(test_rgacompositor
  ${GSTREAMER_LIBRARIES}
)