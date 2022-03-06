#通用cmake顶层脚本
cmake_minimum_required(VERSION 2.6)

project(netlib LANGUAGES "CXX""C")
option(NETLIB_TEST "build netlib tests" ON)
set(CXX_FLAGS
 #添加调试信息
 -g
 #显示编译警告
 -Wall
 -Wextra
 #视警告为错误
 -Werror
 -Wconversion
 -Wno-unused-parameter
 #使用c风格转换时警告
 -Wold-style-cast
 #重载虚函数名时警告
 -Woverloaded-virtual
 #算术表达式使用指针时警告
 -Wpointer-arith
 #发生局部变量掩盖时警告
 -Wshadow
 -Wwrite-strings
 -march=native
 -std=c++11
 #将所有符号添加到动态符号表中
 -rdynamic
)
set(ADDRESS_MODEL 64)
set(NODE_TARGET x64)
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
 list(APPEND CMAKE_CXX_FLAGS "-Wno-null-dereference")
 list(APPEND CMAKE_CXX_FLAGS "-Wno-sign-conversion")
 list(APPEND CMAKE_CXX_FLAGS "-Wno-unused-local-typedef")
 list(APPEND CMAKE_CXX_FLAGS "-Wthread-safety")
 list(REMOVE_ITEM CMAKE_CXX_FLAGS "-rdynamic")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

find_package(Boost REQUIRED)
add_subdirectory(base)
add_subdirectory(net)
include_directories(${Boost_INCLUDE_DIRS})