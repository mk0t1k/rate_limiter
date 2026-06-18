include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libs for benchmarks" FORCE)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark's own tests" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Do not build benchmark unit tests (no GTest)" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Do not install benchmark" FORCE)
set(BENCHMARK_FORCE_WERROR OFF CACHE BOOL "" FORCE)

set(_googlebenchmark_saved_build_type "${CMAKE_BUILD_TYPE}")
set(CMAKE_BUILD_TYPE Release)

FetchContent_Declare(
    googlebenchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.9.1.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)

FetchContent_MakeAvailable(googlebenchmark)

if(_googlebenchmark_saved_build_type STREQUAL "")
  unset(CMAKE_BUILD_TYPE)
else()
  set(CMAKE_BUILD_TYPE "${_googlebenchmark_saved_build_type}")
endif()
unset(_googlebenchmark_saved_build_type)

set(RATE_LIMITER_BENCH_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/uniform_bm.cpp
)

set(RATE_LIMITER_LIB_SRC
    ${CMAKE_SOURCE_DIR}/src/token_bucket.cpp
    ${CMAKE_SOURCE_DIR}/src/sliding_win_log.cpp
    ${CMAKE_SOURCE_DIR}/src/aligned_alloc.cpp
)

add_executable(rate_limiter_bench ${RATE_LIMITER_BENCH_SRC} ${RATE_LIMITER_LIB_SRC})

target_compile_features(rate_limiter_bench PRIVATE cxx_std_23)
target_compile_options(rate_limiter_bench PRIVATE -O3 -Wno-interference-size -U_DEBUG)
target_compile_definitions(rate_limiter_bench PRIVATE NDEBUG _ITERATOR_DEBUG_LEVEL=0)
target_link_options(rate_limiter_bench PRIVATE -Xclang --dependent-lib=msvcrt)

target_link_libraries(
    rate_limiter_bench
    PRIVATE
        benchmark::benchmark_main
)

target_include_directories(rate_limiter_bench PRIVATE ${CMAKE_SOURCE_DIR}/src)
