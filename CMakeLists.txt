##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#




# ESP-IDF component
if(ESP_PLATFORM)
    idf_component_register(SRCS              "api/fft.c" "api/k2fft.c" "api/spectrum.c" "kernel/kernels.c" "kernel/planner.c"
                                             "kernel/problem.c" "kernel/race.c" "kernel/pow.c"
                                             "rdft/espdsp.c" "rdft/pocketfft.c" "rdft/pffft.c" "rdft/trig.c"
                                             "libs/pffft/src/pffft.c" "libs/pffft/src/pffft_common.c"
                           INCLUDE_DIRS      "api"
                           PRIV_INCLUDE_DIRS "." "libs/pffft/include" "libs/pffft/include/pffft" "libs/pffft/pocketfft"
                           PRIV_REQUIRES     espressif__esp-dsp
    )
    target_compile_definitions(${COMPONENT_LIB} PRIVATE KERNEL_espdsp KERNEL_pocketfft KERNEL_trig K2FFT_LARGEST=4096 PFFFT_SCALVEC_ENABLED=1 _USE_MATH_DEFINES) # 4096: support/esp32/count's sizes
    set_source_files_properties(rdft/pocketfft.c PROPERTIES COMPILE_OPTIONS -Wno-error) # pocketfft's own code, under ESP-IDF's -Werror=all
    return()
endif()





##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#


cmake_minimum_required(VERSION 3.21) # for PROJECT_IS_TOP_LEVEL
project(K2FFT VERSION 0.1.0 LANGUAGES C CXX)
if(PROJECT_IS_TOP_LEVEL)
    if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE Release)
    endif()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
