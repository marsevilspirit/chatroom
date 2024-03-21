# 添加 server 目标及其源文件
add_library(server
    server.cc
    server.h
    ../threadpool/threadpool.cc 
    ../threadpool/threadpool.h
    ../mysql/mysql.cc 
    ../mysql/mysql.h
)

target_link_libraries(server threadpool message -lmysqlclient)

# 设置头文件路径（如果有头文件的话）
#target_include_directories(server
#    PUBLIC
#    ${CMAKE_CURRENT_SOURCE_DIR}
#    ../threadpool
#)
