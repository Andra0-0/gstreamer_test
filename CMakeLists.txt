# ./CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(media_hub)

# 设置C标准
# set(C_STANDARD 11)

# 添加编译选项
add_compile_options(-g)
add_compile_options(-Wshadow)
add_compile_options(-Wall)

# 添加自定义pkg-config路径
set(ENV{PKG_CONFIG_SYSROOT_DIR} ${CMAKE_SYSROOT})

set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${CMAKE_SYSROOT}/usr/lib/pkgconfig")
set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${CMAKE_SYSROOT}/usr/share/pkgconfig")

# 查找依赖库
find_package(PkgConfig REQUIRED)
pkg_check_modules(DEPS REQUIRED
  gstreamer-1.0
  gstreamer-video-1.0
)

# 添加头文件搜索路径
include_directories(
  /usr/include/drm
  ${CMAKE_SOURCE_DIR}/matrix
  ${DEPS_INCLUDE_DIRS}
)

# link_directories(${SHERPA_ONNX_LIB_DIR})

# 获取src目录下的所有源文件
# file(GLOB_RECURSE SOURCES "matrix/*.c" "matrix/*.cpp")

# 待完善    这些源文件因为功能变更，已经不适用需要改动
# 移除不需要编译的文件
# list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/src/rasa_nlp.c")


# 添加可执行文件
# 测试
# add_executable(media_hub ${SOURCES})

# 链接库
# target_link_libraries(media_hub
#     ${DEPS_LIBRARIES}
#     -lpthread
#     -lm
#     -ldl
# )

# 安装目标
# install(TARGETS media_hub DESTINATION bin)

# 使能测试
# enable_testing()

#lib时移除main
#file(GLOB_RECURSE SOURCES "src/*.c" "src/*.cpp")
#list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/src/main.c")
#add_library(libmedia_hub STATIC ${SOURCES})
#TODO      当前仅测试部分
# add_library(libmedia_hub STATIC "${CMAKE_SOURCE_DIR}/src/matrix_image.c" "${CMAKE_SOURCE_DIR}/src/media_hub.c")

# target_link_libraries(libmedia_hub
#   ${DEPS_LIBRARIES}
#   -lpthread
#   -lm
#   -ldl
# )

add_subdirectory(matrix)
# add_subdirectory(rgavideoconvert)