option(K2FFT_BENCH "Benchmarks + Checks" ${PROJECT_IS_TOP_LEVEL})
option(K2FFT_INSTALL "the install rules (cmake --install)" ${PROJECT_IS_TOP_LEVEL})
set(BUILD_SHARED_LIBS OFF) # FFT libraries are linked into each external
set(K2FFT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")


##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#




# architecture
include(CheckCSourceCompiles)
function(k2fft_arch var condition)
    unset(${var} CACHE) # CMAKE_OSX_ARCHITECTURES may have changed
    check_c_source_compiles("#if !(${condition})\n#error\n#endif\nint main(void){ return 0; }" ${var})
endfunction()
k2fft_arch(X86_64  "defined(__x86_64__) || defined(_M_X64)")
k2fft_arch(ARM32   "defined(__arm__)")
k2fft_arch(ARM64   "defined(__aarch64__)")
k2fft_arch(RISCV64 "defined(__riscv) && __riscv_xlen == 64")
k2fft_arch(HIGHWAY "defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86) || defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(__riscv) || defined(__powerpc64__) || defined(__s390x__)") # Highway's, each slice of a universal build





##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#


# kernels: rdft/<name>.c, one library each, the candidates this platform builds; kernel/table.h (scripts/table) ranks them by size
include(CheckLibraryExists)
check_library_exists(m sqrt "" K2FFT_HAVE_LIBM)
find_package(Threads)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/pffft.cmake")
k2fft_arch(HELIUM "defined(__ARM_FEATURE_MVE)")
set(K2FFT_LARGEST 65536) # the largest size, which the benchmark covers (cmake/bench.cmake)
if(CMAKE_SYSTEM_NAME STREQUAL "Generic")
    set(K2FFT_LARGEST 16384) # what Cortex-M's 2 MB boards hold, benchmark and all
endif()
set(K2FFT_KERNELS "")
set(K2FFT_SOURCES "")
set(K2FFT_LIBRARIES "")
set(K2FFT_INCLUDES "")
# k2fft_kernel(<name> [SOURCES ...] [LIBRARIES ...] [INCLUDES ...]): SOURCES default to rdft/<name>.c
macro(k2fft_kernel name)
    cmake_parse_arguments(kernel "" "" "SOURCES;LIBRARIES;INCLUDES" ${ARGN})
    if(NOT kernel_SOURCES)
        set(kernel_SOURCES rdft/${name}.c)
    endif()
    list(APPEND K2FFT_KERNELS ${name})
    list(APPEND K2FFT_SOURCES ${kernel_SOURCES})
    list(APPEND K2FFT_LIBRARIES ${kernel_LIBRARIES})
    list(APPEND K2FFT_INCLUDES ${kernel_INCLUDES})
endmacro()
if(X86_64 AND ((WIN32 AND NOT MINGW) OR CMAKE_SYSTEM_NAME STREQUAL "Linux")) # IPP's and oneMKL's archives are MSVC's, which MinGW can't link
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ipp.cmake")
    k2fft_kernel(ipp LIBRARIES ipp_signal)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/mkl.cmake")
    k2fft_kernel(mkl LIBRARIES mkl_dft)
endif()
if(X86_64 AND ((CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID STREQUAL "GNU") OR MSVC)) # AMD's FFTW; its CMake's Clang path adds -march=znver*
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/aocl.cmake")
    k2fft_kernel(aocl LIBRARIES aocl_fftw3f)
endif()
if(ARM64 AND CMAKE_SYSTEM_NAME STREQUAL "Linux") # Arm's
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/armpl.cmake")
    k2fft_kernel(armpl LIBRARIES armpl_fft)
endif()
if(APPLE)
    k2fft_kernel(vdsp LIBRARIES "-framework Accelerate")
endif()
if((X86_64 AND NOT MINGW) OR (ARM64 AND NOT MSVC)) # KFR's ARM needs GCC or Clang; MinGW's GCC misaligns AVX spills
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/kfr.cmake")
    k2fft_kernel(kfr LIBRARIES kfr_rfft)
endif()
if((ARM32 OR ARM64) AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ne10.cmake")
    if(NE10_OK)
        k2fft_kernel(ne10 LIBRARIES ne10_fft)
    endif()
endif()
if(HELIUM AND CMAKE_SYSTEM_NAME STREQUAL "Generic")
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmsis.cmake")
    k2fft_kernel(cmsis LIBRARIES cmsis_dsp_rfft)
endif()
k2fft_kernel(trig) # every platform's: FFTW's 1D r2r kinds over the fastest FFT here (rdft/trig.c)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Generic") # every OS's
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/fftw.cmake")
    k2fft_kernel(fftw3 LIBRARIES fftw3f fftw3)
endif()
if(X86_64 OR (ARM64 AND CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/zldsp.cmake")
    if(TARGET zldsp_rfft)
        k2fft_kernel(zldsp LIBRARIES zldsp_rfft)
    endif()
endif()
if(X86_64)
    pffft_simd_copy(pffft_avx2 avx2_fma)
elseif(ARM32 AND CMAKE_SYSTEM_NAME STREQUAL "Linux") # AArch64's own PFFFT is NEON already
    pffft_simd_copy(pffft_neon neon DEFINITIONS PFFFT_ENABLE_NEON=1)
elseif(RISCV64 AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND ((CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 14)
                                                           OR (CMAKE_C_COMPILER_ID STREQUAL "Clang" AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 18))) # GCC 13 makes PFFFT's RVV code scalar
    pffft_simd_copy(pffft_rvv rvv DEFINITIONS PFFFT_ENABLE_RVV=1)
endif()
foreach(copy pffft_avx2 pffft_neon pffft_rvv)
    if(TARGET ${copy})
        k2fft_kernel(${copy} LIBRARIES ${copy})
    endif()
endforeach()
if(HIGHWAY AND NOT CMAKE_SYSTEM_NAME STREQUAL "Generic") # every OS's, on Highway's dispatch
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/hwy.cmake")
    k2fft_kernel(hwyfft SOURCES rdft/hwyfft.cpp LIBRARIES hwy::hwy)
endif()
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Generic") # every OS's: its scratch is thread-local
    k2fft_kernel(pocketfft INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/libs/pffft/pocketfft")
endif()
k2fft_kernel(pffft LIBRARIES PFFFT::PFFFT) # every platform's, and its fallback
if(Threads_FOUND AND NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "Generic")
    list(APPEND K2FFT_LIBRARIES Threads::Threads) # race.c's pthread lock
endif()
if(K2FFT_HAVE_LIBM)
    list(APPEND K2FFT_LIBRARIES m)
endif()





##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#


# library
# k2fft_library(<name> [DEFINITIONS ...]): the planner on this platform's kernels
function(k2fft_library name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "DEFINITIONS")
    list(TRANSFORM K2FFT_SOURCES PREPEND "${K2FFT_DIR}/" OUTPUT_VARIABLE kernels)
    list(TRANSFORM K2FFT_KERNELS PREPEND KERNEL_ OUTPUT_VARIABLE built) # kernel/kernels.c's
    add_library(${name} STATIC 	"${K2FFT_DIR}/api/fft.c" 
								"${K2FFT_DIR}/api/k2fft.c" 
								"${K2FFT_DIR}/api/spectrum.c" 
								"${K2FFT_DIR}/kernel/kernels.c" 
								"${K2FFT_DIR}/kernel/planner.c" 
								"${K2FFT_DIR}/kernel/problem.c" 
								"${K2FFT_DIR}/kernel/race.c" 
								"${K2FFT_DIR}/kernel/pow.c"
                               	"${K2FFT_DIR}/simd-support/simd.c" 
								${kernels} 
	)
    target_include_directories(${name} 	PUBLIC 	"$<BUILD_INTERFACE:${K2FFT_DIR}/api>" 
										PRIVATE "${K2FFT_DIR}" ${K2FFT_INCLUDES})
    target_compile_definitions(${name} PRIVATE ${built} K2FFT_LARGEST=${K2FFT_LARGEST} ${arg_DEFINITIONS})
    target_link_libraries(${name} PRIVATE ${K2FFT_LIBRARIES})
    set_target_properties(${name} PROPERTIES 	POSITION_INDEPENDENT_CODE ON 
												C_STANDARD 99 
												C_EXTENSIONS OFF 
												CXX_STANDARD 20) # KFR 7 needs C++20
endfunction()
k2fft_library(K2FFT)
message(STATUS "K2FFT's kernels: ${K2FFT_KERNELS}")
add_library(K2FFT::K2FFT ALIAS K2FFT)





##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

# K2FFT.pc, the CMake package and the install rules
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/package.cmake")

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#





# Benchmarks + Checks
if(K2FFT_BENCH)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/bench.cmake")
    add_subdirectory(libbench2)
    add_subdirectory(tests)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Generic") # bare metal calls main() w/o args
        add_subdirectory(tools)
    endif()
    add_subdirectory(benchees)
endif()
