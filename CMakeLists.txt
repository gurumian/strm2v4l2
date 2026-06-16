cmake_minimum_required(VERSION 3.16)

project(strm2v4l2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)

pkg_check_modules(GSTREAMER_RTSP_SERVER QUIET IMPORTED_TARGET gstreamer-rtsp-server-1.0)
if(NOT GSTREAMER_RTSP_SERVER_FOUND)
    message(FATAL_ERROR
        "gstreamer-rtsp-server-1.0 not found.\n"
        "Install the development package:\n"
        "  sudo apt install libgstrtspserver-1.0-dev"
    )
endif()

add_executable(file_rtsp_server src/file_rtsp_server.cpp)

target_link_libraries(file_rtsp_server
    PRIVATE
        PkgConfig::GSTREAMER
        PkgConfig::GSTREAMER_RTSP_SERVER
)

target_compile_options(file_rtsp_server PRIVATE -Wall -Wextra)
