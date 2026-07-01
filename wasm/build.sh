#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-$ROOT/web/public/froggers.wasm}"

EXPORTS='[
  "_froggers_create","_froggers_destroy",
  "_froggers_set_sample_rate","_froggers_set_knob",
  "_froggers_set_vco_morph","_froggers_get_vco_morph","_froggers_get_vco_display_morph",
  "_froggers_cycle_vco_morph","_froggers_randomize_vco_morphs","_froggers_nudge_vco3_morph",
  "_froggers_set_row_mod_source","_froggers_get_row_mod_source",
  "_froggers_set_row_mod_depth","_froggers_get_row_mod_depth",
  "_froggers_mod_level",
  "_froggers_page_next","_froggers_page_prev","_froggers_select_page","_froggers_marbles",
  "_froggers_randomize_all_pages","_froggers_randomize_all_mod",
  "_froggers_process","_froggers_process_stereo",
  "_froggers_row_name","_froggers_row_value","_froggers_row_badge",
  "_froggers_current_page","_froggers_num_pages",
  "_froggers_delay_set_knob","_froggers_delay_get_knob","_froggers_delay_get_effective_knob",
  "_froggers_delay_set_row_mod_source","_froggers_delay_get_row_mod_source",
  "_froggers_delay_set_row_mod_depth","_froggers_delay_get_row_mod_depth",
  "_froggers_delay_row_name","_froggers_delay_randomize_knobs","_froggers_delay_randomize_mod",
  "_froggers_set_cc_pair_enabled","_froggers_cc_pair_enabled",
  "_froggers_set_audio_pair_ar_knob","_froggers_get_audio_pair_ar_knob",
  "_froggers_get_audio_pair_ar_effective",
  "_froggers_set_audio_pair_ar_mod_source","_froggers_get_audio_pair_ar_mod_source",
  "_froggers_set_audio_pair_ar_mod_depth","_froggers_get_audio_pair_ar_mod_depth",
  "_froggers_audio_pair_ar_name",
  "_froggers_set_global_crunchy","_froggers_get_global_crunchy",
  "_froggers_assignable_mod_count","_froggers_assignable_mod_index",
  "_froggers_mod_source_available","_froggers_mod_source_name",
  "_froggers_push_midi_cc",
  "_froggers_randomize_page","_froggers_randomize_page_mod",
  "_malloc","_free"
]'

em++ "$ROOT/wasm/bindings.cpp" \
  -I"$ROOT/src/core" \
  -I"$ROOT/sim" \
  -std=c++17 \
  -O3 \
  -s WASM=1 \
  -s STANDALONE_WASM=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s ERROR_ON_UNDEFINED_SYMBOLS=0 \
  -s "EXPORTED_FUNCTIONS=${EXPORTS}" \
  -o "$OUT"

echo "Wrote $OUT"
