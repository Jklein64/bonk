cmake_minimum_required(VERSION 4.1.1)

# Set to Release for optimizations
set(CMAKE_BUILD_TYPE Debug)

# Configure Clang
set(CMAKE_C_COMPILER "clang-18")
set(CMAKE_CXX_COMPILER "clang++-18")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


project(bonk)

file(GLOB_RECURSE sources src/*.cpp src/*.h)

add_executable(${PROJECT_NAME} ${sources})
target_compile_options(${PROJECT_NAME} PUBLIC -std=c++2a -Wall -Werror)
target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)

# Log debug logs?
option(ENABLE_DEBUG_LOGS OFF)
if (ENABLE_DEBUG_LOGS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ENABLE_DEBUG_LOGS)
endif()

find_package(Eigen3 REQUIRED)
find_package(fmt REQUIRED)
find_package(iir REQUIRED)
find_package(spdlog REQUIRED)

# Non-system deps
include(FetchContent)
set(HTTPLIB_USE_NON_BLOCKING_GETADDRINFO NO) # Fails to link if this is YES, which is the default
FetchContent_Declare(libnpy GIT_REPOSITORY https://github.com/matajoh/libnpy.git GIT_TAG v1.5.3)
FetchContent_Declare(httplib GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git GIT_TAG v0.26.0)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_Declare(base64 GIT_REPOSITORY https://github.com/tobiaslocker/base64.git GIT_TAG 0d0f5a8849a954d96af7befdb5c4014e78ecc9e2)
FetchContent_declare(soxrpp GIT_REPOSITORY https://github.com/Jklein64/soxrpp.git GIT_TAG main)
FetchContent_MakeAvailable(libnpy httplib json base64 soxrpp)

target_link_libraries(${PROJECT_NAME} PUBLIC
    ${Eigen_LIBRARIES} 
    fmt::fmt 
    npy::npy 
    iir::iir 
    httplib::httplib 
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    base64
    soxrpp::soxrpp
)