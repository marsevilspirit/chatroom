# 添加 threadpool 目标及其源文件
add_library(threadpool
    threadpool.h
    threadpool.cc
)

# 设置头文件路径（如果有头文件的话）
target_include_directories(threadpool
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
