cmake_minimum_required(VERSION 3.0)
project(tcp_client C)

set(CMAKE_C_STANDARD 23)

add_executable(tcp_client src/main.c
        src/log.c
        src/tcp_client.c)
