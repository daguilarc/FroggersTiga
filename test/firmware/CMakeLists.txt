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
# only src/core on the include path. It stays free of any host shim so it
# keeps compiling regardless of what happens to the simulator tree.
#
# Two of the simulator's assertions are deliberately absent.
#
# V2IndependentPm_test's only assertion of live firmware behavior is a
# flag-off golden-pin check, and that check only compiles against
# DesktopHostIO and PagedHostIO. Keeping those host shims around to preserve
# one assertion is worse than losing the assertion.
#
# Fuegoize_test compared the simulator's free-function fuegoize against the
# firmware's inline one in Parameter::Get and asserted the two agree. The
# firmware has no fuegoize header of its own, so with the simulator copy gone
# the comparison has nothing on its other side.

add_firmware_test(HookIdentity_test HookIdentity_test.cpp)
add_firmware_test(ModMgr_test ModMgr_test.cpp)
add_firmware_test(PairArEnvelope_test PairArEnvelope_test.cpp)

enable_testing()
add_test(NAME HookIdentity_test COMMAND HookIdentity_test)
add_test(NAME ModMgr_test COMMAND ModMgr_test)
add_test(NAME PairArEnvelope_test COMMAND PairArEnvelope_test)
