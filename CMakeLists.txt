cmake_minimum_required(VERSION 3.23)
project(NoteBlockPlayer)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -s -ffunction-sections -fdata-sections -Wl,--gc-sections")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake
                      ${CMAKE_MODULE_PATH})

add_subdirectory(src)
