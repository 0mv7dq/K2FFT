##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS ON) # for drand48 and clock_gettime
set(CMAKE_CXX_STANDARD 17)
if(NOT PROJECT_IS_TOP_LEVEL AND NOT CMAKE_TESTING_ENABLED) # before enable_testing() sets it
    message(WARNING "K2FFT: call enable_testing() before add_subdirectory, or K2FFT's tests aren't registered at your build's top")
endif()
enable_testing()
set_property(GLOBAL PROPERTY BENCH_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/emulator.txt" "${CMAKE_CROSSCOMPILING_EMULATOR}\n") # for scripts/benchmark, as a toolchain's emulator isn't cached
k2fft_arch(BENCH_X86_64 "defined(__x86_64__) || defined(_M_X64)")
k2fft_arch(BENCH_ARM64  "defined(__aarch64__) || defined(_M_ARM64)")

# tools/alloc-check.c needs the linker's --wrap, and every function it wraps
set(CMAKE_REQUIRED_LINK_OPTIONS "-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=memalign,--wrap=posix_memalign,--wrap=aligned_alloc,--wrap=free")
unset(BENCH_WRAP CACHE)
check_c_source_compiles("#include <stddef.h>
void *__real_malloc(size_t n);
void *__real_calloc(size_t a, size_t b);
void *__real_realloc(void *p, size_t n);
void *__real_memalign(size_t a, size_t n);
int __real_posix_memalign(void **p, size_t a, size_t n);
void *__real_aligned_alloc(size_t a, size_t n);
void __real_free(void *p);
void *__wrap_malloc(size_t n){ return __real_malloc(n); }
void *__wrap_calloc(size_t a, size_t b){ return __real_calloc(a, b); }
void *__wrap_realloc(void *p, size_t n){ return __real_realloc(p, n); }
void *__wrap_memalign(size_t a, size_t n){ return __real_memalign(a, n); }
int __wrap_posix_memalign(void **p, size_t a, size_t n){ return __real_posix_memalign(p, a, n); }
void *__wrap_aligned_alloc(size_t a, size_t n){ return __real_aligned_alloc(a, n); }
void __wrap_free(void *p){ __real_free(p); }
int main(void){ return 0; }" BENCH_WRAP)
unset(CMAKE_REQUIRED_LINK_OPTIONS)
set_property(GLOBAL PROPERTY BENCH_WRAP "${BENCH_WRAP}")
if(NOT BENCH_WRAP AND NOT CMAKE_SYSTEM_NAME STREQUAL "Generic")
    message(STATUS "bench: the heap checks (alloc-check) left out: they need the linker's --wrap and every function it wraps")
endif()

# libbench2's config.h
include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckPrototypeDefinition)
foreach(h alloca.h stddef.h stdlib.h sys/time.h unistd.h)
    string(MAKE_C_IDENTIFIER "HAVE_${h}" var)
    string(TOUPPER "${var}" var)
    check_include_file(${h} ${var})
endforeach()
check_prototype_definition(drand48 "double drand48 (void)" "0" stdlib.h HAVE_DECL_DRAND48)
check_prototype_definition(srand48 "void srand48(long int seedval)" "0" stdlib.h HAVE_DECL_SRAND48)
check_symbol_exists(gettimeofday sys/time.h HAVE_GETTIMEOFDAY)
check_symbol_exists(drand48 stdlib.h HAVE_DRAND48)
check_symbol_exists(isnan math.h HAVE_ISNAN)
check_symbol_exists(snprintf stdio.h HAVE_SNPRINTF)
set(BENCHFFT_SINGLE ON)
configure_file(cmake.config.h.in libbench2/config.h @ONLY)
set(BENCHFFT_SINGLE OFF) # libbench2_double's
configure_file(cmake.config.h.in libbench2/double/config.h @ONLY)
unset(BENCHFFT_SINGLE)

set(BENCH_SIZES "" CACHE STRING "the sizes asked of every benchee (empty: the even 5-smooth sizes from 32 to K2FFT's largest)")
include("${CMAKE_CURRENT_LIST_DIR}/sizes.cmake")
set(sizes ${BENCH_SIZES})
if(NOT sizes)
    bench_sizes(sizes ${K2FFT_LARGEST})
endif()
list(JOIN sizes "\n" text)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/sizes.txt" "${text}\n")
# every kind of problem K2FFT's interface plans, as FFTW's tests check them (tests/check.cmake's, w/o the precision): complex and real
# both ways, in and out of place, the r2r kinds, 2 and 3 dimensions, a batch; classes.txt, at the powers of two, for tools/alloc-check.c
set(BENCH_CLASSES cofN:4096 cobN:4096 cifN:4096 cibN:4096 rofN:4096 robN:4096 rifN:4096 ribN:4096 koNf:1024 koNb:1024 koNh:1024
    koNe00:1024 koNe01:1024 koNe10:1024 koNe11:1024 koNo00:1024 koNo01:1024 koNo10:1024 koNo11:1024 cofNxN:64 robNxN:64 rifNxN:64
    cobNxNxN:32 rofNxNxN:32 cofN*4:1024 robN*4:1024)
set(text "")
foreach(c ${BENCH_CLASSES})
    string(REGEX REPLACE ":.*" "" template "${c}")
    string(REGEX REPLACE ".*:" "" cap "${c}")
    foreach(n ${sizes})
        math(EXPR other "${n}&(${n}-1)")
        if(other EQUAL 0 AND NOT n GREATER cap)
            string(REPLACE "N" "${n}" p "${template}")
            string(APPEND text "${p}\n")
        endif()
    endforeach()
endforeach()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/classes.txt" "${text}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/benchees.txt" "")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/measures.txt" "")

