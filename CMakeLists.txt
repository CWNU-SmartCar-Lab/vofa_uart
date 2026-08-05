cmake_minimum_required(VERSION 3.22)
# Enable CMake support for ASM and C languages
enable_language(C CXX ASM)

# 显式列出源文件：aux_source_directory 在新增文件时不会触发 CMake 重新配置
target_sources(
        ${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/base_transfer.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/vofa_function.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/vofa_uart.c
)

target_include_directories(
        ${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)
