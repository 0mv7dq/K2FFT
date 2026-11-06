##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

include_guard(GLOBAL)
# Arm Performance Libraries' FFTW interface (AutoFFT's ARMv8 baseline), sequential; its fftwf_ symbols renamed armplf_, so it
# links beside K2FFT's own FFTW. The installer is a shell script w/ a .tar.gz of .debs after its __START_OF_PAYLOAD__ line,
# unpacked here w/o running it
set(ARMPL_VERSION 26.01.1)
set(ARMPL_URL https://developer.arm.com/-/cdn-downloads/permalink/Arm-Performance-Libraries/Version_${ARMPL_VERSION}/arm-performance-libraries_${ARMPL_VERSION}_deb_gcc.tar)
set(ARMPL_SHA256 97bea9a6b873d7fbbe0f85150ff32fdc1d08cb3bd012ab79e687a595eb90141f)
set(ARMPL_DIR "${PROJECT_BINARY_DIR}/armpl")
set(ARMPL_ROOT "${ARMPL_DIR}/opt/arm/armpl_${ARMPL_VERSION}_gcc")
set(ARMPL_LIB "${ARMPL_DIR}/libarmpl_k2fft.a")
if(NOT EXISTS "${ARMPL_LIB}")
    file(DOWNLOAD ${ARMPL_URL} "${ARMPL_DIR}/armpl.tar" INACTIVITY_TIMEOUT 60 EXPECTED_HASH SHA256=${ARMPL_SHA256})
    file(ARCHIVE_EXTRACT INPUT "${ARMPL_DIR}/armpl.tar" DESTINATION "${ARMPL_DIR}")
    set(installer "${ARMPL_DIR}/arm-performance-libraries_${ARMPL_VERSION}_deb/arm-performance-libraries_${ARMPL_VERSION}_deb.sh")
    execute_process(COMMAND sh -c "tail -n +$(awk '/^__START_OF_PAYLOAD__/ {print NR + 1; exit}' \"$0\") \"$0\" | tar -xz -C \"$1\""
                            "${installer}" "${ARMPL_DIR}" RESULT_VARIABLE failed)
    file(ARCHIVE_EXTRACT INPUT "${ARMPL_DIR}/armpl_${ARMPL_VERSION}_gcc.deb" DESTINATION "${ARMPL_DIR}/deb") # an ar of data.tar.gz
    file(ARCHIVE_EXTRACT INPUT "${ARMPL_DIR}/deb/data.tar.gz" DESTINATION "${ARMPL_DIR}"
         PATTERNS "*/lib/libarmpl_lp64.a" "*/lib/libamath.a" "*/include/fftw3.h" "*/license_terms/*")
    execute_process(COMMAND sh -c "\"$0\" --defined-only \"$1\" | awk '$2 ~ /^[TDRB]$/ && $3 ~ /^fftwf_/ {print $3, \"armplf_\" substr($3, 7)}' | sort -u > \"$2\""
                            "${CMAKE_NM}" "${ARMPL_ROOT}/lib/libarmpl_lp64.a" "${ARMPL_DIR}/rename.txt" RESULT_VARIABLE failed_nm)
    execute_process(COMMAND "${CMAKE_OBJCOPY}" "--redefine-syms=${ARMPL_DIR}/rename.txt" "${ARMPL_ROOT}/lib/libarmpl_lp64.a" "${ARMPL_LIB}"
                    RESULT_VARIABLE failed_objcopy)
    if(failed OR failed_nm OR failed_objcopy OR NOT EXISTS "${ARMPL_LIB}")
        message(FATAL_ERROR "armpl: couldn't unpack and rename ${ARMPL_URL}")
    endif()
    file(REMOVE_RECURSE "${ARMPL_DIR}/armpl.tar" "${ARMPL_DIR}/deb" "${ARMPL_DIR}/arm-performance-libraries_${ARMPL_VERSION}_deb"
         "${ARMPL_DIR}/armpl_${ARMPL_VERSION}_gcc.deb" "${ARMPL_DIR}/arm-performance-libraries_${ARMPL_VERSION}_gcc.deb"
         "${ARMPL_ROOT}/lib/libarmpl_lp64.a") # 2.5 GB unpacked
endif()
add_library(armpl_fft INTERFACE) # its fftw3.h, in "${ARMPL_ROOT}/include", for benchees/armpl alone
target_link_libraries(armpl_fft INTERFACE "${ARMPL_LIB}" "${ARMPL_ROOT}/lib/libamath.a" m)
