include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/releases/download/v1.16.0/googletest-1.16.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

add_executable(
  rate_limiter_tests
  meta_ut.cpp
  token_bucket_ut.cpp
  sliding_win_log_ut.cpp
  leaking_bucket_ut.cpp
  ttl_ut.cpp
  sharded_storage_ut.cpp
  mutex_storage_ut.cpp
  Interface_ut.cpp
)

include(GoogleTest)
gtest_discover_tests(rate_limiter_tests)