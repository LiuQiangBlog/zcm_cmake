cmake_minimum_required(VERSION 3.16)
project(zcm)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_C_STANDARD 11)
#set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(3rdparty/libzmq)

add_compile_options(-mcx16)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mcx16")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")
add_library(zcm_json STATIC zcm/json/jsoncpp.cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/gen)

file(GLOB ZCM_SRC
        zcm/*.c
        zcm/*.cpp
        zcm/*.hpp
        zcm/util/*.c
        zcm/util/*.cpp
        zcm/tools/*.cpp
        zcm/transport/*.c
        zcm/transport/*.cpp
        zcm/transport/udp/buffers.cpp
        zcm/transport/udp/buffers.hpp
        zcm/transport/udp/mempool.cpp
        zcm/transport/udp/mempool.hpp
        zcm/transport/udp/udp.cpp
        zcm/transport/udp/udp.hpp
        zcm/transport/udp/udpsocket.cpp
        zcm/transport/udp/udpsocket.hpp
        zcm/transport/lockfree/lf_*.c
)

option(BUILD_SHARED_ZCM "build zcm to shared library" ON)
if (BUILD_SHARED_ZCM)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_library(zcm SHARED ${ZCM_SRC})
else ()
    add_library(zcm STATIC ${ZCM_SRC})
endif ()

target_link_libraries(zcm PRIVATE zcm_json pthread rt libzmq)
target_compile_definitions(zcm PRIVATE -DZCM_API=__attribute__$\(visibility\(\"default\"$\)\))

target_compile_definitions(zcm PRIVATE
        -D_GNU_SOURCE
        -D_POSIX_C_SOURCE=200809L
)
target_compile_definitions(zcm PRIVATE
        USING_TRANS_IPC
        USING_TRANS_UDP
        USING_TRANS_IPCSHM
        USING_ZMQ
        USING_TRANS_INPROC
        USING_TRANS_SERIAL
        USING_TRANS_CAN
)

add_executable(zcm_gen
        gen/Main.cpp
        gen/ZCMGen.cpp
        gen/GetOpt.cpp
        gen/tokenize.c
        gen/emit/EmitC.cpp
        gen/emit/EmitCpp.cpp
        gen/emit/EmitJava.cpp
        gen/emit/EmitJulia.cpp
        gen/emit/EmitNode.cpp
        gen/emit/EmitPython.cpp
        gen/emit/Emitter.cpp
)

target_link_libraries(zcm_gen zcm zcm_json)
target_compile_definitions(zcm_gen PRIVATE
        -D_GNU_SOURCE
        -D_POSIX_C_SOURCE=200809L
)

option(BuildPython "Build Python Module" ON)
if (BuildPython)
    add_subdirectory(3rdparty/pybind11)
    include_directories(3rdparty/pybind11/include)
    add_subdirectory(python)
endif ()


