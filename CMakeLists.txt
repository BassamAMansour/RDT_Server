cmake_minimum_required(VERSION 3.15)
project(RDT_Server)

set(CMAKE_CXX_STANDARD 17)

add_executable(RDT_Server main.cpp PacketUtils.cpp PacketUtils.h Packets.h CongestionManager.cpp CongestionManager.h)