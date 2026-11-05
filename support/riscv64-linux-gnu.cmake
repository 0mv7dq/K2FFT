##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#


#? 64-bit RISC-V Linux (RV64GC) w/ Debian's gcc-riscv64-linux-gnu & g++-riscv64-linux-gnu (QEMU w/ 256-bit vectors)
#? For RVV intrinsics, pass -DCMAKE_C_COMPILER=riscv64-linux-gnu-gcc-14 -DCMAKE_CXX_COMPILER=riscv64-linux-gnu-g++-14

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR riscv64)
if(NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER riscv64-linux-gnu-gcc)
endif()
if(NOT CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER riscv64-linux-gnu-g++)
endif()
set(CMAKE_CROSSCOMPILING_EMULATOR qemu-riscv64 -L /usr/riscv64-linux-gnu -cpu rv64,v=true,vlen=256)
set(ENV{PKG_CONFIG_LIBDIR} /usr/lib/riscv64-linux-gnu/pkgconfig)
