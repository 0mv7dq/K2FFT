##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

# FFTS (Blake, Witten and Cree's "fastest Fourier transform in the south"): SSE w/ its run-time generated code on x86-64, its
# NEON assembly on 32-bit ARM (rdft/ffts.c checks the CPU has NEON); its own CMake probes w/ -march=native, so its sources here
include_guard(GLOBAL)
set(FFTS_OK TRUE)
if(ARM32)
    simd_flags(FFTS_NEON_FLAGS neon)
    if(NOT FFTS_NEON_FLAGS)
        set(FFTS_OK FALSE)
        message(STATUS "this toolchain builds no ARMv7 NEON code: FFTS is left out")
        return()
    endif()
endif()
set(FFTS_COMMIT b22d839b61c5bd8a993e24ae3e01f39d9b5084fb)
set(FFTS_URL https://github.com/anthonix/ffts/archive/${FFTS_COMMIT}.tar.gz)
set(FFTS_SHA256 ec623a9e75cf0897370a7988206f16e47ed6e32485522a0a8cc2ae35d846612a)
set(FFTS_DIR "${PROJECT_BINARY_DIR}/ffts")
set(FFTS_SRC "${FFTS_DIR}/ffts-${FFTS_COMMIT}/src")
file(DOWNLOAD ${FFTS_URL} "${FFTS_DIR}/ffts.tar.gz" INACTIVITY_TIMEOUT 60 EXPECTED_HASH SHA256=${FFTS_SHA256})
file(ARCHIVE_EXTRACT INPUT "${FFTS_DIR}/ffts.tar.gz" DESTINATION "${FFTS_DIR}" PATTERNS "*/COPYRIGHT" "*/include/*" "*/src/*")
file(READ "${FFTS_SRC}/ffts_trig.c" trig) # its non-SSE2 twiddles name C99's double_t for its own ffts_double_t
set(at "const double_t *FFTS_RESTRICT hs;")
string(FIND "${trig}" "${at}" found)
if(found LESS 0)
    message(FATAL_ERROR "FFTS ${FFTS_COMMIT}: no '${at}' in ffts_trig.c to fix")
endif()
string(REPLACE "${at}" "const ffts_double_t *FFTS_RESTRICT hs;" trig "${trig}")
file(WRITE "${FFTS_SRC}/ffts_trig.c" "${trig}")
add_library(ffts STATIC "${FFTS_SRC}/ffts.c" "${FFTS_SRC}/ffts_chirp_z.c" "${FFTS_SRC}/ffts_nd.c" "${FFTS_SRC}/ffts_real.c"
                        "${FFTS_SRC}/ffts_real_nd.c" "${FFTS_SRC}/ffts_transpose.c" "${FFTS_SRC}/ffts_trig.c" "${FFTS_SRC}/ffts_static.c")
target_include_directories(ffts SYSTEM PUBLIC "${FFTS_DIR}/ffts-${FFTS_COMMIT}/include")
target_include_directories(ffts PRIVATE "${FFTS_SRC}")
target_compile_definitions(ffts PRIVATE FFTS_BUILD HAVE_STDINT_H HAVE_STDLIB_H HAVE_STRING_H _USE_MATH_DEFINES)
if(NOT WIN32)
    target_compile_definitions(ffts PRIVATE HAVE_POSIX_MEMALIGN HAVE_SYS_MMAN_H HAVE_UNISTD_H)
endif()
if(X86_64)
    target_compile_definitions(ffts PRIVATE HAVE_SSE HAVE_SSE2) # w/o its SSE3 (ffts_real.c's), past x86-64's baseline
    if(APPLE) # a hardened runtime (a notarized Pd) maps no code it writes: its static transforms
        target_compile_definitions(ffts PRIVATE DYNAMIC_DISABLED)
    else()
        target_sources(ffts PRIVATE "${FFTS_SRC}/codegen.c")
    endif()
else()
    enable_language(ASM)
    target_sources(ffts PRIVATE "${FFTS_SRC}/neon.s" "${FFTS_SRC}/neon_static.s")
    target_compile_definitions(ffts PRIVATE HAVE_NEON DYNAMIC_DISABLED)
    # ARM state throughout: its assembly's functions aren't typed, so a Thumb caller's bl wouldn't switch to them
    target_compile_options(ffts PRIVATE ${FFTS_NEON_FLAGS} -marm $<$<COMPILE_LANGUAGE:ASM>:-Wa,--noexecstack>)
endif()
if(NOT MSVC)
    target_compile_options(ffts PRIVATE $<$<COMPILE_LANGUAGE:C>:-w>) # FFTS's own warnings, not K2FFT's
endif()
set_target_properties(ffts PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(K2FFT_HAVE_LIBM)
    target_link_libraries(ffts PRIVATE m)
endif()
