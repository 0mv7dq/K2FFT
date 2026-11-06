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
        add_executable(PUREfft_check_${check} ${check}.c "${PUREFFT_DIR}/support/mcu/startup.c")
        target_include_directories(PUREfft_check_${check} PRIVATE "${PUREFFT_DIR}")
        target_link_libraries(PUREfft_check_${check} PRIVATE PUREfft)
        add_test(NAME PUREfft_${check} COMMAND PUREfft_check_${check})
        set_tests_properties(PUREfft_${check} PROPERTIES PASS_REGULAR_EXPRESSION "all checks passed" FAIL_REGULAR_EXPRESSION "FAIL" LABELS smallcheck)
    endforeach()
    add_benchee(PUREfft SOURCES bench.c hook.c LIBRARIES PUREfft) # counted by tools/count-all.c
    target_include_directories(PUREfft_doit PRIVATE "${PUREFFT_DIR}")
    return()
endif()


# each kernel alone through the planner (api/fft.c's FORCED_KERNEL): tests/fft.c and libbench2's checks, not benchmarked
foreach(k ${PUREFFT_KERNELS})
    purefft_library(PUREfft_${k}_library DEFINITIONS "FORCED_KERNEL=\"${k}\"")
    add_executable(PUREfft_check_fft_${k} fft.c)
    target_include_directories(PUREfft_check_fft_${k} PRIVATE "${PUREFFT_DIR}")
    target_link_libraries(PUREfft_check_fft_${k} PRIVATE PUREfft_${k}_library)
    add_test(NAME PUREfft_fft_${k} COMMAND PUREfft_check_fft_${k})
    set_tests_properties(PUREfft_fft_${k} PROPERTIES LABELS smallcheck)
    add_benchee(PUREfft_${k} CHECKS SOURCES 	bench.c
										hook.c
								LIBRARIES 	PUREfft_${k}_library
	)
    target_include_directories(PUREfft_${k}_doit PRIVATE "${PUREFFT_DIR}")
endforeach()

add_benchee(PUREfft SOURCES 	bench.c
								hook.c
					LIBRARIES 	PUREfft
)
target_include_directories(PUREfft_doit PRIVATE "${PUREFFT_DIR}")
add_bench_check(PUREfft_nosimd PUREfft verify)
set_tests_properties(PUREfft_nosimd PROPERTIES ENVIRONMENT PUREFFT_NOSIMD=1) #! FALLBACK
add_bench_check(PUREfft_paranoid 	PUREfft paranoid 
									LABELS 	paranoid 
)

set(checks fft spectrum simd)
if(Threads_FOUND AND NOT WIN32)
    list(APPEND checks threads)
endif()
foreach(check ${checks})
    add_executable(PUREfft_check_${check} ${check}.c)
    target_include_directories(PUREfft_check_${check} PRIVATE "${PUREFFT_DIR}")
    target_link_libraries(PUREfft_check_${check} PRIVATE PUREfft)
    add_test(NAME PUREfft_${check} COMMAND PUREfft_check_${check})
endforeach()
add_test(NAME PUREfft_simd_off COMMAND PUREfft_check_simd off)
set_tests_properties(PUREfft_fft PUREfft_simd PROPERTIES LABELS smallcheck)
set_tests_properties(PUREfft_simd_off PROPERTIES 	LABELS 		smallcheck 
													ENVIRONMENT PUREFFT_NOSIMD=1)
set_tests_properties(PUREfft_spectrum PROPERTIES LABELS check)
if(TEST PUREfft_threads)
    set_tests_properties(PUREfft_threads PROPERTIES LABELS check)
endif()
