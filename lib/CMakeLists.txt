FILE(GLOB LIMITER_LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB LIMITER_ALG_SRC ${CMAKE_CURRENT_SOURCE_DIR}/algorithms/*.cpp)

add_library(limiter_lib STATIC ${LIMITER_LIB_SRC} ${LIMITER_ALG_SRC})
add_definitions("-g -Wno-interference-size")
target_include_directories(limiter_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})