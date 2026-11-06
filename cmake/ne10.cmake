##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

include_guard(GLOBAL)
set(NE10_OK TRUE)
if(ARM32)
    simd_flags(NE10_NEON_FLAGS neon)
    if(NOT NE10_NEON_FLAGS)
        set(NE10_OK FALSE)
        message(STATUS "this toolchain builds no ARMv7 NEON code: Ne10 is left out")
        return()
    endif()
endif()
set(NE10_COMMIT 1f059a764d0e1bc2481c0055c0e71538470baa83) # v1.2.1 plus position-independent assembly
set(NE10_URL https://github.com/projectNe10/Ne10/archive/${NE10_COMMIT}.tar.gz)
set(NE10_SHA256 faf28b2555105224eb76968bad555f7df32cb5b5580b40ed974bbaa2e997f480)
set(NE10_DIR "${PROJECT_BINARY_DIR}/ne10")
set(NE10_DSP "${NE10_DIR}/Ne10-${NE10_COMMIT}/modules/dsp")
file(DOWNLOAD ${NE10_URL} "${NE10_DIR}/ne10.tar.gz" INACTIVITY_TIMEOUT 60 EXPECTED_HASH SHA256=${NE10_SHA256})
file(ARCHIVE_EXTRACT INPUT "${NE10_DIR}/ne10.tar.gz" DESTINATION "${NE10_DIR}" PATTERNS "*/LICENSE" "*/inc/*" "*/common/*" "*/modules/dsp/*")
# Ne10's own CMake forces -mfpu=vfp3 on every file
add_library(ne10_fft STATIC "${NE10_DSP}/NE10_fft.c"
                            "${NE10_DSP}/NE10_fft_float32.c"
                            "${NE10_DSP}/NE10_fft_generic_float32.c"
                            "${NE10_DSP}/NE10_fft_generic_float32.neonintrinsic.cpp"
                            "${NE10_DSP}/NE10_fft_int32.c" # unused, but NE10_fft.c refers to these two
                            "${NE10_DSP}/NE10_fft_generic_int32.cpp"
)
if(ARM32)
    enable_language(ASM)
    target_sources(ne10_fft PRIVATE "${NE10_DSP}/NE10_fft_float32.neon.c" "${NE10_DSP}/NE10_fft_float32.neon.s")
    set_source_files_properties("${NE10_DSP}/NE10_fft_generic_float32.neonintrinsic.cpp" PROPERTIES COMPILE_OPTIONS "${NE10_NEON_FLAGS}")
    set_source_files_properties("${NE10_DSP}/NE10_fft_float32.neon.c" PROPERTIES COMPILE_OPTIONS "${NE10_NEON_FLAGS};-funsafe-math-optimizations") # so GCC uses NEON for its float loops
    set_source_files_properties("${NE10_DSP}/NE10_fft_float32.neon.s" PROPERTIES COMPILE_OPTIONS "${NE10_NEON_FLAGS};-Wa,--noexecstack")
else()
    target_sources(ne10_fft PRIVATE "${NE10_DSP}/NE10_fft_float32.neonintrinsic.c" "${NE10_DSP}/NE10_rfft_float32.c" "${NE10_DSP}/NE10_rfft_float32.neonintrinsic.c")
    target_compile_definitions(ne10_fft PRIVATE NE10_UNROLL_LEVEL=1)
endif()
target_include_directories(ne10_fft SYSTEM PUBLIC "${NE10_DIR}/Ne10-${NE10_COMMIT}/inc")
target_include_directories(ne10_fft PRIVATE "${NE10_DIR}/Ne10-${NE10_COMMIT}/common" "${NE10_DSP}")
target_compile_options(ne10_fft PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:-fno-strict-aliasing>) # Ne10 reads float arrays through its complex struct
set_target_properties(ne10_fft PROPERTIES C_EXTENSIONS ON POSITION_INDEPENDENT_CODE ON) # Ne10 is GNU C (asm labels)
