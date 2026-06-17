cmake_minimum_required(VERSION 3.13)

project(
    rate_limiter
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
FILE(GLOB RATE_LIMITER_APP_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_executable(${PROJECT_NAME} ${RATE_LIMITER_APP_SRC})

add_definitions("-g -Wno-interference-size")

add_subdirectory(unit_tests)
target_link_libraries(rate_limiter_tests PRIVATE gtest_main gmock_main)

target_include_directories(rate_limiter_tests PUBLIC ${PROJECT_SOURCE_DIR})

enable_testing()
