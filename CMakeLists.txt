cmake_minimum_required(VERSION 3.28)
project(OS2)

set(CMAKE_CXX_STANDARD 17)

add_executable(OS2
        spinlock_intraproc.cpp
        spinlock_interproc.cpp
        sem_intraproc.cpp
        docker/client.cpp
        zmq_ipc.cpp
        better_spinlock.cpp
        zmq_inproc.cpp
)
