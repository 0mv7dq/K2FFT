##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#


# tools/count-all.c on libbench2 and the benchee -D BENCHEE= names, its main called by count.c's app_main
get_filename_component(purefft "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
get_filename_component(component "${purefft}" NAME)
set(pffft "${purefft}/libs/pffft")
if(BENCHEE STREQUAL "PUREfft")
    set(doit "${purefft}/tests/bench.c" "${purefft}/tests/hook.c")
elseif(BENCHEE STREQUAL "pffft")
    set(doit "${purefft}/benchees/pffft/doit.c") # the component's PFFFT
elseif(BENCHEE STREQUAL "kissfft")
    set(doit "${purefft}/benchees/kissfft/doit.c" "${pffft}/kissfft/kiss_fft.c" "${pffft}/kissfft/tools/kiss_fftr.c")
elseif(BENCHEE STREQUAL "pocketfft")
    set(doit "${purefft}/benchees/pocketfft/doit.c" "${pffft}/pocketfft/pocketfft_single.c")
elseif(BENCHEE STREQUAL "cmsis")
    set(doit "${purefft}/benchees/cmsis/doit.c")
elseif(NOT CMAKE_BUILD_EARLY_EXPANSION) # ESP-IDF's first pass, for requirements, has no -D
    message(FATAL_ERROR "idf.py -D BENCHEE=PUREfft|pffft|kissfft|pocketfft|cmsis")
endif()
file(GLOB libbench2 "${purefft}/libbench2/*.c")
list(REMOVE_ITEM libbench2 "${purefft}/libbench2/main.c") # count-all.c's main, as count_main
# the benchee before libbench2, so the archive's first definition of a hook (after-*.c) is the benchee's, as when libbench2 is its own library
idf_component_register(SRCS              "count.c" "${purefft}/tools/count-all.c" ${doit} ${libbench2}
                       PRIV_INCLUDE_DIRS "${purefft}" "${purefft}/benchees" "${pffft}/include" "${pffft}/kissfft" "${pffft}/kissfft/tools" "${pffft}/pocketfft"
                       PRIV_REQUIRES     ${component}
)

# libbench2's config.h, for newlib
foreach(have ALLOCA_H DECL_DRAND48 DECL_SRAND48 DRAND48 GETTIMEOFDAY ISNAN SNPRINTF STDDEF_H STDLIB_H SYS_TIME_H UNISTD_H)
    set(HAVE_${have} 1)
endforeach()
configure_file("${purefft}/cmake.config.h.in" libbench2/config.h @ONLY)
target_include_directories(${COMPONENT_LIB} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/libbench2")
include("${purefft}/cmake/sizes.cmake")
bench_sizes(sizes 4096) # what the S3's 512 KB of SRAM holds
list(JOIN sizes "," sizes)
target_compile_definitions(${COMPONENT_LIB} PRIVATE "BENCHEE=\"${BENCHEE}\"" "BENCH_SIZE_LIST=${sizes}" kiss_fft_scalar=float)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=all) # libbench2 is FFTW's, ESP-IDF's -Werror=all isn't
set_source_files_properties("${purefft}/tools/count-all.c" PROPERTIES COMPILE_DEFINITIONS main=count_main)
if(BENCHEE STREQUAL "cmsis")
    include("${purefft}/cmake/cmsis.cmake")
    target_compile_options(cmsis_dsp_rfft PRIVATE -O2) # sdkconfig.defaults' COMPILER_OPTIMIZATION_PERF reaches components alone
    target_link_libraries(${COMPONENT_LIB} PRIVATE cmsis_dsp_rfft)
endif()
