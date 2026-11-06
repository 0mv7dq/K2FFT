##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

include_guard(GLOBAL)
# bench_sizes(<var> <largest>): 2^a 3^b 5^c, a>0, from 64 to <largest>; scalar pffft_is_valid_size's sizes
function(bench_sizes var largest)
    set(sizes "")
    set(p2 2)
    while(p2 LESS_EQUAL largest)
        set(p23 ${p2})
        while(p23 LESS_EQUAL largest)
            set(n ${p23})
            while(n LESS_EQUAL largest)
                if(n GREATER_EQUAL 64)
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
