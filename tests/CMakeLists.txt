##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#


# bare metal (support/mcu/) ... QEMU's board
if(CMAKE_SYSTEM_NAME STREQUAL "Generic")
    foreach(check fft spectrum)
        add_executable(K2FFT_check_${check} ${check}.c "${K2FFT_DIR}/support/mcu/startup.c")
        target_include_directories(K2FFT_check_${check} PRIVATE "${K2FFT_DIR}")
        target_link_libraries(K2FFT_check_${check} PRIVATE K2FFT)
        add_test(NAME K2FFT_${check} COMMAND K2FFT_check_${check})
        set_tests_properties(K2FFT_${check} PROPERTIES PASS_REGULAR_EXPRESSION "all checks passed" FAIL_REGULAR_EXPRESSION "FAIL" LABELS smallcheck)
    endforeach()
    add_benchee(K2FFT SOURCES bench.c hook.c LIBRARIES K2FFT) # counted by tools/count-all.c
    target_include_directories(K2FFT_doit PRIVATE "${K2FFT_DIR}")
    return()
endif()


# each kernel alone through the planner (kernel/planner.c's FORCED_KERNEL; bench.c's, the problems it does): tests/fft.c and
# libbench2's checks, not benchmarked
foreach(k ${K2FFT_KERNELS})
    k2fft_library(K2FFT_${k}_library DEFINITIONS "FORCED_KERNEL=\"${k}\"")
    add_executable(K2FFT_check_fft_${k} fft.c)
    target_include_directories(K2FFT_check_fft_${k} PRIVATE "${K2FFT_DIR}")
    target_link_libraries(K2FFT_check_fft_${k} PRIVATE K2FFT_${k}_library)
    add_test(NAME K2FFT_fft_${k} COMMAND K2FFT_check_fft_${k})
    set_tests_properties(K2FFT_fft_${k} PROPERTIES LABELS smallcheck)
    add_benchee(K2FFT_${k} CHECKS CLASSES SOURCES 	bench.c
										hook.c
								LIBRARIES 	K2FFT_${k}_library
								DEFINITIONS "FORCED_KERNEL=\"${k}\""
	)
    target_include_directories(K2FFT_${k}_doit PRIVATE "${K2FFT_DIR}")
endforeach()
if(TARGET K2FFT_hwyfft_library) # K2FFT's own kernel, benchmarked as the libraries are
    add_benchee(hwyfft SOURCES bench.c hook.c LIBRARIES K2FFT_hwyfft_library DEFINITIONS "FORCED_KERNEL=\"hwyfft\"")
    target_include_directories(hwyfft_doit PRIVATE "${K2FFT_DIR}")
endif()

add_benchee(K2FFT CLASSES SOURCES 	bench.c
								hook.c
					LIBRARIES 	K2FFT
)
target_include_directories(K2FFT_doit PRIVATE "${K2FFT_DIR}")
add_benchee(K2FFT-d DOUBLE CLASSES SOURCES bench.c hook.c LIBRARIES K2FFT) # api/k2fft.h's k2fft_, for the d classes
target_include_directories(K2FFT-d_doit PRIVATE "${K2FFT_DIR}")
add_bench_check(K2FFT_nosimd K2FFT verify)
set_tests_properties(K2FFT_nosimd PROPERTIES ENVIRONMENT K2FFT_NOSIMD=1) #! FALLBACK
add_bench_check(K2FFT_paranoid 	K2FFT paranoid 
									LABELS 	paranoid 
)

set(checks fft spectrum simd)
if(Threads_FOUND AND NOT WIN32)
    list(APPEND checks threads)
endif()
foreach(check ${checks})
    add_executable(K2FFT_check_${check} ${check}.c)
    target_include_directories(K2FFT_check_${check} PRIVATE "${K2FFT_DIR}")
    target_link_libraries(K2FFT_check_${check} PRIVATE K2FFT)
    add_test(NAME K2FFT_${check} COMMAND K2FFT_check_${check})
endforeach()
add_test(NAME K2FFT_simd_off COMMAND K2FFT_check_simd off)
set_tests_properties(K2FFT_fft K2FFT_simd PROPERTIES LABELS smallcheck)
set_tests_properties(K2FFT_simd_off PROPERTIES 	LABELS 		smallcheck 
													ENVIRONMENT K2FFT_NOSIMD=1)
set_tests_properties(K2FFT_spectrum PROPERTIES LABELS check)
if(TEST K2FFT_threads)
    set_tests_properties(K2FFT_threads PROPERTIES LABELS check)
endif()

if(CMAKE_CROSSCOMPILING_EMULATOR) # an emulator's time isn't the target's: plans w/o timing (kernel/race.h)
    get_property(tests DIRECTORY PROPERTY TESTS)
    set_property(TEST ${tests} APPEND PROPERTY ENVIRONMENT K2FFT_NORACE=1)
endif()
