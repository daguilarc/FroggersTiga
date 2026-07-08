if(NOT DEFINED RELEASE_BUNDLE OR NOT DEFINED STALE_BUNDLE)
    message(FATAL_ERROR "RELEASE_BUNDLE and STALE_BUNDLE must be set")
endif()

if(NOT EXISTS "${STALE_BUNDLE}")
    return()
endif()

if("${STALE_BUNDLE}" STREQUAL "${RELEASE_BUNDLE}")
    return()
endif()

get_filename_component(stale_real "${STALE_BUNDLE}" REALPATH)
get_filename_component(release_real "${RELEASE_BUNDLE}" REALPATH)
if(stale_real STREQUAL release_real)
    return()
endif()

message(STATUS "Removing stale FroggersTigaV2.app at ${STALE_BUNDLE}")
file(REMOVE_RECURSE "${STALE_BUNDLE}")
