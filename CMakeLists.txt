##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#




# ESP-IDF component
if(ESP_PLATFORM)
    idf_component_register(SRCS              "api/spectrum.c" "kernel/race.c" "kernel/pow.c" "rdft/z_espdsp.c"
                                             "libs/pffft/src/pffft.c" "libs/pffft/src/pffft_common.c"
                           INCLUDE_DIRS      "api"
                           PRIV_INCLUDE_DIRS "." "libs/pffft/include" "libs/pffft/include/pffft"
                           PRIV_REQUIRES     espressif__esp-dsp
    )
    target_compile_definitions(${COMPONENT_LIB} PRIVATE PFFFT_SCALVEC_ENABLED=1 _USE_MATH_DEFINES)
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


# backend
include(CheckLibraryExists)
check_library_exists(m sqrt "" PUREFFT_HAVE_LIBM)
find_package(Threads)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/pffft.cmake")
set(PUREFFT_RACE "") # names of the race's paths, 0 & 1
if(X86_64 AND ((WIN32 AND NOT MINGW) OR CMAKE_SYSTEM_NAME STREQUAL "Linux")) # IPP's archives are MSVC's, which MinGW can't link
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ipp.cmake")
    set(PUREFFT_BACKEND rdft/z_ipp.c)
    set(PUREFFT_LIBRARIES ipp_signal pffft_avx2 PFFFT::PFFFT)
    set(PUREFFT_RACE dft pffft)
elseif(X86_64 AND APPLE)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/kfr.cmake")
    set(PUREFFT_BACKEND rdft/z_kfr.cpp)
    set(PUREFFT_LIBRARIES kfr_dft pffft_avx2 PFFFT::PFFFT "-framework Accelerate")
    set(PUREFFT_RACE kfr vdsp)
elseif(APPLE) # ARM64
    set(PUREFFT_BACKEND rdft/z_vdsp.c)
    set(PUREFFT_LIBRARIES "-framework Accelerate" PFFFT::PFFFT)
    set(PUREFFT_RACE pffft vdsp)
elseif(ARM32 AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ne10.cmake")
    if(NE10_OK)
        set(PUREFFT_BACKEND rdft/z_ne10.c)
        set(PUREFFT_LIBRARIES ne10_fft pffft_neon PFFFT::PFFFT)
        set(PUREFFT_RACE pffft ne10)
    else()
        set(PUREFFT_BACKEND rdft/z_pffft.c)
        set(PUREFFT_LIBRARIES PFFFT::PFFFT)
    endif()
elseif(ARM64 AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ne10.cmake")
    set(PUREFFT_BACKEND rdft/z_ne10.c)
    set(PUREFFT_LIBRARIES ne10_fft PFFFT::PFFFT)
    set(PUREFFT_RACE pffft ne10)
elseif(RISCV64 AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/fftw.cmake")
    set(PUREFFT_BACKEND rdft/z_fftw.c)
    set(PUREFFT_LIBRARIES fftw3f pffft_rvv)
    set(PUREFFT_RACE fftw pffft)
else()
    set(PUREFFT_BACKEND rdft/z_pffft.c)
    set(PUREFFT_LIBRARIES PFFFT::PFFFT)
endif()
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
# purefft_library(<name> <backend source> [SOURCES ...] [DEFINITIONS ...] [LIBRARIES ...])
function(purefft_library name backend)
    cmake_parse_arguments(PARSE_ARGV 2 arg "" "" "SOURCES;DEFINITIONS;LIBRARIES")
    add_library(${name} STATIC 	"${PUREFFT_DIR}/api/spectrum.c" 
								"${PUREFFT_DIR}/kernel/race.c" 
								"${PUREFFT_DIR}/kernel/pow.c"
                               	"${PUREFFT_DIR}/simd-support/simd.c" 
								"${backend}" 
								${arg_SOURCES} 
	)
    target_include_directories(${name} 	PUBLIC 	"$<BUILD_INTERFACE:${PUREFFT_DIR}/api>" 
										PRIVATE "${PUREFFT_DIR}")
    target_compile_definitions(${name} PRIVATE ${arg_DEFINITIONS})
    target_link_libraries(${name} PRIVATE ${arg_LIBRARIES})
    set_target_properties(${name} PROPERTIES 	POSITION_INDEPENDENT_CODE ON 
												C_STANDARD 99 
												C_EXTENSIONS OFF 
												CXX_STANDARD 20) # KFR 7 needs C++20
endfunction()
purefft_library(PUREfft "${PUREFFT_BACKEND}" LIBRARIES ${PUREFFT_LIBRARIES})
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
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Generic") # bare metal calls main() w/o args
        add_subdirectory(libbench2)
    endif()
    add_subdirectory(tests)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Generic")
        add_subdirectory(tools)
        add_subdirectory(benchees)
    endif()
endif()
