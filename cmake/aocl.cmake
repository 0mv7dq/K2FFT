##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

include_guard(GLOBAL)
# AOCL-FFTW, AMD's FFTW (GFFT's EPYC baseline), w/ its AVX2, AVX-512 and AMD paths; its fftwf_ names made aoclf_, so it links
# beside K2FFT's own FFTW, and built on its own (ExternalProject), as its CMake also names its library fftw3f
set(AOCL_VERSION 5.3)
set(AOCL_URL https://github.com/amd/amd-fftw/archive/refs/tags/${AOCL_VERSION}.tar.gz)
set(AOCL_SHA256 175a426dbbe9ba332416adeead59af364177016c7c3057c597a26c0159c83c43)
set(AOCL_DIR "${PROJECT_BINARY_DIR}/aocl")
set(AOCL_SRC "${AOCL_DIR}/amd-fftw-${AOCL_VERSION}")
get_filename_component(heap "${CMAKE_CURRENT_LIST_DIR}/../rdft/fftw_heap.h" ABSOLUTE)
if(NOT EXISTS "${AOCL_SRC}/CMakeLists.txt")
    file(DOWNLOAD ${AOCL_URL} "${AOCL_DIR}/aocl.tar.gz" INACTIVITY_TIMEOUT 60 EXPECTED_HASH SHA256=${AOCL_SHA256})
    file(ARCHIVE_EXTRACT INPUT "${AOCL_DIR}/aocl.tar.gz" DESTINATION "${AOCL_DIR}")
    foreach(edit "kernel/ifftw.h|CONCAT(fftwf_, name)|CONCAT(aoclf_, name)" "kernel/ifftw.h|define fftwf_mkstride|define aoclf_mkstride"
                 "kernel/ifftw.h|define fftwf_stride_destroy|define aoclf_stride_destroy" "api/fftw3.h|FFTW_CONCAT(fftwf_, name)|FFTW_CONCAT(aoclf_, name)"
                 "kernel/kalloc.c|#define real_free free|#include \"${heap}\"\n#define real_free free") # its heap as K2FFT's FFTW's (cmake/fftw.cmake)
        string(REPLACE "|" ";" edit "${edit}")
        list(GET edit 0 file)
        list(GET edit 1 from)
        list(GET edit 2 to)
        file(READ "${AOCL_SRC}/${file}" text)
        string(FIND "${text}" "${from}" at)
        if(at LESS 0)
            message(FATAL_ERROR "aocl: no '${from}' in ${file} to rename")
        endif()
        string(REPLACE "${from}" "${to}" text "${text}")
        file(WRITE "${AOCL_SRC}/${file}" "${text}")
    endforeach()
endif()
include(ExternalProject)
set(AOCL_LIB "${AOCL_DIR}/libaocl_fftw3f.a") # unlike K2FFT's own libfftw3f.a, once installed
# its library alone: its tests and tools spell fftwf_ themselves
ExternalProject_Add(aocl_fftw SOURCE_DIR "${AOCL_SRC}" BINARY_DIR "${AOCL_DIR}/build"
                    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                               -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=OFF
                               -DBUILD_TESTS=OFF -DENABLE_FLOAT=ON -DENABLE_AVX2=ON -DENABLE_AVX512=ON -DENABLE_AMD_OPT=ON
                               -DENABLE_THREADS=OFF -DENABLE_OPENMP=OFF -DDISABLE_FORTRAN=ON -DCMAKE_C_FLAGS=-DWITH_OUR_MALLOC=1
                    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target libfftw3f
                    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy "<BINARY_DIR>/liblibfftw3f.a" "${AOCL_LIB}" # its target is libfftw3f
                            COMMAND ${CMAKE_COMMAND} -E copy "${AOCL_SRC}/api/fftw3.h" "${AOCL_DIR}/include/fftw3.h"
                    BUILD_BYPRODUCTS "${AOCL_LIB}")
add_library(aocl_fftw3f INTERFACE) # its fftw3.h, in "${AOCL_DIR}/include", for benchees/aocl alone
add_dependencies(aocl_fftw3f aocl_fftw)
target_link_libraries(aocl_fftw3f INTERFACE "${AOCL_LIB}" m)
