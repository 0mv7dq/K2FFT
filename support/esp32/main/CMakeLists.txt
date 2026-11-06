##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#


get_filename_component(k2fft "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
get_filename_component(component "${k2fft}" NAME)
idf_component_register(SRCS "checks.c" 
							"${k2fft}/tests/fft.c" 
							"${k2fft}/tests/spectrum.c" 
                       PRIV_INCLUDE_DIRS 	"${k2fft}" 
                       PRIV_REQUIRES 		${component} 
)
set_source_files_properties("${k2fft}/tests/fft.c" 		PROPERTIES COMPILE_DEFINITIONS main=fft_main)
set_source_files_properties("${k2fft}/tests/spectrum.c" 	PROPERTIES COMPILE_DEFINITIONS main=spectrum_main)
