# 添加 server 目标及其源文件
add_library(server
    server.cc
    server.h
)

target_link_libraries(server threadpool message log mysql)

# 设置头文件路径（如果有头文件的话）
#target_include_directories(server
#    PUBLIC
#    ${CMAKE_CURRENT_SOURCE_DIR}
#    ../threadpool
#)
