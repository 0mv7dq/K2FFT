##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#




# ESP-IDF component
if(ESP_PLATFORM)
    idf_component_register(SRCS              "api/fft.c" "api/spectrum.c" "kernel/kernels.c" "kernel/race.c" "kernel/pow.c"
                                             "rdft/espdsp.c" "rdft/pocketfft.c" "rdft/pffft.c"
                                             "libs/pffft/src/pffft.c" "libs/pffft/src/pffft_common.c"
                           INCLUDE_DIRS      "api"
                           PRIV_INCLUDE_DIRS "." "libs/pffft/include" "libs/pffft/include/pffft" "libs/pffft/pocketfft"
                           PRIV_REQUIRES     espressif__esp-dsp
    )
    target_compile_definitions(${COMPONENT_LIB} PRIVATE KERNEL_espdsp KERNEL_pocketfft PFFFT_SCALVEC_ENABLED=1 _USE_MATH_DEFINES)
    set_source_files_properties(rdft/pocketfft.c PROPERTIES COMPILE_OPTIONS -Wno-error) # pocketfft's own code, under ESP-IDF's -Werror=all
    return()
endif()





##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#


cmake_minimum_required(VERSION 3.21) # for PROJECT_IS_TOP_LEVEL
project(PUREfft VERSION 0.1.0 LANGUAGES C CXX)
if(PROJECT_IS_TOP_LEVEL)
    if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE Release)
    endif()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
