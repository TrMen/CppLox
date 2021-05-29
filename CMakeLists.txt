cmake_minimum_required(VERSION 3.10)

# flags mostly according to https://developers.redhat.com/blog/2018/03/21/compiler-and-linker-flags-gcc
add_compile_options(-Wall -Wextra -pedantic -g -pipe)

# set the project name
project(MyLang)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

add_subdirectory(src)

# add the executable
add_executable(MyLang main.cpp)

target_link_libraries(MyLang PUBLIC Error Lexer Interpreter Expr Error Parser Stmt Token Environment Function Buildin Logging)
