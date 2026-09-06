cmake_minimum_required(VERSION 3.16)
project(froggers_firmware_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../src/core")

function(add_firmware_test name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE "${CORE_DIR}")
    target_compile_definitions(${name} PRIVATE FROGGERS_TEST_INSTRUMENTATION)
endfunction()

# This target asserts the Daisy firmware's DSP directly out of src/core, with
# only src/core on the include path, so it compiles with nothing else in the
# repository present.

add_firmware_test(EngineDeterminism_test EngineDeterminism_test.cpp)
add_firmware_test(ModMgr_test ModMgr_test.cpp)
add_firmware_test(PairArEnvelope_test PairArEnvelope_test.cpp)

# One binary, both variants: each variant's engine is a separate translation
# unit because the variant is a build-time define, and asserting them together
# is what stops an edit to one silently moving the other.
add_firmware_test(RefreshGate_test RefreshGate_test.cpp)
add_firmware_test(VariantMix_test
    VariantMix_test.cpp
    VariantMix_solo.cpp
    VariantMix_guitar.cpp)

enable_testing()
add_test(NAME EngineDeterminism_test COMMAND EngineDeterminism_test)
add_test(NAME ModMgr_test COMMAND ModMgr_test)
add_test(NAME PairArEnvelope_test COMMAND PairArEnvelope_test)
add_test(NAME RefreshGate_test COMMAND RefreshGate_test)
add_test(NAME VariantMix_test COMMAND VariantMix_test)
