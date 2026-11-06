##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

include_guard(GLOBAL)
set(FFTW_RELEASE 3.3.11)
set(FFTW_URL https://www.fftw.org/fftw-${FFTW_RELEASE}.tar.gz)
set(FFTW_SHA256 5630c24cdeb33b131612f7eb4b1a9934234754f9f388ff8617458d0be6f239a1)
set(FFTW_DIR "${PROJECT_BINARY_DIR}/fftw")
file(DOWNLOAD ${FFTW_URL} "${FFTW_DIR}/fftw.tar.gz" EXPECTED_HASH SHA256=${FFTW_SHA256})
file(ARCHIVE_EXTRACT INPUT "${FFTW_DIR}/fftw.tar.gz" DESTINATION "${FFTW_DIR}")
# the options stay out of the cache
set(fftw_options BUILD_TESTS ENABLE_OPENMP ENABLE_THREADS WITH_COMBINED_THREADS ENABLE_LONG_DOUBLE ENABLE_QUAD_PRECISION ENABLE_SSE ENABLE_SSE2
                 ENABLE_AVX ENABLE_AVX2)
foreach(option ${fftw_options})
    set(${option} OFF)
endforeach()
set(ENABLE_FLOAT ON)
set(DISABLE_FORTRAN ON)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW) # option() honours normal variables
set(CMAKE_POLICY_VERSION_MINIMUM 3.5) # FFTW asks for CMake 3.0, which CMake 4 refuses
add_subdirectory("${FFTW_DIR}/fftw-${FFTW_RELEASE}" "${FFTW_DIR}/build" EXCLUDE_FROM_ALL)
unset(CMAKE_POLICY_VERSION_MINIMUM)
unset(CMAKE_POLICY_DEFAULT_CMP0077)
foreach(option ${fftw_options} ENABLE_FLOAT DISABLE_FORTRAN)
    unset(${option})
endforeach()
target_compile_options(fftw3f PRIVATE $<$<C_COMPILER_ID:GNU>:-fno-schedule-insns>) # GCC's early scheduling makes the codelets spill
target_include_directories(fftw3f SYSTEM INTERFACE "$<BUILD_INTERFACE:${FFTW_DIR}/fftw-${FFTW_RELEASE}/api>")
set_target_properties(fftw3f PROPERTIES C_EXTENSIONS ON POSITION_INDEPENDENT_CODE ON) # ISO C stops GCC fusing multiply-adds

# PFFFT for RVV, raced against FFTW
simd_flags(RVV_FLAGS rvv)
add_library(pffft_rvv STATIC "${CMAKE_CURRENT_LIST_DIR}/../libs/pffft/src/pffft.c" "${CMAKE_CURRENT_LIST_DIR}/../libs/pffft/src/pffft_common.c")
if(RVV_FLAGS)
    set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/../libs/pffft/src/pffft.c" PROPERTIES COMPILE_OPTIONS "${RVV_FLAGS}")
    target_compile_definitions(pffft_rvv PRIVATE PFFFT_ENABLE_RVV=1)
endif()
target_compile_definitions(pffft_rvv PRIVATE _USE_MATH_DEFINES)
target_include_directories(pffft_rvv PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../libs/pffft/include")
target_include_directories(pffft_rvv PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../libs/pffft/include/pffft")
set_target_properties(pffft_rvv PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(RVV_FLAGS AND ((CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 14) OR (CMAKE_C_COMPILER_ID STREQUAL "Clang" AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 18)))
    target_compile_definitions(pffft_rvv INTERFACE PFFFT_RVV) # GCC 13 makes PFFFT's RVV code scalar
endif()
