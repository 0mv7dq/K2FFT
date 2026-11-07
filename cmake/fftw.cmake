##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/simd.cmake")
set(FFTW_RELEASE 3.3.11)
set(FFTW_URL https://www.fftw.org/fftw-${FFTW_RELEASE}.tar.gz)
set(FFTW_SHA256 5630c24cdeb33b131612f7eb4b1a9934234754f9f388ff8617458d0be6f239a1)
set(FFTW_DIR "${PROJECT_BINARY_DIR}/fftw")
file(DOWNLOAD ${FFTW_URL} "${FFTW_DIR}/fftw.tar.gz" INACTIVITY_TIMEOUT 60 EXPECTED_HASH SHA256=${FFTW_SHA256})
file(ARCHIVE_EXTRACT INPUT "${FFTW_DIR}/fftw.tar.gz" DESTINATION "${FFTW_DIR}")
# its heap through rdft/fftw_heap.h, where a K2FFT plan lends its arena while it runs (WITH_OUR_MALLOC below: malloc and free alone)
get_filename_component(heap "${CMAKE_CURRENT_LIST_DIR}/../rdft/fftw_heap.h" ABSOLUTE)
set(kalloc "${FFTW_DIR}/fftw-${FFTW_RELEASE}/kernel/kalloc.c")
file(READ "${kalloc}" text)
set(at "#define real_free free")
string(FIND "${text}" "${at}" found)
if(found LESS 0)
    message(FATAL_ERROR "fftw: no '${at}' in ${kalloc} to route its heap")
endif()
string(REPLACE "${at}" "#include \"${heap}\"\n${at}" text "${text}")
file(WRITE "${kalloc}" "${text}")
# a timer where kernel/cycle.h has none (32-bit ARM, MIPS, MSVC's ARM64), w/o which FFTW_MEASURE plans as FFTW_ESTIMATE: the ARM
# generic timer, else POSIX's monotonic clock in ns, timed 10 us at least (timer.c's TIME_MIN, 100 ticks, is a cycle counter's)
file(APPEND "${FFTW_DIR}/fftw-${FFTW_RELEASE}/kernel/cycle.h" [=[

#if defined(_MSC_VER) && defined(_M_ARM64) && !defined(HAVE_TICK_COUNTER)
#include <intrin.h>
typedef unsigned __int64 ticks;
static __inline ticks getticks(void) { return (ticks)_ReadStatusReg(ARM64_CNTVCT); }
INLINE_ELAPSED(__inline)
#define HAVE_TICK_COUNTER
#endif

#if defined(__unix__) && !defined(HAVE_TICK_COUNTER)
#include <time.h>
typedef unsigned long long ticks;
static inline ticks getticks(void)
{
     struct timespec t;
     clock_gettime(CLOCK_MONOTONIC, &t);
     return (ticks)t.tv_sec * 1000000000ull + (ticks)t.tv_nsec;
}
INLINE_ELAPSED(inline)
#define HAVE_TICK_COUNTER
#define TIME_MIN 10000.0
#endif
]=])
# the options stay out of the cache; the SIMD codelets are added below, as FFTW's CMake has no NEON, VSX or AVX-512
set(fftw_options BUILD_TESTS ENABLE_OPENMP ENABLE_THREADS WITH_COMBINED_THREADS ENABLE_LONG_DOUBLE ENABLE_QUAD_PRECISION ENABLE_SSE ENABLE_SSE2
                 ENABLE_AVX ENABLE_AVX2)
foreach(option ${fftw_options})
    set(${option} OFF)
endforeach()
set(DISABLE_FORTRAN ON)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW) # option() honours normal variables
set(CMAKE_POLICY_VERSION_MINIMUM 3.5) # FFTW asks for CMake 3.0, which CMake 4 refuses
set(ENABLE_FLOAT ON) # fftw3f, then fftw3: one build each, as FFTW's CMake makes one precision
add_subdirectory("${FFTW_DIR}/fftw-${FFTW_RELEASE}" "${FFTW_DIR}/build" EXCLUDE_FROM_ALL)
set(ENABLE_FLOAT OFF)
add_subdirectory("${FFTW_DIR}/fftw-${FFTW_RELEASE}" "${FFTW_DIR}/build-double" EXCLUDE_FROM_ALL)
unset(CMAKE_POLICY_VERSION_MINIMUM)
unset(CMAKE_POLICY_DEFAULT_CMP0077)
foreach(option ${fftw_options} ENABLE_FLOAT DISABLE_FORTRAN)
    unset(${option})
