cmake_minimum_required(VERSION 2.8)
project(ChatServer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
set(TARGET ChatServer)


set(Boost_USE_STATIC_LIBS ON)

find_package(Boost COMPONENTS program_options REQUIRED)
if(NOT Boost_FOUND)
    message(SEND_ERROR "Failed to find boost library component")
    return()
else()
    include_directories(${Boost_INCLUDE_DIRS})
endif()


include_directories(include)

add_executable(${TARGET} src/main.cpp)

add_library(logger src/logger.cpp)
add_library(epoll src/epoll.cpp)
add_library(socket src/socket.cpp)
add_library(client src/client.cpp)
add_library(server src/server.cpp)

target_link_libraries(${TARGET} server
                                client
                                socket
                                epoll
                                logger
                                ${Boost_LIBRARIES})