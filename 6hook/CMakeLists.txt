# 设置 CMake 的最低版本要求
cmake_minimum_required(VERSION 3.10)

# 定义项目名称和使用的 C++ 标准
project(ab_fiber)
set(CMAKE_CXX_STANDARD 17) # 设置 C++ 标准为 C++11

# 添加编译选项
add_definitions(-pthread)

# 指定源文件
set(SOURCE_FILES
    ab_fiber.cpp
    fd_manager.cpp
    fiber.cpp
    hook.cpp
    ioscheduler.cpp
    scheduler.cpp
    thread.cpp
    timer.cpp
)

# 指定生成的可执行文件和相关源文件
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# 添加链接库
# 添加编译选项和链接选项
target_compile_options(${PROJECT_NAME} PRIVATE "-Wall")
target_link_libraries(${PROJECT_NAME} PRIVATE "-pthread" dl)

