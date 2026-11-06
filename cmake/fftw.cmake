##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

include_guard(GLOBAL)
set(FFTW_RELEASE 3.3.10)
set(FFTW_URL https://www.fftw.org/fftw-${FFTW_RELEASE}.tar.gz)
set(FFTW_SHA256 56c932549852cddcfafdab3820b0200c7742675be92179e59e6215b340e26467)
set(FFTW_DIR "${PROJECT_BINARY_DIR}/fftw")
file(DOWNLOAD ${FFTW_URL} "${FFTW_DIR}/fftw.tar.gz" EXPECTED_HASH SHA256=${FFTW_SHA256})
file(ARCHIVE_EXTRACT INPUT "${FFTW_DIR}/fftw.tar.gz" DESTINATION "${FFTW_DIR}")
# the options stay out of the cache
set(fftw_options BUILD_TESTS ENABLE_OPENMP ENABLE_THREADS WITH_COMBINED_THREADS ENABLE_LONG_DOUBLE ENABLE_QUAD_PRECISION ENABLE_SSE ENABLE_SSE2
                 ENABLE_AVX ENABLE_AVX2)
foreach(option ${fftw_options})
    set(${option} OFF)
endforeach()
set(fftw_simd "")
if(X86_64)
    set(fftw_simd ENABLE_SSE2 ENABLE_AVX ENABLE_AVX2) # Homebrew's and Debian's, which benchees/fftw3 measures
endif()
foreach(option ${fftw_simd})
    set(${option} ON)
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

# FFTW's CMake builds every file with the SIMD flags; as its configure, only the codelets get them, which FFTW runs after its CPUID checks
if(fftw_simd)
    set(src "${FFTW_DIR}/fftw-${FFTW_RELEASE}")
    get_target_property(options fftw3f COMPILE_OPTIONS)
    list(REMOVE_ITEM options -msse2 -mavx -mavx2 -mfma)
    set_target_properties(fftw3f PROPERTIES COMPILE_OPTIONS "${options}")
    file(GLOB sse2 "${src}/dft/simd/sse2/*.c" "${src}/rdft/simd/sse2/*.c")
    file(GLOB avx "${src}/dft/simd/avx/*.c" "${src}/rdft/simd/avx/*.c")
    file(GLOB avx2 "${src}/dft/simd/avx2/*.c" "${src}/dft/simd/avx2-128/*.c" "${src}/rdft/simd/avx2/*.c" "${src}/rdft/simd/avx2-128/*.c")
    set_source_files_properties(${sse2} DIRECTORY "${src}" PROPERTIES COMPILE_OPTIONS -msse2)
    set_source_files_properties(${avx} DIRECTORY "${src}" PROPERTIES COMPILE_OPTIONS -mavx)
    set_source_files_properties(${avx2} DIRECTORY "${src}" PROPERTIES COMPILE_OPTIONS "-mavx2;-mfma")
endif()
