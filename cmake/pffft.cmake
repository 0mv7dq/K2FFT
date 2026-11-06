##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/simd.cmake")
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/../libs/pffft/CMakeLists.txt")
    message(FATAL_ERROR "libs/pffft is empty: git submodule update --init --recursive")
endif()
set(PFFFT_USE_TYPE_DOUBLE OFF CACHE BOOL "")
set(INSTALL_PFFFT OFF CACHE BOOL "")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../libs/pffft" "${PROJECT_BINARY_DIR}/pffft" EXCLUDE_FROM_ALL)
set_target_properties(PFFFT PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(PUREFFT_HAVE_LIBM)
    target_link_libraries(PFFFT m) # sinf and cosf: PFFFT's find_library(m) misses a cross toolchain's
endif()

# PFFFT's CMake guesses NEON from the processor name
include(CheckCSourceCompiles)
unset(PFFFT_NEON_OK CACHE) # CMAKE_C_FLAGS may have changed
check_c_source_compiles("#if defined(__arm__) && !defined(__ARM_NEON)\n#error 32-bit ARM without NEON\n#endif\nint main(void){ return 0; }" PFFFT_NEON_OK)
if(PFFFT_NEON_OK)
    target_compile_definitions(PFFFT PRIVATE PFFFT_ENABLE_NEON=1)
endif()
# PFFFT's NEON check misses MSVC's _M_ARM64
if(CMAKE_C_COMPILER_ID STREQUAL "MSVC" AND CMAKE_C_COMPILER_ARCHITECTURE_ID STREQUAL "ARM64")
    target_compile_definitions(PFFFT PRIVATE __arm64__=1)
endif()

# a PFFFT for other SIMD flags, its functions renamed <prefix>_*
function(pffft_copy target prefix)
    set(dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../libs/pffft")
    set(functions new_setup destroy_setup transform transform_ordered transform_internal zreorder zconvolve zconvolve_accumulate zconvolve_no_accu zconvolve_scale zconvolve_zp zconvert_zp cplx_finalize cplx_preprocess is_valid_size min_fft_size nearest_transform_size simd_arch simd_size)
    list(TRANSFORM functions REPLACE "^(.+)$" "pffft_\\1=${prefix}_\\1" OUTPUT_VARIABLE names)
    add_library(${target} STATIC "${dir}/src/pffft.c")
    target_compile_definitions(${target} PRIVATE ${names} validate_pffft_simd=validate_${prefix}_simd validate_pffft_simd_ex=validate_${prefix}_simd_ex PFFFT_STATIC_DEFINE _USE_MATH_DEFINES)
    target_include_directories(${target} PRIVATE "${dir}/include" "${dir}/include/pffft")
    set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endfunction()
