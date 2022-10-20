cmake_minimum_required(VERSION 3.10)

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  target_compile_options(cmake_cpp_boilerplate_compiler_options INTERFACE /W4 "/permissive-")
  if(BP_WARNING_AS_ERROR)
    target_compile_options(cmake_cpp_boilerplate_compiler_options INTERFACE /WX)
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
add_compile_options(-Wall -Wextra -pedantic -g -pipe)
endif()

# set the project name
project(Lox)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

add_subdirectory(src)

# add the executable
add_executable(Lox main.cpp)


target_link_libraries(Lox PUBLIC Error Lexer Interpreter Expr Error Parser Stmt Token Environment Function Buildin Logging Resolver Class Instance)
