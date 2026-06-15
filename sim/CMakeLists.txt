cmake_minimum_required(VERSION 3.16)
project(froggers_sim_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src/core")
set(SIM_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

function(add_sim_test name source)
    add_executable(${name} ${source})
    target_include_directories(${name} PRIVATE "${CORE_DIR}" "${SIM_DIR}")
endfunction()

add_sim_test(Fuegoize_test Fuegoize_test.cpp)
add_sim_test(HookIdentity_test HookIdentity_test.cpp)
add_sim_test(PairArEnvelope_test PairArEnvelope_test.cpp)
add_sim_test(AudioPairArEffective_test AudioPairArEffective_test.cpp)
add_sim_test(DelayCrispyMod_test DelayCrispyMod_test.cpp)
add_sim_test(HostRandomize_test HostRandomize_test.cpp)

enable_testing()
add_test(NAME Fuegoize_test COMMAND Fuegoize_test)
add_test(NAME HookIdentity_test COMMAND HookIdentity_test)
add_test(NAME PairArEnvelope_test COMMAND PairArEnvelope_test)
add_test(NAME AudioPairArEffective_test COMMAND AudioPairArEffective_test)
add_test(NAME DelayCrispyMod_test COMMAND DelayCrispyMod_test)
add_test(NAME HostRandomize_test COMMAND HostRandomize_test)