# add_bench_check(<test> <benchee> verify|accuracy|paranoid [POW2] [ROUNDS <n>] [LABELS ...] [CLASSES <problem> ...]), CLASSES
# tests/check.cmake's (rofN)
function(add_bench_check test benchee mode)
    cmake_parse_arguments(PARSE_ARGV 3 arg "POW2" "ROUNDS" "LABELS;CLASSES")
    get_property(out GLOBAL PROPERTY BENCH_BINARY_DIR)
    string(REPLACE ";" "$<SEMICOLON>" emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
    string(REPLACE ";" "$<SEMICOLON>" classes "${arg_CLASSES}")
    add_test(NAME ${test} COMMAND ${CMAKE_COMMAND} -DBENCH=$<TARGET_FILE:${benchee}_bench> "-DSIZES=${out}/sizes.txt" -DMODE=${mode}
             -DROUNDS=${arg_ROUNDS} -DPOW2=${arg_POW2} "-DCLASSES=${classes}" "-DEMULATOR=${emulator}" -P "${K2FFT_DIR}/tests/check.cmake")
    if(NOT arg_LABELS)
        set(arg_LABELS check)
    endif()
    set_tests_properties(${test} PROPERTIES LABELS "${arg_LABELS}" SKIP_REGULAR_EXPRESSION "can do none of the sizes asked:") # a CPU it can't run on, as IPP's w/o SSE4.2
endfunction()

# add_benchee(<name> [CHECKS] [DOUBLE] [CLASSES] SOURCES <doit.c ...> [LIBRARIES ...] [DEFINITIONS ...]): CHECKS, its checks alone,
# left out of benchees.txt; DOUBLE, double precision (libbench2_double), name <code>-d; CLASSES, BENCH_CLASSES checked too
function(add_benchee name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "CHECKS;DOUBLE;CLASSES" "" "SOURCES;LIBRARIES;DEFINITIONS")
    set(bench libbench2)
    if(arg_DOUBLE)
        set(bench libbench2_double)
    endif()
    string(REGEX REPLACE "-d$" "" code "${name}") # what its lines call it
    get_property(out GLOBAL PROPERTY BENCH_BINARY_DIR)
    get_property(wrap GLOBAL PROPERTY BENCH_WRAP)
    set(here "${K2FFT_DIR}")
    set(language C)
    if(arg_SOURCES MATCHES "\\.(cpp|cxx|cc)(;|$)")
        set(language CXX)
    endif()
    foreach(library ${arg_LIBRARIES}) # a C++ library's runtime comes w/ the C++ driver: a universal build detects none for C
        if(TARGET ${library})
            get_target_property(sources ${library} SOURCES)
            if(sources MATCHES "\\.(cpp|cxx|cc)(;|$)")
                set(language CXX)
            endif()
        endif()
    endforeach()
    add_library(${name}_doit OBJECT ${arg_SOURCES})
    target_link_libraries(${name}_doit PRIVATE ${bench} ${arg_LIBRARIES})
    target_include_directories(${name}_doit PRIVATE "${here}/benchees")
    target_compile_definitions(${name}_doit PRIVATE ${arg_DEFINITIONS} "BENCHEE=\"${code}\""
                               "BENCH_CC=\"${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}\"" "BENCH_CXX=\"${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}\"")
    if(CMAKE_SYSTEM_NAME STREQUAL "Generic") # bare metal has no command line: tools/count-all.c does every problem in one run
        file(STRINGS "${out}/sizes.txt" sizes)
        bench_problems(problems ${sizes})
        add_executable(${name}_count-all "${here}/tools/count-all.c" "${here}/support/mcu/startup.c" $<TARGET_OBJECTS:${name}_doit>)
        target_include_directories(${name}_count-all PRIVATE "${here}/benchees")
        target_compile_definitions(${name}_count-all PRIVATE "BENCH_PROBLEM_LIST=${problems}")
        target_link_libraries(${name}_count-all PRIVATE ${bench} ${arg_LIBRARIES})
        set_target_properties(${name}_count-all PROPERTIES OUTPUT_NAME count-all C_STANDARD 99 C_EXTENSIONS ON LINKER_LANGUAGE ${language}
                                                           RUNTIME_OUTPUT_DIRECTORY "$<1:${out}/bin/${name}>")
        file(APPEND "${out}/benchees.txt" "${name}\n")
        return()
    endif()
    set(tools bench cost count)
    if(arg_CHECKS)
        set(tools bench)
    endif()
    if(wrap)
        list(APPEND tools alloc-check)
    endif()
    foreach(tool ${tools})
        if(tool STREQUAL "bench")
            add_executable(${name}_${tool} $<TARGET_OBJECTS:${name}_doit>) # main() is libbench2's
        else()
            add_executable(${name}_${tool} "${here}/tools/${tool}.c" $<TARGET_OBJECTS:${name}_doit>)
            target_include_directories(${name}_${tool} PRIVATE "${here}/benchees")
            set_target_properties(${name}_${tool} PROPERTIES C_STANDARD 99 C_EXTENSIONS ON)
        endif()
        target_link_libraries(${name}_${tool} PRIVATE ${bench} ${arg_LIBRARIES})
        set_target_properties(${name}_${tool} PROPERTIES OUTPUT_NAME ${tool} LINKER_LANGUAGE ${language} RUNTIME_OUTPUT_DIRECTORY "$<1:${out}/bin/${name}>")
    endforeach()

    add_bench_check(${name}_smallcheck ${name} verify POW2 LABELS smallcheck)
    add_bench_check(${name}_verify ${name} verify)
    add_bench_check(${name}_accuracy ${name} accuracy LABELS big)
    add_bench_check(${name}_bigcheck ${name} verify ROUNDS 30 LABELS big)
    if(arg_CLASSES)
        add_bench_check(${name}_classes ${name} verify POW2 LABELS smallcheck CLASSES ${BENCH_CLASSES})
    endif()
    if(wrap)
        target_link_options(${name}_alloc-check PRIVATE "-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=memalign,--wrap=posix_memalign,--wrap=aligned_alloc,--wrap=free")
        if(language STREQUAL "CXX")
            target_link_options(${name}_alloc-check PRIVATE -static-libstdc++) # so --wrap sees operator new's malloc
        endif()
        add_test(NAME ${name}_alloc COMMAND ${name}_alloc-check "${out}/sizes.txt")
        set_tests_properties(${name}_alloc PROPERTIES LABELS check SKIP_RETURN_CODE 77) # 77 if the library's heap calls can't be seen
        if(arg_CLASSES AND code MATCHES "^K2FFT") # K2FFT's transforms never allocate; the libraries', as FFTW's DCTs, may
            add_test(NAME ${name}_classes_alloc COMMAND ${name}_alloc-check "${out}/classes.txt")
            set_tests_properties(${name}_classes_alloc PROPERTIES LABELS smallcheck SKIP_RETURN_CODE 77)
        endif()
    endif()

    if(NOT arg_CHECKS)
        file(APPEND "${out}/benchees.txt" "${name}\n")
    endif()
endfunction()

# bench_measure(<target> [EACH <argument ...>])
function(bench_measure target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "EACH")
    get_property(out GLOBAL PROPERTY BENCH_BINARY_DIR)
    set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<1:${out}/bin/measures>")
    set(line ${target})
    if(arg_EACH)
        list(JOIN arg_EACH "," each)
        string(APPEND line " each=${each}")
    endif()
    file(APPEND "${out}/measures.txt" "${line}\n")
endfunction()
