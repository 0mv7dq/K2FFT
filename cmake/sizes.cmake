##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

include_guard(GLOBAL)
# bench_sizes(<var> <largest>): 2^a 3^b 5^c, a>0, from 32 (K2FFT's smallest) to <largest>; scalar pffft_is_valid_size's sizes
function(bench_sizes var largest)
    set(sizes "")
    set(p2 2)
    while(p2 LESS_EQUAL largest)
        set(p23 ${p2})
        while(p23 LESS_EQUAL largest)
            set(n ${p23})
            while(n LESS_EQUAL largest)
                if(n GREATER_EQUAL 32)
                    list(APPEND sizes ${n})
                endif()
                math(EXPR n "${n}*5")
            endwhile()
            math(EXPR p23 "${p23}*3")
        endwhile()
        math(EXPR p2 "${p2}*2")
    endwhile()
    list(SORT sizes COMPARE NATURAL)
    set(${var} ${sizes} PARENT_SCOPE)
endfunction()

# bench_problems(<var> <sizes ...>): tools/count-all.c's BENCH_PROBLEM_LIST, "rof32","rof64",...: K2FFT_CLASSES's (scripts/benchmark's,
# default srofN) single-precision classes at the sizes, :m capped, w/o the precision, as libbench2 names them
function(bench_problems var)
    set(classes srofN)
    if(DEFINED ENV{K2FFT_CLASSES} AND NOT "$ENV{K2FFT_CLASSES}" STREQUAL "")
        string(REPLACE " " ";" classes "$ENV{K2FFT_CLASSES}")
    endif()
    set(list "")
    foreach(c ${classes})
        if(NOT c MATCHES "^s")
            continue()
        endif()
        string(REGEX REPLACE ":.*" "" template "${c}")
        string(SUBSTRING "${template}" 1 -1 template)
        set(cap "")
        if(c MATCHES ":([0-9]+)$")
            set(cap ${CMAKE_MATCH_1})
        endif()
        foreach(n ${ARGN})
            if(NOT cap OR NOT n GREATER cap)
                string(REPLACE "N" "${n}" p "${template}")
                list(APPEND list "\"${p}\"")
            endif()
        endforeach()
    endforeach()
    list(JOIN list "," list)
    set(${var} "${list}" PARENT_SCOPE)
endfunction()
