cmake_minimum_required(VERSION 3.10)
project(Semestralka C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE Debug)

add_executable(server
    server.c
    hra.c
)

target_link_libraries(server pthread m)

add_executable(klient
    klient.c
    hrac.c
)

target_link_libraries(klient pthread)
