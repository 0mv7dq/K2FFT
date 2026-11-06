##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

include_guard(GLOBAL)
# zldsp_fft (headers on Highway): zldsp_fft for benchees/zldsp, zldsp_rfft for rdft/zldsp.c
set(ZLDSP_COMMIT 73e96f1c05b8d8846b8a514bdb6e0931e55f3693)
set(ZLDSP_URL https://github.com/ZL-Audio/zldsp_fft/archive/${ZLDSP_COMMIT}.tar.gz)
set(ZLDSP_SHA256 0690dc7c22315820452d00a5b7c12712255a42e3789118669631ae92bd11fc88)
set(HWY_COMMIT 2607d3b5b0113992fe84d3848859eae13b3b52c1)
set(HWY_URL https://github.com/google/highway/archive/${HWY_COMMIT}.tar.gz)
set(HWY_SHA256 741d705781e0b3e406beda8f1f994fbae01321237ce8023a1ad90fbaf7940c25)
set(ZLDSP_DIR "${PROJECT_BINARY_DIR}/zldsp")
file(DOWNLOAD ${ZLDSP_URL} "${ZLDSP_DIR}/zldsp.tar.gz" INACTIVITY_TIMEOUT 60 EXPECTED_HASH SHA256=${ZLDSP_SHA256})
file(DOWNLOAD ${HWY_URL} "${ZLDSP_DIR}/highway.tar.gz" INACTIVITY_TIMEOUT 60 EXPECTED_HASH SHA256=${HWY_SHA256})
file(ARCHIVE_EXTRACT INPUT "${ZLDSP_DIR}/zldsp.tar.gz" DESTINATION "${ZLDSP_DIR}" PATTERNS "*/src/*" "*/LICENSE.md")
file(ARCHIVE_EXTRACT INPUT "${ZLDSP_DIR}/highway.tar.gz" DESTINATION "${ZLDSP_DIR}")
foreach(option HWY_ENABLE_TESTS HWY_ENABLE_EXAMPLES HWY_ENABLE_CONTRIB HWY_ENABLE_INSTALL)
    set(${option} OFF CACHE BOOL "")
endforeach()
set(HWY_FORCE_STATIC_LIBS ON CACHE BOOL "") # whatever the consumer's BUILD_SHARED_LIBS
add_subdirectory("${ZLDSP_DIR}/highway-${HWY_COMMIT}" "${ZLDSP_DIR}/highway" EXCLUDE_FROM_ALL)
add_library(zldsp_fft INTERFACE)
target_include_directories(zldsp_fft SYSTEM INTERFACE "${ZLDSP_DIR}/zldsp_fft-${ZLDSP_COMMIT}/src")
target_link_libraries(zldsp_fft INTERFACE hwy)

# Highway's one target, as benchees/zldsp: AVX2 and the features HWY_AVX2 needs on x86-64, the baseline's NEON on 64-bit ARM
set(ZLDSP_FLAGS "")
if(X86_64)
    simd_flags(ZLDSP_FLAGS avx2_fma)
    if(NOT ZLDSP_FLAGS)
        message(STATUS "no AVX2 + FMA flags build with this compiler: zldsp is left out")
        return()
    endif()
    if(NOT MSVC)
        list(APPEND ZLDSP_FLAGS -mbmi2 -mf16c -mpclmul -maes)
    endif()
endif()
add_library(zldsp_rfft STATIC EXCLUDE_FROM_ALL "${CMAKE_CURRENT_LIST_DIR}/../rdft/zldsp_rfft.cpp")
target_compile_definitions(zldsp_rfft PRIVATE HWY_COMPILE_ONLY_STATIC)
target_compile_options(zldsp_rfft PRIVATE ${ZLDSP_FLAGS})
target_link_libraries(zldsp_rfft PRIVATE zldsp_fft)
set_target_properties(zldsp_rfft PROPERTIES CXX_STANDARD 20 POSITION_INDEPENDENT_CODE ON) # zldsp's std::numbers and std::bit_width
