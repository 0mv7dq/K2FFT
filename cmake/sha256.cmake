##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

#* Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

##cmake-format:off[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]##
##[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#[[]]#cmake-format:on#

# usage: cmake -P cmake/sha256.cmake
# sets each <name>_SHA256 in cmake/*.cmake to the hash of what its <name>_URL downloads, after Renovate bumps a version

set(download "${CMAKE_CURRENT_LIST_DIR}/../build/sha256.download")
file(GLOB modules "${CMAKE_CURRENT_LIST_DIR}/*.cmake")
foreach(module ${modules})
    file(READ "${module}" text)
    set(lines "\n${text}") # set( at the start of a line only, not inside unset(
    string(REGEX MATCHALL "\n[ \t]*set\\([A-Z0-9_]+ +[^ \"\n)]+\\)" plain "${lines}") # the values a URL may refer to
    foreach(line ${plain})
        string(REGEX REPLACE "\n[ \t]*set\\(([A-Z0-9_]+) +([^ \"\n)]+)\\)" "\\1;\\2" pair "${line}")
        list(GET pair 0 name)
        list(GET pair 1 value)
        set(${name} "${value}")
    endforeach()
    set(changed FALSE)
    string(REGEX MATCHALL "\n[ \t]*set\\([A-Z0-9_]+_URL +[^ \n)]+\\)" urls "${lines}")
    foreach(line ${urls})
        string(REGEX REPLACE "\n[ \t]*set\\(([A-Z0-9_]+)_URL +([^ \n)]+)\\)" "\\1;\\2" pair "${line}")
        list(GET pair 0 name)
        list(GET pair 1 url)
        if(NOT text MATCHES "set\\(${name}_SHA256 +([0-9a-f]+)\\)")
            continue()
        endif()
        set(old "${CMAKE_MATCH_1}")
        string(CONFIGURE "${url}" url)
        file(DOWNLOAD "${url}" "${download}" STATUS status)
        list(GET status 0 code)
        if(NOT code EQUAL 0)
            message(FATAL_ERROR "${url}: ${status}")
        endif()
        file(SHA256 "${download}" new)
        file(REMOVE "${download}")
        if(NOT new STREQUAL old)
            string(REGEX REPLACE "set\\(${name}_SHA256( +)${old}\\)" "set(${name}_SHA256\\1${new})" text "${text}")
            set(changed TRUE)
            message(STATUS "${name}_SHA256 ${new}")
        endif()
    endforeach()
    if(changed)
        file(WRITE "${module}" "${text}")
    endif()
endforeach()
