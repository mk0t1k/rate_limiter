FILE(GLOB LIMITER_LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(limiter_lib STATIC ${LIMITER_LIB_SRC})
add_definitions("-g -Wno-interference-size")