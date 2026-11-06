##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

include_guard(GLOBAL)
set(IPP_VERSION 2026.1.0)
set(IPP_PYPI https://files.pythonhosted.org/packages/py2.py3/i) # PyPI's path by file name, which redirects to the file
set(IPP_WIN_STATIC_URL       ${IPP_PYPI}/ipp-static/ipp_static-${IPP_VERSION}-py2.py3-none-win_amd64.whl)
set(IPP_WIN_STATIC_SHA256    41e47007eda2eb2ea0174e7a1c7258d87c7aa762316c66a3a6e769cc65124f70)
set(IPP_WIN_INCLUDE_URL      ${IPP_PYPI}/ipp-include/ipp_include-${IPP_VERSION}-py2.py3-none-win_amd64.whl)
set(IPP_WIN_INCLUDE_SHA256   54674c58bdb50b20bdc55e96555292bb79aa2a470b98e2f148352ce62a634fb9)
set(IPP_LINUX_STATIC_URL     ${IPP_PYPI}/ipp-static/ipp_static-${IPP_VERSION}-py2.py3-none-manylinux_2_28_x86_64.whl)
set(IPP_LINUX_STATIC_SHA256  a1f8258198a75e9ba8695c168542bacafb28c0695c672942336f9a86f3a4b0d3)
set(IPP_LINUX_INCLUDE_URL    ${IPP_PYPI}/ipp-include/ipp_include-${IPP_VERSION}-py2.py3-none-manylinux_2_28_x86_64.whl)
set(IPP_LINUX_INCLUDE_SHA256 866abc2721bcaa7b9285be09c4f8f4fc3e037160bbdf5183dc0d5dbfceeba445)
set(IPP_DIR "${PROJECT_BINARY_DIR}/ipp")
if(WIN32)
    set(IPP_OS WIN)
    set(IPP_LIBS Library/lib/ippsmt.lib Library/lib/ippvmmt.lib Library/lib/ippcoremt.lib)
else()
    set(IPP_OS LINUX)
    set(IPP_LIBS lib/libipps.a lib/libippvm.a lib/libippcore.a)
endif()
set(IPP_STATIC  "${IPP_DIR}/ipp_static-${IPP_VERSION}.data/data")
set(IPP_INCLUDE "${IPP_DIR}/ipp_include-${IPP_VERSION}.data/data/include")
set(IPP_LICENSE "${IPP_DIR}/ipp_static-${IPP_VERSION}.dist-info/LICENSE.txt")
file(DOWNLOAD ${IPP_${IPP_OS}_STATIC_URL}  "${IPP_DIR}/ipp_static.whl"  EXPECTED_HASH SHA256=${IPP_${IPP_OS}_STATIC_SHA256})
file(DOWNLOAD ${IPP_${IPP_OS}_INCLUDE_URL} "${IPP_DIR}/ipp_include.whl" EXPECTED_HASH SHA256=${IPP_${IPP_OS}_INCLUDE_SHA256})
list(TRANSFORM IPP_LIBS PREPEND "*" OUTPUT_VARIABLE IPP_PATTERNS)
file(ARCHIVE_EXTRACT INPUT "${IPP_DIR}/ipp_static.whl"  DESTINATION "${IPP_DIR}" PATTERNS ${IPP_PATTERNS} "*LICENSE.txt")
file(ARCHIVE_EXTRACT INPUT "${IPP_DIR}/ipp_include.whl" DESTINATION "${IPP_DIR}" PATTERNS "*/include/*")
list(TRANSFORM IPP_LIBS PREPEND "${IPP_STATIC}/")
add_library(ipp_signal INTERFACE)
target_include_directories(ipp_signal SYSTEM INTERFACE "${IPP_INCLUDE}")
target_link_libraries(ipp_signal INTERFACE ${IPP_LIBS})
