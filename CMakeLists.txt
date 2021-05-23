cmake_minimum_required(VERSION 3.10)

# set the project name
project(MyLang)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

add_subdirectory(src)

# add the executable
add_executable(MyLang main.cpp)

target_link_libraries(MyLang PUBLIC Error Lexer Interpreter Expr Error Parser Stmt Token Environment Callable Function Buildin Logging)
