FILE(GLOB LIMITER_LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(limiter_lib STATIC ${LIMITER_LIB_SRC})

target_include_directories(limiter_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(limiter_lib PUBLIC cxx_std_23)
target_compile_options(limiter_lib PRIVATE -Wno-interference-size $<$<CONFIG:Debug>:-g>)
