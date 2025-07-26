cmake_minimum_required(VERSION 3.15)
project(PlayerServer)

# 配置debug文件夹
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug/bin)

set(TOP_DIR ${CMAKE_SOURCE_DIR})

# 创建全局可访问的接口目标 - 最佳实践
add_library(project_includes INTERFACE)
add_library(project_thirdparty INTERFACE)

# 定义源文件
set(SRC_LIST main_.cpp)

# 设置 FFmpeg 和 C++库 根路径
set(FFMPEG_ROOT ${CMAKE_SOURCE_DIR}/ffmpeg)
set(CPP_ROOT C:/msys64/ucrt64)
# set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  
set(VLD_PATH "D:\\Visual Leak detector\\Visual Leak Detector")


# 设置全局头文件目录
target_include_directories( project_includes INTERFACE
    ${FFMPEG_ROOT}/include
    ${CPP_ROOT}/include
    ${VLD_PATH}/include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/DirectXMath/inc
)

# 查找 FFmpeg 静态库
find_library(AVCODEC_LIB avcodec PATHS ${FFMPEG_ROOT}/lib REQUIRED)
find_library(AVFORMAT_LIB avformat PATHS ${FFMPEG_ROOT}/lib REQUIRED)
find_library(AVUTIL_LIB avutil PATHS ${FFMPEG_ROOT}/lib REQUIRED)
find_library(SWSCALE_LIB swscale PATHS ${FFMPEG_ROOT}/lib REQUIRED)
find_library(SWRESAMPLE_LIB swresample PATHS ${FFMPEG_ROOT}/lib REQUIRED)
find_library(AVFILTER_LIB avfilter PATHS ${FFMPEG_ROOT}/lib REQUIRED)
find_library(AVDEVICE_LIB avdevice PATHS ${FFMPEG_ROOT}/lib REQUIRED)
find_library(D3D11 d3d11 PATHS ${CPP_ROOT}/lib REQUIRED)
find_library(D3DCOMPILER d3dcompiler PATHS ${CPP_ROOT}/lib REQUIRED)
find_library(DXGI dxgi PATHS ${CPP_ROOT}/lib REQUIRED)
find_library(WS2_32 ws2_32 PATHS ${CPP_ROOT}/lib REQUIRED)

# 配置VLD内存检测
find_library(VLD vld PATHS ${}/lib PATHS  ${VLD_PATH}/lib/Win64 REQUIRED)
if(NOT VLD)
    message(WARNING "VLD library not found. Memory leak detection disabled.")
else()
    message(STATUS "Found VLD library: ${VLD}")
endif()

# 链接所有静态库
target_link_libraries(project_thirdparty INTERFACE
    ${AVCODEC_LIB}
    ${AVFORMAT_LIB}
    ${AVUTIL_LIB}
    ${SWSCALE_LIB}
    ${SWRESAMPLE_LIB}
    ${AVFILTER_LIB} 
    ${AVDEVICE_LIB}
    ${D3D11}
    ${DXGI}
    ${WS2_32}
    ${VLD}
    ${D3DCOMPILER}
)

# 添加子目录
add_subdirectory(ZGPU)
add_subdirectory(ZTest)
add_subdirectory(ZAudio)
add_subdirectory(ZUDP)
add_subdirectory(ZQueue)
add_subdirectory(ZCapture)
add_subdirectory(ZServer)
add_subdirectory(ZTPool)
add_subdirectory(ZControl)

# 添加根目录cpp文件静态库
# add_library(control control.cpp)
# target_link_libraries(control PRIVATE project_includes)
# add_library(texturepool texturepool.cpp)
# target_link_libraries(texturepool PRIVATE project_includes queue) 

# include(opencl.cmake)
# add_library(opencl test_opencl.cpp)
# target_link_libraries(opencl PRIVATE dep::opencl project_includes)

# 添加可执行文件
add_executable(player ${SRC_LIST})

# 链接全局包含目标及子目录功能库
target_link_libraries(player PRIVATE
    project_includes
    project_thirdparty
    gpu
    # test
    control
    # audio
    # opencl
    # dep::opencl
    udp_streamer
    queue
    texturepool
    capture
    server
)
