cmake_minimum_required(VERSION 3.13)
project(tiledjinn)

set(CMAKE_CXX_STANDARD 99)
set(CMAKE_C_STANDARD 99)

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)

set(CMAKE_C_FLAGS -DLIB_EXPORTS)

file(GLOB SRC
        "src/*.h"
        "src/*.c"
        "third_party/*.h"
        "third_party/*.c"
        )


include_directories("include")
include_directories("third_party")
include_directories("lib/SDL2-2.0.22/include")
include_directories("${PROJECT_SOURCE_DIR}/lib/libpng/include")
include_directories("${PROJECT_SOURCE_DIR}/lib/zlib/include")

add_library(tiledjinn STATIC ${SRC})

target_link_directories(tiledjinn PUBLIC ${PROJECT_SOURCE_DIR}/lib/libpng/lib)
target_link_libraries(tiledjinn libpng16_static)

target_link_directories(tiledjinn PUBLIC ${PROJECT_SOURCE_DIR}/lib/zlib/lib)
target_link_libraries(tiledjinn zlibstatic)

target_link_directories(tiledjinn PUBLIC ${PROJECT_SOURCE_DIR}/SDL2-2.0.22/lib/x64)
target_link_libraries(tiledjinn SDL2)
