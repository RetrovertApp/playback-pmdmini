# PMDmini Playback Plugin
#
# This plugin plays NEC PC-98 PMD music files using the pmdmini library.
# PMD uses YM2608 (OPNA) FM synthesis. The engine has global state so only
# one file can be open at a time (fine since only one song plays at a time).
# Files: .m, .m2

cmake_minimum_required(VERSION 3.16)
project(playback_pmdmini C CXX)

include(CTest)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(PlaybackPlugin)

# Download pmdmini v2.0.0
download_library(
    NAME pmdmini
    URL "https://github.com/mistydemeo/pmdmini/archive/refs/tags/v2.0.0.tar.gz"
    FILENAME "pmdmini-2.0.0.tar.gz"
    HASH "e3288dcf356e83ef4ad48cde44fcb703ca9ce478b9fcac1b44bd9d2d84bf2ba3"
    EXTRACT_DIR "pmdmini-2.0.0"
    PATCHES patches/pmdmini-disable-x86asm.patch patches/pmdmini-packed-clang.patch patches/pmdmini-narrowing-fix.patch patches/pmdmini-pointer-compare-fix.patch patches/scope-capture.patch patches/pmdmini-rvio.patch
)

# C wrapper
set(PMDMINI_C_SOURCES ${pmdmini_SOURCE_DIR}/src/pmdmini.c)
set(PMDMINI_IO_SOURCE ${pmdmini_SOURCE_DIR}/src/pmd_io.cpp)

# C++ engine
file(GLOB PMDMINI_FMGEN_SOURCES ${pmdmini_SOURCE_DIR}/src/fmgen/*.cpp)
file(GLOB PMDMINI_PMDWIN_SOURCES ${pmdmini_SOURCE_DIR}/src/pmdwin/*.cpp)

add_library(pmdmini_static STATIC
    ${PMDMINI_C_SOURCES}
    ${PMDMINI_IO_SOURCE}
    ${PMDMINI_FMGEN_SOURCES}
    ${PMDMINI_PMDWIN_SOURCES}
)

target_include_directories(pmdmini_static PUBLIC
    ${pmdmini_SOURCE_DIR}/src
    ${pmdmini_SOURCE_DIR}/src/pmdwin
    ${pmdmini_SOURCE_DIR}/src/fmgen
)

# Route every upstream stdio read through the callback-backed file layer. The
# implementation itself retains access to libc stdio for non-Retrovert users.
set_source_files_properties(${PMDMINI_IO_SOURCE} PROPERTIES COMPILE_DEFINITIONS PMD_IO_IMPLEMENTATION)
target_compile_definitions(pmdmini_static PRIVATE PMD_IO_ENABLED)
if(MSVC)
    target_compile_options(pmdmini_static PRIVATE "/FI${pmdmini_SOURCE_DIR}/src/pmd_io.h")
else()
    target_compile_options(pmdmini_static PRIVATE -include "${pmdmini_SOURCE_DIR}/src/pmd_io.h")
endif()

# GCC 15+ on Windows: _splitpath/_makepath conflict with stdlib declarations
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(pmdmini_static PRIVATE -fpermissive)
endif()
suppress_external_warnings(pmdmini_static)
set_target_properties(pmdmini_static PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 14
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

# Plugin shared library
add_library(pmdmini_playback SHARED
    pmdmini_plugin.c
)

target_include_directories(pmdmini_playback PRIVATE
    ${pmdmini_SOURCE_DIR}/src
)

target_link_libraries(pmdmini_playback PRIVATE pmdmini_static)
if(UNIX)
    # The plugin is C but pmdmini is C++: link with the C++ driver so the
    # C++ runtime follows the toolchain's static/dynamic choice instead of
    # an explicit -lstdc++ always DT_NEEDing the shared one. On Windows the
    # C++ runtime comes from MSVC's CRT automatically.
    set_target_properties(pmdmini_playback PROPERTIES LINKER_LANGUAGE CXX)
    target_link_libraries(pmdmini_playback PRIVATE m)
endif()

target_compile_options(pmdmini_playback PRIVATE
    -Wno-unused-parameter
)

rv_set_plugin_properties(pmdmini_playback)

if(BUILD_TESTING)
    add_executable(pmdmini_rvio_test tests/pmdmini_rvio_test.cpp)
    target_link_libraries(pmdmini_rvio_test PRIVATE pmdmini_static)
    add_test(NAME pmdmini_rvio_test COMMAND pmdmini_rvio_test)
endif()
