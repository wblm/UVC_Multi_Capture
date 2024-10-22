cmake_minimum_required(VERSION 3.27)
project(UVC_frame)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置 Http 的路径
set(HTTP_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/thirdParty/httplib/include)

# 添加 Http 头文件路径
include_directories(${HTTP_INCLUDE_DIR})

# 查找 OpenCV 包
find_package(OpenCV REQUIRED)

# 包含 OpenCV 的头文件目录
include_directories(${OpenCV_INCLUDE_DIRS})

# 添加头文件的搜索路径
include_directories(${CMAKE_SOURCE_DIR}/thirdParty/libuvc/include)

# 添加子项目目录
#add_subdirectory(Test/UVC_Video)
#add_subdirectory(Test/White_balance_auto)

add_executable(UVC_frame main.cpp
        usbCamera.cpp
        usbCamera.h)
# 查找库文件
find_library(UVC_LIBRARY NAMES uvc HINTS ${CMAKE_SOURCE_DIR}/thirdParty/libuvc/lib)

if(NOT UVC_LIBRARY)
    message(FATAL_ERROR "libuvc library not found")
endif()

# 链接动态库
target_link_libraries(UVC_frame ${UVC_LIBRARY}  ${OpenCV_LIBS})
