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
    message(WARNING "PUREfft: call enable_testing() before add_subdirectory, or PUREfft's tests aren't registered at your build's top")
endif()
enable_testing()
set_property(GLOBAL PROPERTY BENCH_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/emulator.txt" "${CMAKE_CROSSCOMPILING_EMULATOR}\n") # for scripts/benchmark, as a toolchain's emulator isn't cached
purefft_arch(BENCH_X86_64 "defined(__x86_64__) || defined(_M_X64)")
purefft_arch(BENCH_ARM64  "defined(__aarch64__) || defined(_M_ARM64)")

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
configure_file(cmake.config.h.in libbench2/config.h @ONLY)

set(BENCH_SIZES "" CACHE STRING "the sizes asked of every benchee (empty: the even 5-smooth sizes from 64 to 16384)")
set(sizes ${BENCH_SIZES})
if(NOT sizes)
    foreach(two 2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384)
        set(three ${two})
        while(three LESS_EQUAL 16384)
            set(n ${three})
            while(n LESS_EQUAL 16384)
                if(n GREATER_EQUAL 64)
                    list(APPEND sizes ${n})
                endif()
                math(EXPR n "${n}*5")
            endwhile()
            math(EXPR three "${three}*3")
        endwhile()
    endforeach()
    list(SORT sizes COMPARE NATURAL)
endif()
list(JOIN sizes "\n" text)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/sizes.txt" "${text}\n")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/benchees.txt" "")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/measures.txt" "")

# add_bench_check(<test> <benchee> verify|accuracy|paranoid [POW2] [ROUNDS <n>] [LABELS ...])
function(add_bench_check test benchee mode)
    cmake_parse_arguments(PARSE_ARGV 3 arg "POW2" "ROUNDS" "LABELS")
    get_property(out GLOBAL PROPERTY BENCH_BINARY_DIR)
    string(REPLACE ";" "$<SEMICOLON>" emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME ${test} COMMAND ${CMAKE_COMMAND} -DBENCH=$<TARGET_FILE:${benchee}_bench> "-DSIZES=${out}/sizes.txt" -DMODE=${mode}
             -DROUNDS=${arg_ROUNDS} -DPOW2=${arg_POW2} "-DEMULATOR=${emulator}" -P "${PUREFFT_DIR}/tests/check.cmake")
    if(NOT arg_LABELS)
        set(arg_LABELS check)
    endif()
    set_tests_properties(${test} PROPERTIES LABELS "${arg_LABELS}")
endfunction()

# add_benchee(<name> SOURCES <doit.c ...> [LIBRARIES ...] [DEFINITIONS ...])
function(add_benchee name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "SOURCES;LIBRARIES;DEFINITIONS")
    get_property(out GLOBAL PROPERTY BENCH_BINARY_DIR)
    get_property(wrap GLOBAL PROPERTY BENCH_WRAP)
    set(here "${PUREFFT_DIR}")
    set(language C)
    if(arg_SOURCES MATCHES "\\.(cpp|cxx|cc)(;|$)")
        set(language CXX)
    endif()
    add_library(${name}_doit OBJECT ${arg_SOURCES})
    target_link_libraries(${name}_doit PRIVATE libbench2 ${arg_LIBRARIES})
    target_include_directories(${name}_doit PRIVATE "${here}/benchees")
    target_compile_definitions(${name}_doit PRIVATE ${arg_DEFINITIONS} "BENCHEE=\"${name}\""
                               "BENCH_CC=\"${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}\"" "BENCH_CXX=\"${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}\"")
    if(CMAKE_SYSTEM_NAME STREQUAL "Generic") # bare metal has no command line: tools/count-all.c does every size in one run
        file(STRINGS "${out}/sizes.txt" sizes)
        list(JOIN sizes "," sizes)
        add_executable(${name}_count-all "${here}/tools/count-all.c" "${here}/support/mcu/startup.c" $<TARGET_OBJECTS:${name}_doit>)
        target_include_directories(${name}_count-all PRIVATE "${here}/benchees")
        target_compile_definitions(${name}_count-all PRIVATE "BENCH_SIZE_LIST=${sizes}")
        target_link_libraries(${name}_count-all PRIVATE libbench2 ${arg_LIBRARIES})
        set_target_properties(${name}_count-all PROPERTIES OUTPUT_NAME count-all C_STANDARD 99 C_EXTENSIONS ON LINKER_LANGUAGE ${language}
                                                           RUNTIME_OUTPUT_DIRECTORY "$<1:${out}/bin/${name}>")
        file(APPEND "${out}/benchees.txt" "${name}\n")
        return()
    endif()
    set(tools bench cost count)
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
        target_link_libraries(${name}_${tool} PRIVATE libbench2 ${arg_LIBRARIES})
        set_target_properties(${name}_${tool} PROPERTIES OUTPUT_NAME ${tool} LINKER_LANGUAGE ${language} RUNTIME_OUTPUT_DIRECTORY "$<1:${out}/bin/${name}>")
    endforeach()

    add_bench_check(${name}_smallcheck ${name} verify POW2 LABELS smallcheck)
    add_bench_check(${name}_verify ${name} verify)
    add_bench_check(${name}_accuracy ${name} accuracy LABELS big)
    add_bench_check(${name}_bigcheck ${name} verify ROUNDS 30 LABELS big)
    if(wrap)
        target_link_options(${name}_alloc-check PRIVATE "-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=memalign,--wrap=posix_memalign,--wrap=aligned_alloc,--wrap=free")
        if(language STREQUAL "CXX")
            target_link_options(${name}_alloc-check PRIVATE -static-libstdc++) # so --wrap sees operator new's malloc
        endif()
        add_test(NAME ${name}_alloc COMMAND ${name}_alloc-check "${out}/sizes.txt")
        set_tests_properties(${name}_alloc PROPERTIES LABELS check SKIP_RETURN_CODE 77) # 77 if the library's heap calls can't be seen
    endif()

    file(APPEND "${out}/benchees.txt" "${name}\n")
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
