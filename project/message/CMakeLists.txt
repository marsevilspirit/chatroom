# 添加 threadpool 目标及其源文件
add_library(message
    message.cc 
    message.h
    ../json.hpp
)

target_link_libraries(message log mysql)

# 设置头文件路径（如果有头文件的话）
target_include_directories(message
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
