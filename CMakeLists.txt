cmake_minimum_required(VERSION 3.20)
project(ThreadPool)

set(CMAKE_CXX_STANDARD 17)

add_executable(ThreadPool main.cpp
        BlockingQueue.h
        AutoThread.h
        "Utility.h"
        WorkBranch.h
        Supervisor.h
        Workspace.h)

target_link_libraries(ThreadPool PUBLIC pthread)