##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#


get_filename_component(purefft "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
get_filename_component(component "${purefft}" NAME)
idf_component_register(SRCS "checks.c" 
							"${purefft}/tests/fft.c" 
							"${purefft}/tests/spectrum.c" 
                       PRIV_INCLUDE_DIRS 	"${purefft}" 
                       PRIV_REQUIRES 		${component} 
)
set_source_files_properties("${purefft}/tests/fft.c" 		PROPERTIES COMPILE_DEFINITIONS main=fft_main)
set_source_files_properties("${purefft}/tests/spectrum.c" 	PROPERTIES COMPILE_DEFINITIONS main=spectrum_main)
