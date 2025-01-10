cmake_minimum_required(VERSION 3.28)
project(OS2)

set(CMAKE_CXX_STANDARD 17)

add_executable(OS2 main.cpp
        spinlock_intraproc.cpp
        spinlock_inter_process.cpp
        sem_intra.cpp)
