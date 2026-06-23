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

add_executable(rate_limiter_bench ${CMAKE_CURRENT_SOURCE_DIR}/uniform_bm.cpp
                                  ${CMAKE_CURRENT_SOURCE_DIR}/diurnal_bm.cpp
                                  ${CMAKE_CURRENT_SOURCE_DIR}/sparse+burst_bm.cpp
                                  ${CMAKE_CURRENT_SOURCE_DIR}/burst_bm.cpp
                                  ${CMAKE_CURRENT_SOURCE_DIR}/zombie_bm.cpp
)

target_compile_features(rate_limiter_bench PRIVATE cxx_std_23)
target_compile_options(
    rate_limiter_bench
    PRIVATE
        -Wno-interference-size
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Debug>:-g>
)

target_link_libraries(
    rate_limiter_bench
    PRIVATE
        limiter_lib
        benchmark::benchmark_main
)
