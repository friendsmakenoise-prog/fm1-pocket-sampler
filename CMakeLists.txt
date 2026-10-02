cmake_minimum_required(VERSION 3.16)
project(fm1_pocket_sampler VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_library(fm1_core
    core/src/WavFile.cpp
    core/src/Sampler.cpp
)

target_include_directories(fm1_core PUBLIC core/include)

target_compile_options(fm1_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

add_executable(fm1_desktop desktop/src/main.cpp)
target_link_libraries(fm1_desktop PRIVATE fm1_core)

add_executable(fm1_tests tests/test_sampler.cpp)
target_link_libraries(fm1_tests PRIVATE fm1_core)
add_test(NAME fm1_tests COMMAND fm1_tests)
