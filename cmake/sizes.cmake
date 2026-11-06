##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

include_guard(GLOBAL)
# bench_sizes(<var> <largest>): the even 5-smooth sizes from 64 to <largest>
function(bench_sizes var largest)
    set(sizes "")
    foreach(two 2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384)
        set(three ${two})
        while(three LESS_EQUAL largest)
            set(n ${three})
            while(n LESS_EQUAL largest)
                if(n GREATER_EQUAL 64)
                    list(APPEND sizes ${n})
                endif()
                math(EXPR n "${n}*5")
            endwhile()
            math(EXPR three "${three}*3")
        endwhile()
    endforeach()
    list(SORT sizes COMPARE NATURAL)
    set(${var} ${sizes} PARENT_SCOPE)
endfunction()
