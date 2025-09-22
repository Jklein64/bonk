cmake_minimum_required(VERSION 4.1.1)

# Configure Clang
set(CMAKE_C_COMPILER "clang-18")
set(CMAKE_CXX_COMPILER "clang++-18")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(bonk)

file(GLOB_RECURSE sources src/*.cpp src/*.h)

add_executable(${PROJECT_NAME} ${sources})
target_compile_options(${PROJECT_NAME} PUBLIC -std=c++2a -Wall -Werror)

find_package(Eigen3 REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC ${Eigen_LIBRARIES})