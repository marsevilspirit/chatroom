# 添加 threadpool 目标及其源文件
add_library(mysql
    mysql.cc
    mysql.h
)

# 设置头文件路径（如果有头文件的话）
target_include_directories(mysql
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

