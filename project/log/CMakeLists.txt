# 添加 threadpool 目标及其源文件
add_library(log
    mars_logger.h 
    mars_logger.cc
)

target_link_libraries(log -lfmt -lstdc++fs -ljsoncpp)

# 设置头文件路径（如果有头文件的话）
target_include_directories(log
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
