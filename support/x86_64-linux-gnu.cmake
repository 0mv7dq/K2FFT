##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#


#? x86-64 Linux on an older CPU, as a portable build meets it, under QEMU: -D QEMU_CPU=qemu64 (SSE2, x86-64's baseline) or Nehalem (SSE4.2, no AVX)

set(CMAKE_SYSTEM_NAME Linux) # cross-compiling to CMake, so tests and try_run go through the emulator
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(QEMU_CPU qemu64 CACHE STRING "the CPU qemu-x86_64 emulates")
set(CMAKE_CROSSCOMPILING_EMULATOR qemu-x86_64 -cpu ${QEMU_CPU})
