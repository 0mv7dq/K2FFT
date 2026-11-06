##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

include_guard(GLOBAL)
set(KFR_VERSION 7.2.0)
set(KFR_URL https://github.com/kfrlib/kfr/archive/refs/tags/${KFR_VERSION}.tar.gz)
set(KFR_SHA256 4f1603d5b6e609c25d25cff58fe7154a371583fd6d379615b342e44223be0763)
set(KFR_DIR "${PROJECT_BINARY_DIR}/kfr")
file(DOWNLOAD ${KFR_URL} "${KFR_DIR}/kfr.tar.gz" INACTIVITY_TIMEOUT 60 EXPECTED_HASH SHA256=${KFR_SHA256})
file(ARCHIVE_EXTRACT INPUT "${KFR_DIR}/kfr.tar.gz" DESTINATION "${KFR_DIR}")
set(KFR_ENABLE_DFT ON CACHE BOOL "")
set(KFR_ENABLE_DSP OFF CACHE BOOL "")
set(KFR_ENABLE_AUDIO OFF CACHE BOOL "")
set(KFR_ENABLE_IO OFF CACHE BOOL "")
set(KFR_ENABLE_CAPI_BUILD OFF CACHE BOOL "")
set(KFR_INSTALL_HEADERS OFF CACHE BOOL "")
set(KFR_INSTALL_LICENSE OFF CACHE BOOL "")
set(KFR_INSTALL_LIBRARIES OFF CACHE BOOL "")
if(X86_64)
    set(KFR_ARCH sse2 CACHE STRING "") # the baseline, w/ KFR's run-time dispatch to sse41, avx, avx2 and avx512
    set(KFR_ENABLE_MULTIARCH ON CACHE BOOL "")
    if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES x86_64) # KFR rejects sse2 unless CMAKE_OSX_ARCHITECTURES names x86_64
    endif()
else()
    set(KFR_ARCH neon64 CACHE STRING "") # AArch64's baseline; KFR dispatches on x86 alone
endif()
add_subdirectory("${KFR_DIR}/kfr-${KFR_VERSION}" "${KFR_DIR}/build" EXCLUDE_FROM_ALL)

# each DFT file needs a lot of memory to compile
get_property(pools GLOBAL PROPERTY JOB_POOLS)
if(NOT pools AND CMAKE_JOB_POOLS) # the property overrides CMAKE_JOB_POOLS
    set_property(GLOBAL PROPERTY JOB_POOLS ${CMAKE_JOB_POOLS})
endif()
set_property(GLOBAL APPEND PROPERTY JOB_POOLS kfr_dft=2)
get_property(KFR_DFT_TARGETS DIRECTORY "${KFR_DIR}/kfr-${KFR_VERSION}/src/dft" PROPERTY BUILDSYSTEM_TARGETS)
set_target_properties(${KFR_DFT_TARGETS} PROPERTIES JOB_POOL_COMPILE kfr_dft POSITION_INDEPENDENT_CODE ON) # PIC: they go into Pd's shared externals

# rdft/kfr_rfft.cpp alone, as KFR's GCC flags (-flax-vector-conversions) would reach K2FFT's every source, Highway's among them
add_library(kfr_rfft STATIC EXCLUDE_FROM_ALL "${CMAKE_CURRENT_LIST_DIR}/../rdft/kfr_rfft.cpp")
target_link_libraries(kfr_rfft PRIVATE kfr_dft)
set_target_properties(kfr_rfft PROPERTIES CXX_STANDARD 20 POSITION_INDEPENDENT_CODE ON) # KFR 7 needs C++20
