##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

option(BENCH_ZLDSP "build the zldsp benchee (Highway is the bench's largest build)" ON)
if(NOT BENCH_ZLDSP)
    return()
endif()
if(NOT BENCH_AVX2_FLAGS AND NOT (BENCH_ARM64 AND NOT MSVC))
    message(STATUS "bench: zldsp left out: it needs AVX2 on x86-64 or NEON on 64-bit ARM (with GCC or Clang)")
    return()
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/zldsp_fft/src/zldsp_fft_rfft.hpp")
    message(STATUS "bench: zldsp left out: its submodule is missing (git submodule update --init --recursive)")
    return()
endif()
if(NOT TARGET hwy)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/highway/CMakeLists.txt")
        message(STATUS "bench: zldsp left out: Highway's submodule is missing (git submodule update --init --recursive)")
        return()
    endif()
    foreach(option HWY_ENABLE_TESTS HWY_ENABLE_EXAMPLES HWY_ENABLE_CONTRIB HWY_ENABLE_INSTALL)
        set(${option} OFF CACHE BOOL "")
    endforeach()
    add_subdirectory(highway highway EXCLUDE_FROM_ALL)
endif()
add_benchee(zldsp SOURCES doit.cpp LIBRARIES hwy DEFINITIONS HWY_COMPILE_ONLY_STATIC)
target_include_directories(zldsp_doit PRIVATE zldsp_fft/src)
set_target_properties(zldsp_doit PROPERTIES CXX_STANDARD 20) # zldsp uses std::numbers and std::bit_width
set(flags ${BENCH_AVX2_FLAGS})
if(BENCH_AVX2_FLAGS AND NOT MSVC)
    list(APPEND flags -mbmi2 -mf16c -mpclmul -maes) # HWY_AVX2 needs these too
endif()
target_compile_options(zldsp_doit PRIVATE ${flags})
