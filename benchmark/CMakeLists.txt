include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libs for benchmarks" FORCE)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark's own tests" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Do not build benchmark unit tests (no GTest)" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Do not install benchmark" FORCE)
set(BENCHMARK_FORCE_WERROR OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    googlebenchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.9.1.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)

FetchContent_MakeAvailable(googlebenchmark)

file(GLOB BENCHMARK_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*_bm.cpp")

set(RATE_LIMITER_LIB_SRC
    ${CMAKE_SOURCE_DIR}/src/interface.cpp
    ${CMAKE_SOURCE_DIR}/src/token_bucket.cpp
)

add_executable(rate_limiter_bench ${BENCHMARK_SOURCES} ${RATE_LIMITER_LIB_SRC})

target_link_libraries(
    rate_limiter_bench
    PRIVATE
        benchmark::benchmark_main
)

target_include_directories(rate_limiter_bench PRIVATE ${CMAKE_SOURCE_DIR}/src)
