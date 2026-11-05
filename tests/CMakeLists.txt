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
    return()
endif()


# checking + setting race pathing
if(PUREFFT_RACE)
    foreach(path 0 1)
        list(GET PUREFFT_RACE ${path} part)
        purefft_library(PUREfft_${part}_library "${PUREFFT_DIR}/${PUREFFT_BACKEND}" 
												DEFINITIONS RACE_PATH=${path} 
												LIBRARIES 	${PUREFFT_LIBRARIES} 
		)
        add_benchee(PUREfft_${part} SOURCES 	bench.c
												hook.c
									LIBRARIES 	PUREfft_${part}_library
		)
        target_include_directories(PUREfft_${part}_doit PRIVATE "${PUREFFT_DIR}")
    endforeach()
endif()

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
