# Asserts the operator documentation actually reached a built plugin bundle.
# Run per format by CTest with FROGGERS_VST_DOC_DIR pointing at that format's
# Resources directory, which differs between a macOS bundle and a Windows VST3
# directory -- the requirement does not.
if(NOT DEFINED FROGGERS_VST_DOC_DIR)
    message(FATAL_ERROR "FROGGERS_VST_DOC_DIR must be set")
endif()

foreach(FROGGERS_DOC MANUAL.md QUICK_DICT.md)
    set(FROGGERS_DOC_PATH "${FROGGERS_VST_DOC_DIR}/${FROGGERS_DOC}")
    if(NOT EXISTS "${FROGGERS_DOC_PATH}")
        message(FATAL_ERROR "operator documentation missing from the built plugin: ${FROGGERS_DOC_PATH}")
    endif()
    file(SIZE "${FROGGERS_DOC_PATH}" FROGGERS_DOC_SIZE)
    # A zero-byte copy satisfies EXISTS and ships nothing readable.
    if(FROGGERS_DOC_SIZE EQUAL 0)
        message(FATAL_ERROR "operator documentation is empty in the built plugin: ${FROGGERS_DOC_PATH}")
    endif()
endforeach()
