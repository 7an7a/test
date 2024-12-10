cmake_minimum_required(VERSION 3.16)

# 包含 ESP-IDF 的项目配置
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# 指定额外的组件目录
set(EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/rc522")






# 定义项目名称
project(https)
