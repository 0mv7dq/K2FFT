##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

include_guard(GLOBAL)
# arm_rfft_fast_f32's files, as CMSIS-DSP's Source/TransformFunctions/Config.cmake; its own CMake builds all 660
set(CMSISDSP_VERSION 1.18.0)
set(CMSISDSP_URL https://github.com/ARM-software/CMSIS-DSP/archive/refs/tags/v${CMSISDSP_VERSION}.tar.gz)
set(CMSISDSP_SHA256 b60944180f51f9bb50df1a8cccad19d7c1ef8c14ed13da50b9b3f141032c3f56)
set(CMSISCORE_VERSION 6.4.0) # CMSIS_6's Core, which CMSIS-DSP needs on Cortex-M
set(CMSISCORE_URL https://github.com/ARM-software/CMSIS_6/archive/refs/tags/v${CMSISCORE_VERSION}.tar.gz)
set(CMSISCORE_SHA256 3f8ccae0c7f8ec2e1db2b671ddd1f5d97f5b42558ca7b15fff337c43d813fa02)
set(CMSISDSP_DIR "${PROJECT_BINARY_DIR}/cmsis-dsp")
file(DOWNLOAD ${CMSISDSP_URL} "${CMSISDSP_DIR}/cmsis-dsp.tar.gz" EXPECTED_HASH SHA256=${CMSISDSP_SHA256})
file(ARCHIVE_EXTRACT INPUT "${CMSISDSP_DIR}/cmsis-dsp.tar.gz" DESTINATION "${CMSISDSP_DIR}"
     PATTERNS "CMSIS-DSP-${CMSISDSP_VERSION}/Include/*" "CMSIS-DSP-${CMSISDSP_VERSION}/PrivateInclude/*"
              "CMSIS-DSP-${CMSISDSP_VERSION}/Source/TransformFunctions/*" "CMSIS-DSP-${CMSISDSP_VERSION}/Source/CommonTables/*")
set(CMSISDSP_SRC "${CMSISDSP_DIR}/CMSIS-DSP-${CMSISDSP_VERSION}")
add_library(cmsis_dsp_rfft STATIC "${CMSISDSP_SRC}/Source/TransformFunctions/arm_rfft_fast_f32.c"
                                  "${CMSISDSP_SRC}/Source/TransformFunctions/arm_rfft_fast_init_f32.c"
                                  "${CMSISDSP_SRC}/Source/TransformFunctions/arm_cfft_f32.c"
                                  "${CMSISDSP_SRC}/Source/TransformFunctions/arm_cfft_init_f32.c"
                                  "${CMSISDSP_SRC}/Source/TransformFunctions/arm_cfft_radix8_f32.c"
                                  "${CMSISDSP_SRC}/Source/TransformFunctions/arm_bitreversal2.c"
                                  "${CMSISDSP_SRC}/Source/CommonTables/arm_common_tables.c"
                                  "${CMSISDSP_SRC}/Source/CommonTables/arm_const_structs.c"
                                  "${CMSISDSP_SRC}/Source/CommonTables/arm_mve_tables.c" # Helium's, empty elsewhere
)
target_include_directories(cmsis_dsp_rfft SYSTEM PUBLIC "${CMSISDSP_SRC}/Include" PRIVATE "${CMSISDSP_SRC}/PrivateInclude")
if(CMAKE_SYSTEM_NAME STREQUAL "Generic" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm") # Cortex-M, w/ CMSIS-Core as CMSIS-DSP's own build
    file(DOWNLOAD ${CMSISCORE_URL} "${CMSISDSP_DIR}/cmsis-core.tar.gz" EXPECTED_HASH SHA256=${CMSISCORE_SHA256})
    file(ARCHIVE_EXTRACT INPUT "${CMSISDSP_DIR}/cmsis-core.tar.gz" DESTINATION "${CMSISDSP_DIR}" PATTERNS "*/CMSIS/Core/Include/*")
    target_include_directories(cmsis_dsp_rfft SYSTEM PUBLIC "${CMSISDSP_DIR}/CMSIS_6-${CMSISCORE_VERSION}/CMSIS/Core/Include")
elseif(NOT MSVC)
    target_compile_definitions(cmsis_dsp_rfft PUBLIC __GNUC_PYTHON__) # CMSIS-DSP's switch for hosts w/o CMSIS-Core
endif()
