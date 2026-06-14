cmake_minimum_required(VERSION 3.16)
project(froggers_wasm C CXX)

set(CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src/core")
set(SIM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../sim")

add_executable(froggers bindings.cpp)
target_include_directories(froggers PRIVATE "${CORE_DIR}" "${SIM_DIR}")
target_compile_features(froggers PRIVATE cxx_std_17)

set(EXPORTED_FUNCS
    _froggers_create
    _froggers_destroy
    _froggers_set_sample_rate
    _froggers_set_knob
    _froggers_set_vco_morph
    _froggers_get_vco_morph
    _froggers_get_vco_display_morph
    _froggers_cycle_vco_morph
    _froggers_randomize_vco_morphs
    _froggers_nudge_vco3_morph
    _froggers_set_row_mod_source
    _froggers_get_row_mod_source
    _froggers_set_row_mod_depth
    _froggers_get_row_mod_depth
    _froggers_mod_level
    _froggers_page_next
    _froggers_page_prev
    _froggers_select_page
    _froggers_marbles
    _froggers_randomize_all_pages
    _froggers_randomize_all_mod
    _froggers_randomize_page
    _froggers_randomize_page_mod
    _froggers_process
    _froggers_process_stereo
    _froggers_row_name
    _froggers_mod_source_name
    _froggers_push_midi_cc
    _froggers_assignable_mod_count
    _froggers_assignable_mod_index
    _froggers_mod_source_available
    _froggers_set_cc_pair_enabled
    _froggers_cc_pair_enabled
    _froggers_row_value
    _froggers_row_badge
    _froggers_current_page
    _froggers_num_pages
    _froggers_delay_set_knob
    _froggers_delay_get_knob
    _froggers_delay_set_row_mod_source
    _froggers_delay_get_row_mod_source
    _froggers_delay_set_row_mod_depth
    _froggers_delay_get_row_mod_depth
    _froggers_delay_row_name
    _froggers_delay_randomize_knobs
    _froggers_delay_randomize_mod
    _froggers_copy_scope_samples
    _malloc
    _free
)

set(EXPORTED_FUNCS_JSON "")
foreach(func ${EXPORTED_FUNCS})
    string(APPEND EXPORTED_FUNCS_JSON "'${func}',")
endforeach()
string(REGEX REPLACE ",$" "" EXPORTED_FUNCS_JSON "${EXPORTED_FUNCS_JSON}")

target_link_options(froggers PRIVATE
    "-sWASM=1"
    "-sSTANDALONE_WASM=1"
    "--no-entry"
    "-sEXPORTED_FUNCTIONS=[${EXPORTED_FUNCS_JSON}]"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sERROR_ON_UNDEFINED_SYMBOLS=0"
    "-O3"
)

set_target_properties(froggers PROPERTIES OUTPUT_NAME "froggers")
