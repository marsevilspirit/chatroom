cmake_minimum_required(VERSION 3.10)
project(chatroom)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加子目录
add_subdirectory(project/server)
add_subdirectory(project/threadpool)
add_subdirectory(project/message)

# 主项目构建
add_executable(chatroom 
    project/main.cc
)

add_executable(client
    project/client/client.cc 
    project/client/client_menu.cc
    project/client/mail.cc
    project/client/command_menu.cc
)

target_link_libraries(chatroom server)

target_link_libraries(client message -lesmtp -lmysqlclient -lssl -lcrypto -lpthread)

#target_include_directories(chatroom
#    PUBLIC
#    project/server
#)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")

    message("Debug build type detected. Adding debug flags.")
    add_compile_options(-g -O0)  # 添加调试信息和关闭优化
endif()
