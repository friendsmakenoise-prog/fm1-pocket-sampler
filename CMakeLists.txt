cmake_minimum_required(VERSION 3.16)
project(fm1_pocket_sampler VERSION 0.2.7 LANGUAGES CXX)

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
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
)

add_executable(fm1_tests tests/test_sampler.cpp)
target_link_libraries(fm1_tests PRIVATE fm1_core)
add_test(NAME fm1_tests COMMAND fm1_tests)

option(FM1_BUILD_DESKTOP "Build the graphical desktop sampler" ON)

if(FM1_BUILD_DESKTOP)
    include(FetchContent)

    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(raylib)

    if(APPLE)
        add_executable(fm1_desktop MACOSX_BUNDLE desktop/src/main.cpp)
        set_target_properties(fm1_desktop PROPERTIES
            MACOSX_BUNDLE_BUNDLE_NAME "FM-1 B-Boy Edition"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.friendsmakenoise.fm1bboyedition"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "0.2.7"
            MACOSX_BUNDLE_BUNDLE_VERSION "0.2.7"
        )
    else()
        add_executable(fm1_desktop desktop/src/main.cpp)
    endif()

    target_link_libraries(fm1_desktop PRIVATE fm1_core raylib)
    target_compile_definitions(fm1_desktop PRIVATE FM1_VERSION="0.2.7")
endif()
