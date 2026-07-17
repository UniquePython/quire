cmake_minimum_required(VERSION 3.20)

project(quire LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)

pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)
pkg_check_modules(FREETYPE REQUIRED IMPORTED_TARGET freetype2)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.c
)

add_executable(quire ${SOURCES})

set_target_properties(quire PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin"
)

target_include_directories(quire PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(quire PRIVATE
    PkgConfig::X11
    PkgConfig::FREETYPE
)

target_compile_options(quire PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Wsign-conversion
    -Wcast-qual
    -Wwrite-strings
    -Wformat=2
    -Wundef
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wold-style-definition
    -Wimplicit-fallthrough
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wcast-align
    -Wvla
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat-overflow=2
    -Wformat-truncation=2
    -Walloc-zero
    -Warray-bounds=2
    -Wstringop-overflow=4
)

target_compile_options(quire PRIVATE
    $<$<CONFIG:Debug>:
        -Og
        -g
        -fanalyzer
        -fstack-protector-strong
        -fno-omit-frame-pointer
        -fsanitize=address,undefined,leak
    >
)

target_link_options(quire PRIVATE
    $<$<CONFIG:Debug>:
        -fsanitize=address,undefined,leak
    >
)

target_compile_options(quire PRIVATE
    $<$<CONFIG:Release>:-O3>
)

target_compile_definitions(quire PRIVATE
    $<$<CONFIG:Release>:VERBOSITY=3>
)