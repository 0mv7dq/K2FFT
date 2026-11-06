##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

include_guard(GLOBAL)
# Highway, for rdft/hwyfft.cpp and zldsp: the including project's hwy::hwy if it has one (PARSHL's), else this commit's
set(HWY_COMMIT 2607d3b5b0113992fe84d3848859eae13b3b52c1)
set(HWY_URL https://github.com/google/highway/archive/${HWY_COMMIT}.tar.gz)
set(HWY_SHA256 741d705781e0b3e406beda8f1f994fbae01321237ce8023a1ad90fbaf7940c25)
if(NOT TARGET hwy::hwy)
    set(HWY_DIR "${PROJECT_BINARY_DIR}/highway")
    file(DOWNLOAD ${HWY_URL} "${HWY_DIR}/highway.tar.gz" INACTIVITY_TIMEOUT 60 EXPECTED_HASH SHA256=${HWY_SHA256})
    file(ARCHIVE_EXTRACT INPUT "${HWY_DIR}/highway.tar.gz" DESTINATION "${HWY_DIR}")
    foreach(option HWY_ENABLE_TESTS HWY_ENABLE_EXAMPLES HWY_ENABLE_CONTRIB HWY_ENABLE_INSTALL)
        set(${option} OFF CACHE BOOL "")
    endforeach()
    set(HWY_FORCE_STATIC_LIBS ON CACHE BOOL "") # whatever the consumer's BUILD_SHARED_LIBS
    add_subdirectory("${HWY_DIR}/highway-${HWY_COMMIT}" "${HWY_DIR}/build" EXCLUDE_FROM_ALL)
    set_target_properties(hwy PROPERTIES POSITION_INDEPENDENT_CODE ON) # its dispatch goes into Pd's shared externals
    if(NOT TARGET hwy::hwy)
        add_library(hwy::hwy ALIAS hwy)
    endif()
endif()