option(PUREFFT_BENCH "Benchmarks + Checks" ${PROJECT_IS_TOP_LEVEL})
option(PUREFFT_INSTALL "the install rules (cmake --install)" ${PROJECT_IS_TOP_LEVEL})
set(BUILD_SHARED_LIBS OFF) # FFT libraries are linked into each external
set(PUREFFT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")


##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#




# architecture
include(CheckCSourceCompiles)
function(purefft_arch var condition)
    unset(${var} CACHE) # CMAKE_OSX_ARCHITECTURES may have changed
    check_c_source_compiles("#if !(${condition})\n#error\n#endif\nint main(void){ return 0; }" ${var})
endfunction()
purefft_arch(X86_64  "defined(__x86_64__) || defined(_M_X64)")
purefft_arch(ARM32   "defined(__arm__)")
purefft_arch(ARM64   "defined(__aarch64__)")
purefft_arch(RISCV64 "defined(__riscv) && __riscv_xlen == 64")





##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#


# kernels: rdft/<name>.c, one library each, the candidates this platform builds; kernel/table.h (scripts/table) ranks them by size
include(CheckLibraryExists)
check_library_exists(m sqrt "" PUREFFT_HAVE_LIBM)
find_package(Threads)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/pffft.cmake")
purefft_arch(HELIUM "defined(__ARM_FEATURE_MVE)")
set(PUREFFT_KERNELS "")
set(PUREFFT_SOURCES "")
set(PUREFFT_LIBRARIES "")
set(PUREFFT_INCLUDES "")
# purefft_kernel(<name> [SOURCES ...] [LIBRARIES ...] [INCLUDES ...]): SOURCES default to rdft/<name>.c
macro(purefft_kernel name)
    cmake_parse_arguments(kernel "" "" "SOURCES;LIBRARIES;INCLUDES" ${ARGN})
    if(NOT kernel_SOURCES)
        set(kernel_SOURCES rdft/${name}.c)
    endif()
    list(APPEND PUREFFT_KERNELS ${name})
    list(APPEND PUREFFT_SOURCES ${kernel_SOURCES})
    list(APPEND PUREFFT_LIBRARIES ${kernel_LIBRARIES})
    list(APPEND PUREFFT_INCLUDES ${kernel_INCLUDES})
endmacro()
if(X86_64 AND ((WIN32 AND NOT MINGW) OR CMAKE_SYSTEM_NAME STREQUAL "Linux")) # IPP's archives are MSVC's, which MinGW can't link
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ipp.cmake")
    purefft_kernel(ipp LIBRARIES ipp_signal)
endif()
if(APPLE)
    purefft_kernel(vdsp LIBRARIES "-framework Accelerate")
endif()
if(X86_64 AND APPLE)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/kfr.cmake")
    purefft_kernel(kfr SOURCES rdft/kfr.cpp LIBRARIES kfr_dft)
endif()
if((ARM32 OR ARM64) AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ne10.cmake")
    if(NE10_OK)
        purefft_kernel(ne10 LIBRARIES ne10_fft)
    endif()
endif()
if(HELIUM AND CMAKE_SYSTEM_NAME STREQUAL "Generic")
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmsis.cmake")
    purefft_kernel(cmsis LIBRARIES cmsis_dsp_rfft)
endif()
if((X86_64 AND APPLE) OR (RISCV64 AND CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/fftw.cmake")
    purefft_kernel(fftw3 LIBRARIES fftw3f)
endif()
if(X86_64 OR (ARM64 AND CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/zldsp.cmake")
    if(TARGET zldsp_rfft)
        purefft_kernel(zldsp LIBRARIES zldsp_rfft)
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
        purefft_kernel(${copy} LIBRARIES ${copy})
    endif()
endforeach()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT X86_64 AND NOT ARM64) # 32-bit ARM, RISC-V and the scalar ISAs, where benchmark/ ranks it
    purefft_kernel(pocketfft INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/libs/pffft/pocketfft")
endif()
purefft_kernel(pffft LIBRARIES PFFFT::PFFFT) # every platform's, and its fallback
if(Threads_FOUND AND NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "Generic")
    list(APPEND PUREFFT_LIBRARIES Threads::Threads) # race.c's pthread lock
endif()
if(PUREFFT_HAVE_LIBM)
    list(APPEND PUREFFT_LIBRARIES m)
endif()





##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#


# library
# purefft_library(<name> [DEFINITIONS ...]): the planner on this platform's kernels
function(purefft_library name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "DEFINITIONS")
    list(TRANSFORM PUREFFT_SOURCES PREPEND "${PUREFFT_DIR}/" OUTPUT_VARIABLE kernels)
    list(TRANSFORM PUREFFT_KERNELS PREPEND KERNEL_ OUTPUT_VARIABLE built) # kernel/kernels.c's
    add_library(${name} STATIC 	"${PUREFFT_DIR}/api/fft.c" 
								"${PUREFFT_DIR}/api/spectrum.c" 
								"${PUREFFT_DIR}/kernel/kernels.c" 
								"${PUREFFT_DIR}/kernel/race.c" 
								"${PUREFFT_DIR}/kernel/pow.c"
                               	"${PUREFFT_DIR}/simd-support/simd.c" 
								${kernels} 
	)
    target_include_directories(${name} 	PUBLIC 	"$<BUILD_INTERFACE:${PUREFFT_DIR}/api>" 
										PRIVATE "${PUREFFT_DIR}" ${PUREFFT_INCLUDES})
    target_compile_definitions(${name} PRIVATE ${built} ${arg_DEFINITIONS})
    target_link_libraries(${name} PRIVATE ${PUREFFT_LIBRARIES})
    set_target_properties(${name} PROPERTIES 	POSITION_INDEPENDENT_CODE ON 
												C_STANDARD 99 
												C_EXTENSIONS OFF 
												CXX_STANDARD 20) # KFR 7 needs C++20
endfunction()
purefft_library(PUREfft)
message(STATUS "PUREfft's kernels: ${PUREFFT_KERNELS}")
add_library(PUREfft::PUREfft ALIAS PUREfft)





##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

# PUREfft.pc, the CMake package and the install rules
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/package.cmake")

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#





# Benchmarks + Checks
if(PUREFFT_BENCH)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/bench.cmake")
    add_subdirectory(libbench2)
    add_subdirectory(tests)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Generic") # bare metal calls main() w/o args
        add_subdirectory(tools)
    endif()
    add_subdirectory(benchees)
endif()
