cmake_minimum_required(VERSION 3.16)
project(froggers_firmware_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../src/core")

function(add_firmware_test name source)
    add_executable(${name} ${source})
    target_include_directories(${name} PRIVATE "${CORE_DIR}")
endfunction()

# This target asserts the Daisy firmware's DSP directly out of src/core, with
# only src/core on the include path, so it compiles with nothing else in the
# repository present.

add_firmware_test(HookIdentity_test HookIdentity_test.cpp)
add_firmware_test(ModMgr_test ModMgr_test.cpp)
add_firmware_test(PairArEnvelope_test PairArEnvelope_test.cpp)

enable_testing()
add_test(NAME HookIdentity_test COMMAND HookIdentity_test)
add_test(NAME ModMgr_test COMMAND ModMgr_test)
add_test(NAME PairArEnvelope_test COMMAND PairArEnvelope_test)
