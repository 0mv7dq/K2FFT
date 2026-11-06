##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

include_guard(GLOBAL)
# oneMKL's DFTI, sequential (one thread, as AutoFFT's and GFFT's comparisons), from PyPI's wheels as cmake/ipp.cmake
set(MKL_VERSION 2026.2.0)
set(MKL_PYPI https://files.pythonhosted.org/packages/py2.py3/m) # PyPI's path by file name, which redirects to the file
set(MKL_WIN_STATIC_URL       ${MKL_PYPI}/mkl-static/mkl_static-${MKL_VERSION}-py2.py3-none-win_amd64.whl)
set(MKL_WIN_STATIC_SHA256    df7274c5b5ccee7f9544f9744bb80b8e4b9801becd522c7972534053da36418f)
set(MKL_WIN_INCLUDE_URL      ${MKL_PYPI}/mkl-include/mkl_include-${MKL_VERSION}-py2.py3-none-win_amd64.whl)
set(MKL_WIN_INCLUDE_SHA256   e07acc96c043de9e8c72ddfd624455ce58f077d6bd95db7854ab2bc4f7c323f5)
set(MKL_LINUX_STATIC_URL     ${MKL_PYPI}/mkl-static/mkl_static-${MKL_VERSION}-py2.py3-none-manylinux_2_28_x86_64.whl)
set(MKL_LINUX_STATIC_SHA256  090e439bf202b45579c75d2ba50ff7ead05752d553a06aba79c3ed2b5247921b)
set(MKL_LINUX_INCLUDE_URL    ${MKL_PYPI}/mkl-include/mkl_include-${MKL_VERSION}-py2.py3-none-manylinux_2_28_x86_64.whl)
set(MKL_LINUX_INCLUDE_SHA256 889cc72057a1671fee1e9a8beb2fc4e0d222ba78c08b6d69584dae03fbec4822)
set(MKL_DIR "${PROJECT_BINARY_DIR}/mkl")
if(WIN32)
    set(MKL_OS WIN)
    set(MKL_LIBS Library/lib/mkl_intel_lp64.lib Library/lib/mkl_sequential.lib Library/lib/mkl_core.lib)
else()
    set(MKL_OS LINUX)
    set(MKL_LIBS lib/libmkl_intel_lp64.a lib/libmkl_sequential.a lib/libmkl_core.a)
endif()
set(MKL_STATIC  "${MKL_DIR}/mkl_static-${MKL_VERSION}.data/data")
set(MKL_INCLUDE "${MKL_DIR}/mkl_include-${MKL_VERSION}.data/data/include")
if(WIN32)
    set(MKL_INCLUDE "${MKL_DIR}/mkl_include-${MKL_VERSION}.data/data/Library/include") # as its libraries, under Library
endif()
file(DOWNLOAD ${MKL_${MKL_OS}_STATIC_URL}  "${MKL_DIR}/mkl_static.whl"  INACTIVITY_TIMEOUT 60 EXPECTED_HASH SHA256=${MKL_${MKL_OS}_STATIC_SHA256})
file(DOWNLOAD ${MKL_${MKL_OS}_INCLUDE_URL} "${MKL_DIR}/mkl_include.whl" INACTIVITY_TIMEOUT 60 EXPECTED_HASH SHA256=${MKL_${MKL_OS}_INCLUDE_SHA256})
list(TRANSFORM MKL_LIBS PREPEND "*" OUTPUT_VARIABLE MKL_PATTERNS)
file(ARCHIVE_EXTRACT INPUT "${MKL_DIR}/mkl_static.whl"  DESTINATION "${MKL_DIR}" PATTERNS ${MKL_PATTERNS} "*LICENSE.txt")
file(ARCHIVE_EXTRACT INPUT "${MKL_DIR}/mkl_include.whl" DESTINATION "${MKL_DIR}" PATTERNS "*/include/*")
list(TRANSFORM MKL_LIBS PREPEND "${MKL_STATIC}/")
add_library(mkl_dft INTERFACE)
target_include_directories(mkl_dft SYSTEM INTERFACE "${MKL_INCLUDE}")
if(WIN32)
    target_link_libraries(mkl_dft INTERFACE ${MKL_LIBS})
else() # the three refer to each other: one archive, which ld rescans, also for K2FFT.pc's single pass
    set(merged "${MKL_DIR}/libmkl_k2fft.a")
    if(NOT EXISTS "${merged}")
        list(TRANSFORM MKL_LIBS PREPEND "addlib " OUTPUT_VARIABLE adds)
        list(JOIN adds "\n" adds)
        file(WRITE "${MKL_DIR}/merge.mri" "create ${merged}\n${adds}\nsave\nend\n")
        execute_process(COMMAND "${CMAKE_AR}" -M INPUT_FILE "${MKL_DIR}/merge.mri" RESULT_VARIABLE failed)
        if(failed)
            message(FATAL_ERROR "mkl: ${CMAKE_AR} -M couldn't merge ${MKL_LIBS}")
        endif()
    endif()
    target_link_libraries(mkl_dft INTERFACE "${merged}" Threads::Threads ${CMAKE_DL_LIBS} m)
endif()
