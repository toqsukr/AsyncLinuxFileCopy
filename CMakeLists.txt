cmake_minimum_required(VERSION 3.26)
project(AsyncLinuxFileCopy)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(AsyncLinuxFileCopy
        AsyncManager.h
        File.h
        FileManager.h
        main.cpp
        util.h)
