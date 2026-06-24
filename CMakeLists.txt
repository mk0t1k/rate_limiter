cmake_minimum_required(VERSION 3.13)

project(
    rate_limiter
    LANGUAGES CXX
)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose Debug or Release" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(lib)

FILE(GLOB RATE_LIMITER_APP_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_executable(${PROJECT_NAME} ${RATE_LIMITER_APP_SRC})

target_link_libraries(${PROJECT_NAME} limiter_lib)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-interference-size $<$<CONFIG:Debug>:-g>)

add_subdirectory(unit_tests)
target_link_libraries(rate_limiter_tests PRIVATE limiter_lib gtest_main gmock_main)
target_compile_options(rate_limiter_tests PRIVATE -Wno-interference-size $<$<CONFIG:Debug>:-g>)

target_include_directories(rate_limiter_tests PUBLIC ${PROJECT_SOURCE_DIR})


add_subdirectory(benchmark)
enable_testing()
