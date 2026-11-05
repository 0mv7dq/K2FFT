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
file(DOWNLOAD ${KFR_URL} "${KFR_DIR}/kfr.tar.gz" EXPECTED_HASH SHA256=${KFR_SHA256})
file(ARCHIVE_EXTRACT INPUT "${KFR_DIR}/kfr.tar.gz" DESTINATION "${KFR_DIR}")
set(KFR_ENABLE_DFT ON CACHE BOOL "")
set(KFR_ENABLE_DSP OFF CACHE BOOL "")
set(KFR_ENABLE_AUDIO OFF CACHE BOOL "")
set(KFR_ENABLE_IO OFF CACHE BOOL "")
set(KFR_ENABLE_CAPI_BUILD OFF CACHE BOOL "")
set(KFR_INSTALL_HEADERS OFF CACHE BOOL "")
set(KFR_INSTALL_LICENSE OFF CACHE BOOL "")
set(KFR_INSTALL_LIBRARIES OFF CACHE BOOL "")
set(KFR_ARCH sse2 CACHE STRING "")
set(KFR_ENABLE_MULTIARCH ON CACHE BOOL "")
# KFR rejects sse2 unless CMAKE_OSX_ARCHITECTURES names x86_64
if(NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES x86_64)
endif()
add_subdirectory("${KFR_DIR}/kfr-${KFR_VERSION}" "${KFR_DIR}/build" EXCLUDE_FROM_ALL)

# each DFT file needs a lot of memory to compile
get_property(pools GLOBAL PROPERTY JOB_POOLS)
if(NOT pools AND CMAKE_JOB_POOLS) # the property overrides CMAKE_JOB_POOLS
    set_property(GLOBAL PROPERTY JOB_POOLS ${CMAKE_JOB_POOLS})
endif()
set_property(GLOBAL APPEND PROPERTY JOB_POOLS kfr_dft=2)
get_property(KFR_DFT_TARGETS DIRECTORY "${KFR_DIR}/kfr-${KFR_VERSION}/src/dft" PROPERTY BUILDSYSTEM_TARGETS)
set_target_properties(${KFR_DFT_TARGETS} PROPERTIES JOB_POOL_COMPILE kfr_dft)

# for the sizes that aren't powers of two
pffft_avx2_copy()