endforeach()
foreach(fftw fftw3f fftw3)
    target_compile_definitions(${fftw} PRIVATE WITH_OUR_MALLOC=1) # kernel/kalloc.c's aligned blocks from malloc, which rdft/fftw_heap.h routes
    target_compile_options(${fftw} PRIVATE $<$<C_COMPILER_ID:GNU>:-fno-schedule-insns>) # GCC's early scheduling makes the codelets spill
    target_include_directories(${fftw} SYSTEM INTERFACE "$<BUILD_INTERFACE:${FFTW_DIR}/fftw-${FFTW_RELEASE}/api>")
    set_target_properties(${fftw} PROPERTIES C_EXTENSIONS ON POSITION_INDEPENDENT_CODE ON) # ISO C stops GCC fusing multiply-adds
endforeach()

# fftw_simd(<isa> <define> [flags ...]): configure's --enable-<isa>, its codelets alone built w/ flags, in both precisions' builds; FFTW runs
# them after its CPU checks
set(FFTW_SIMD "")
function(fftw_simd isa define)
    set(src "${FFTW_DIR}/fftw-${FFTW_RELEASE}")
    file(GLOB codelets "${src}/dft/simd/${isa}/*.c" "${src}/rdft/simd/${isa}/*.c")
    foreach(fftw fftw3f fftw3)
        if(isa STREQUAL "neon" AND fftw STREQUAL "fftw3")
            continue() # FFTW's NEON is single precision's
        endif()
        get_target_property(dir ${fftw} BINARY_DIR) # each build's scope, as their source files are the same
        target_sources(${fftw} PRIVATE ${codelets})
        set_source_files_properties(${codelets} DIRECTORY "${dir}" PROPERTIES COMPILE_OPTIONS "${ARGN}")
        target_compile_definitions(${fftw} PRIVATE ${define}=1)
    endforeach()
    set(FFTW_SIMD ${FFTW_SIMD} ${isa} PARENT_SCOPE)
endfunction()
k2fft_arch(X86_32 "defined(__i386__) || defined(_M_IX86)")
k2fft_arch(PPC64LE "defined(__powerpc64__) && defined(__LITTLE_ENDIAN__)")
if(MINGW)
    set(fftw_unaligned -Wa,-muse-unaligned-vector-move) # GCC aligns AVX spills to 32 bytes, Windows' stack to 16
endif()
if(X86_64 AND MSVC)
    fftw_simd(sse2 HAVE_SSE2) # x64's baseline
    fftw_simd(avx HAVE_AVX /arch:AVX)
    fftw_simd(avx2 HAVE_AVX2 /arch:AVX2)
    fftw_simd(avx2-128 HAVE_AVX2 /arch:AVX2)
    fftw_simd(avx512 HAVE_AVX512 /arch:AVX512)
elseif(X86_64)
    fftw_simd(sse2 HAVE_SSE2 -msse2)
    fftw_simd(avx HAVE_AVX -mavx ${fftw_unaligned})
    fftw_simd(avx2 HAVE_AVX2 -mavx2 -mfma ${fftw_unaligned})
    fftw_simd(avx2-128 HAVE_AVX2 -mavx2 -mfma ${fftw_unaligned})
    fftw_simd(avx512 HAVE_AVX512 -mavx512f ${fftw_unaligned})
elseif(X86_32)
    fftw_simd(sse2 HAVE_SSE2 -msse2)
elseif(ARM64 AND NOT MSVC) # MSVC: neither FFTW's NEON nor its timer
    fftw_simd(neon HAVE_NEON)
    if(NOT APPLE) # cycle.h has Apple's
        target_compile_definitions(fftw3f PRIVATE HAVE_ARMV8_CNTVCT_EL0=1) # the timer Linux lets user space read; w/o one FFTW_MEASURE plans as FFTW_ESTIMATE
        target_compile_definitions(fftw3 PRIVATE HAVE_ARMV8_CNTVCT_EL0=1)
    endif()
elseif(ARM32 AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    simd_flags(fftw_neon neon)
    if(fftw_neon)
        fftw_simd(neon HAVE_NEON ${fftw_neon})
        set_source_files_properties("${FFTW_DIR}/fftw-${FFTW_RELEASE}/simd-support/neon.c" DIRECTORY "${FFTW_DIR}/build"
                                    PROPERTIES COMPILE_OPTIONS -marm) # its check is an ARM-state instruction
    endif()
elseif(PPC64LE)
    fftw_simd(vsx HAVE_VSX -mvsx)
endif()
message(STATUS "FFTW ${FFTW_RELEASE}'s SIMD: ${FFTW_SIMD}")
