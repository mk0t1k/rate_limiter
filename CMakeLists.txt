cmake_minimum_required(VERSION 3.13)

project(
    rate_limiter
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
FILE(GLOB RATE_LIMITER_APP_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_executable(${PROJECT_NAME} ${RATE_LIMITER_APP_SRC})

add_definitions("-g")