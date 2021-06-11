cmake_minimum_required(VERSION 3.10)

add_compile_options(-Wall -Wextra -pedantic -g -pipe)

# set the project name
project(Lox)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

add_subdirectory(src)

# add the executable
add_executable(Lox main.cpp)

target_link_libraries(Lox PUBLIC Error Lexer Interpreter Expr Error Parser Stmt Token Environment Function Buildin Logging Resolver Class Instance)
