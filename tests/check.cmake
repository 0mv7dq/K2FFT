##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

# usage: cmake -DBENCH=<bench> -DSIZES=<sizes.txt> -DMODE=verify|accuracy|paranoid [-DROUNDS=<n>] [-DPOW2=TRUE] [-DCLASSES=<problems>]
#              [-DEMULATOR=<command>] -P check.cmake
# CLASSES, libbench2's problems (default rofN): N each size, :m up to m, as scripts/benchmark's w/o the precision

file(STRINGS "${SIZES}" candidates REGEX "^[0-9]+$")
if(NOT CLASSES)
    set(CLASSES rofN)
endif()
set(ask "")
set(tried "")
foreach(c ${CLASSES})
    string(REGEX REPLACE ":.*" "" template "${c}")
    set(cap "")
    if(c MATCHES ":([0-9]+)$")
        set(cap ${CMAKE_MATCH_1})
    endif()
    foreach(n ${candidates})
        math(EXPR other "${n}&(${n}-1)")
        if((NOT POW2 OR other EQUAL 0) AND (NOT cap OR NOT n GREATER cap))
            string(REPLACE "N" "${n}" p "${template}")
            list(APPEND tried ${p})
            list(APPEND ask --can-do ${p})
        endif()
    endforeach()
endforeach()
execute_process(COMMAND ${EMULATOR} "${BENCH}" ${ask} OUTPUT_VARIABLE out RESULT_VARIABLE r)
string(REGEX MATCHALL "#[tf]" answers "${out}")
list(LENGTH tried count)
list(LENGTH answers got)
if(NOT r EQUAL 0 OR NOT got EQUAL count)
    message(FATAL_ERROR "${BENCH} --can-do failed (${r}): ${got} answers for ${count} problems")
endif()
set(sizes "")
set(problems "")
foreach(i RANGE 1 ${count})
    math(EXPR i "${i}-1")
    list(GET answers ${i} yes)
    if(yes STREQUAL "#t")
        list(GET tried ${i} p)
        list(APPEND sizes ${p})
        if(MODE STREQUAL "accuracy")
            list(APPEND problems -a ${p})
        else()
            list(APPEND problems -y ${p})
        endif()
    endif()
endforeach()
list(LENGTH sizes count)
if(count EQUAL 0)
    message(FATAL_ERROR "can do none of the sizes asked: ${BENCH}") # first, as CMake wraps the line
endif()

set(options "")
if(MODE STREQUAL "verify")
    set(options -v1)
    if(ROUNDS)
        list(APPEND options --verify-rounds ${ROUNDS})
    endif()
elseif(MODE STREQUAL "paranoid")
    set(options -o paranoid)
endif()
execute_process(COMMAND ${EMULATOR} "${BENCH}" ${options} ${problems} OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE r)
string(REGEX MATCHALL "[^\n]+" lines "${out}")
list(LENGTH lines got)
if(NOT r EQUAL 0 OR (NOT MODE STREQUAL "paranoid" AND NOT got EQUAL count))
    message(FATAL_ERROR "${MODE} failed (${r}), ${got} results for ${count} sizes:\n${out}${err}")
endif()
if(MODE STREQUAL "accuracy")
    string(REGEX MATCHALL "[0-9.]+e[-+][0-9]+" errors "${out}")
    foreach(e ${errors})
        if(NOT e MATCHES "e-(0[6-9]|[1-9][0-9])$") #! under 1e-5
            message(FATAL_ERROR "accuracy: an error of ${e}:\n${out}")
        endif()
    endforeach()
endif()
message(STATUS "${MODE}: ${count} sizes passed (${sizes})")
