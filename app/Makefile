# app/Makefile — build rules for the Froggers-on-Sheaf app.
#
# The build-skeleton target ($(APP), built from Main.cpp) proves the include
# path into Sheaf's synth
# core resolves and that an empty translation unit links against Sheaf's
# static core library (build/libsynth.a, built by Sheaf's own Makefile at
# External/Sheaf/projects/synth/Makefile:89). This Makefile does NOT include Sheaf's
# runtime/juce_build.mk — that lives with the out-of-tree app-registration
# hook (FroggersRegistration.hpp) and the launcher build instead
# (see app/build-launcher.sh); nothing here touches the
# sheaf-patch launcher or JUCE headers.
#
# Path anchoring: resolved relative to this Makefile's own location (mirrors
# apps/miniapp/Makefile's APP_DIR pattern), so the build works regardless of
# the caller's cwd.

APP_DIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
SHEAF_SYNTH_DIR := $(APP_DIR)/../External/Sheaf/projects/synth

CXX ?= clang++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -O2
CPPFLAGS := -I$(SHEAF_SYNTH_DIR)/include

BUILD_DIR := $(APP_DIR)/build
APP_NAME := froggers_skeleton
APP := $(BUILD_DIR)/$(APP_NAME)
APP_SOURCES := $(APP_DIR)/Main.cpp

SHEAF_LIB := $(SHEAF_SYNTH_DIR)/build/libsynth.a

# --- headless FroggersApp test -------------------------------------
# synth_rig::SynthRig lives under Sheaf's tests/ tree; add it to the include
# path so FroggersHeadlessTests.cpp's `#include "support/SynthRig.hpp"`
# resolves (it is a quoted include, so without this it would search relative
# to app/, not Sheaf's tests/ directory).
TEST_SOURCES := $(APP_DIR)/FroggersHeadlessTests.cpp
TEST_BIN := $(BUILD_DIR)/froggers_headless_tests
TEST_CPPFLAGS := $(CPPFLAGS) -I$(SHEAF_SYNTH_DIR)/tests

# --- mono-assumption validation rig (stop-and-ask gate) -------------
# Standalone throwaway validation app (not FroggersApp) exercising one mono
# ParameterGroup + Bank + BankSlot + modulator + depth cell end-to-end.
MONO_VALIDATION_SOURCES := $(APP_DIR)/FroggersMonoValidationTests.cpp
MONO_VALIDATION_BIN := $(BUILD_DIR)/froggers_mono_validation_tests

# --- parameter model (mono, 16-slot banks) tests ---------------
# Structural checks use a bare ParameterManager + FroggersParameterModel;
# scene-blend and shared-Crunchy runtime checks (a stop-and-ask
# gate) drive FroggersApp through synth_rig::SynthRig, same as the
# headless/mono-validation test binaries below.
PARAMETER_MODEL_SOURCES := $(APP_DIR)/FroggersParameterModelTests.cpp
PARAMETER_MODEL_BIN := $(BUILD_DIR)/froggers_parameter_model_tests

# --- modulation slate + drill-in + randomize + default patch --
# Bare ParameterManager + FroggersParameterModel + FroggersModulationSlate,
# same structural-check convention as the parameter-model tests above -- no
# Engine/SynthRig needed (FroggersHeadlessTests.cpp already covers the
# FroggersApp::ProcessBlock/PrepareToPlay wiring end-to-end).
MODULATION_SOURCES := $(APP_DIR)/FroggersModulationTests.cpp
MODULATION_BIN := $(BUILD_DIR)/froggers_modulation_tests

# --- audio routing (parameters -> DSP -> stereo output) tests --------
# Drives FroggersApp through synth_rig::SynthRig, same convention as the
# test binaries above -- ProcessBlock is the thing under
# test, so this needs the real Engine/SynthRig headless harness, not the
# bare-model convention DSP_TEST_BIN/MODULATION_BIN's structural checks use.
# Also covers tempo-change
# tracking, missing-clock-plan safety, and tempo-tracking tests.
AUDIO_ROUTING_SOURCES := $(APP_DIR)/FroggersAudioRoutingTests.cpp
AUDIO_ROUTING_BIN := $(BUILD_DIR)/froggers_audio_routing_tests

# --- VCO scope + transfer-function visualizer tests -------------
# Scope-channel-bound/connected-flag checks need a real FroggersApp
# (constructor wiring + ProcessBlock), so this uses the same
# synth_rig::SynthRig convention as AUDIO_ROUTING_BIN; the transfer-function
# bounded-plot check is a standalone Visualizer/UIState check needing no rig.
VISUALIZER_SOURCES := $(APP_DIR)/FroggersVisualizerTests.cpp
VISUALIZER_BIN := $(BUILD_DIR)/froggers_visualizer_tests

# --- scope AdvanceIndex() bug + regression tests ----------------
# Guards against the missing ScopeWriter::AdvanceIndex() call: a
# reader-non-empty check plus a stronger
# regression guard (published index strictly advances across blocks). Same
# synth_rig::SynthRig convention as AUDIO_ROUTING_BIN/VISUALIZER_BIN -- the
# scope wiring lives in FroggersApp's own constructor/ProcessBlock.
SCOPE_ADVANCE_INDEX_SOURCES := $(APP_DIR)/FroggersScopeAdvanceIndexTests.cpp
SCOPE_ADVANCE_INDEX_BIN := $(BUILD_DIR)/froggers_scope_advance_index_tests

# --- Marbles clock/rate + no-source-level-control tests ---------
# Bare ParameterManager + FroggersParameterModel + FroggersModulationSlate,
# same structural-check convention as MODULATION_BIN -- clock/tempo values
# are driven directly (FroggersModulationSlate::Step/PrepareBlockClock take
# a plain std::optional<double>/std::optional<double>, no real MasterClock
# or AudioBlock needed to exercise the per-source rate logic).
MARBLES_CLOCK_SOURCES := $(APP_DIR)/FroggersMarblesClockTests.cpp
MARBLES_CLOCK_BIN := $(BUILD_DIR)/froggers_marbles_clock_tests

# --- surface layout tests -------------------------------------------
# Layout-bounds/no-overlap/cell-containment checks are pure math
# (FroggersPageLayout/FroggersEncoderGridLayout, no Engine/SynthRig); the
# in-place drill-in swap, encoder-ring-shows-fuegoized-value, BPM
# normal/external-clock-slaved states, and randomize-control-inventory
# checks drive FroggersApp through synth_rig::SynthRig, same convention as
# AUDIO_ROUTING_BIN/VISUALIZER_BIN, dispatching actions through the real
# FroggersUiSurface rather than reaching into the app model directly.
SURFACE_SOURCES := $(APP_DIR)/FroggersSurfaceTests.cpp
SURFACE_BIN := $(BUILD_DIR)/froggers_surface_tests

# --- mechanically enforce no-JUCE-in-app-core ----------------------
# check_no_juce.cpp includes only Froggers.hpp, compiled with JUCE's module
# directory ON the include path (so "we never pointed at JUCE" can't
# masquerade as passing) and asserts JUCE_MAJOR_VERSION stays undefined.
JUCE_DIR ?= $(HOME)/JUCE
NO_JUCE_CHECK_SRC := $(APP_DIR)/check_no_juce.cpp
NO_JUCE_CHECK := $(BUILD_DIR)/check_no_juce

# --- DSP port (pure, dependency-free, own test suite) ------
# app/dsp/*.hpp are copies of Froggers DSP formulas -- no
# #include of src/ or desktop-v2/, and no Sheaf/JUCE dependency at
# all: the port is deliberately pure DSP. DSP_HEADERS is
# listed as a prerequisite so editing any ported unit rebuilds the suite.
DSP_HEADERS := $(wildcard $(APP_DIR)/dsp/*.hpp)
#
DSP_TEST_SOURCES := $(APP_DIR)/FroggersDspParityTests.cpp
DSP_TEST_BIN := $(BUILD_DIR)/froggers_dsp_parity_tests

.PHONY: all clean check-no-juce check-no-frozen-deps test force

all: check-no-frozen-deps $(APP)

$(BUILD_DIR):
	mkdir -p $@

# Sheaf's own Makefile owns the object list for its static core library;
# delegate to it rather than duplicating that knowledge here. $(MAKE) (not a
# literal "make") is required so the recursive invocation shares the caller's
# job server instead of running unbounded parallel jobs of its own.
force:

$(SHEAF_LIB): force
	$(MAKE) -C $(SHEAF_SYNTH_DIR) build

# The `force` prerequisite makes the delegation run every invocation, so a
# genuine Sheaf change is always picked up. Make re-stats $(SHEAF_LIB) after
# the recipe returns, so $(APP) relinks only when libsynth.a's mtime actually
# moved -- not merely because the sub-make ran.
$(APP): $(APP_SOURCES) $(SHEAF_LIB) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(APP_SOURCES) $(SHEAF_LIB) -o $@

# --- mechanical frozen-tree-include guard --------------------
# Fails the build if any file under app/ (**/*.hpp, **/*.cpp) #includes
# from src/ -- the DSP port is a sanctioned copy, not a shared
# header, so that tree may never be reached from app/ code, however the
# include path is spelled.
check-no-frozen-deps:
	@bash $(APP_DIR)/check_no_frozen_includes.sh "$(APP_DIR)"

# --- microphone usage string drift guard -------------------------------------
# The sentence is declared once per platform because the two build systems
# share no substrate; this fails when they stop agreeing. See the script's own
# header for why the duplication cannot be collapsed.
check-microphone-usage:
	@bash $(APP_DIR)/check_microphone_usage_strings.sh "$(APP_DIR)"

# --- no-JUCE-in-app-core target ---------------------------------------------
$(NO_JUCE_CHECK): $(NO_JUCE_CHECK_SRC) $(APP_DIR)/Froggers.hpp $(APP_DIR)/FroggersAppCore.hpp $(APP_DIR)/FroggersUiSurface.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) -I$(JUCE_DIR)/modules $(CXXFLAGS) $(NO_JUCE_CHECK_SRC) -o $@

check-no-juce: $(NO_JUCE_CHECK)
	@echo "check-no-juce: OK - Froggers.hpp (app core) resolves no JUCE header"

# --- headless test target -------------------------------------------------------------
$(TEST_BIN): $(TEST_SOURCES) $(APP_DIR)/Froggers.hpp $(APP_DIR)/FroggersAppCore.hpp $(APP_DIR)/FroggersUiSurface.hpp $(SHEAF_LIB) | $(BUILD_DIR)
	$(CXX) $(TEST_CPPFLAGS) $(CXXFLAGS) $(TEST_SOURCES) $(SHEAF_LIB) -o $@

# --- mono-validation target -------------------------------------------------------------
$(MONO_VALIDATION_BIN): $(MONO_VALIDATION_SOURCES) $(APP_DIR)/Froggers.hpp $(APP_DIR)/FroggersAppCore.hpp $(APP_DIR)/FroggersUiSurface.hpp $(SHEAF_LIB) | $(BUILD_DIR)
	$(CXX) $(TEST_CPPFLAGS) $(CXXFLAGS) $(MONO_VALIDATION_SOURCES) $(SHEAF_LIB) -o $@

# --- parameter-model target ----------------------------------------------------------
$(PARAMETER_MODEL_BIN): $(PARAMETER_MODEL_SOURCES) $(APP_DIR)/Froggers.hpp $(APP_DIR)/FroggersAppCore.hpp $(APP_DIR)/FroggersUiSurface.hpp $(APP_DIR)/FroggersParameters.hpp $(SHEAF_LIB) | $(BUILD_DIR)
	$(CXX) $(TEST_CPPFLAGS) $(CXXFLAGS) $(PARAMETER_MODEL_SOURCES) $(SHEAF_LIB) -o $@

# --- modulation-slate target ------------------------------------------------
$(MODULATION_BIN): $(MODULATION_SOURCES) $(APP_DIR)/FroggersModulation.hpp $(APP_DIR)/FroggersParameters.hpp $(DSP_HEADERS) $(SHEAF_LIB) | $(BUILD_DIR)
	$(CXX) $(TEST_CPPFLAGS) $(CXXFLAGS) $(MODULATION_SOURCES) $(SHEAF_LIB) -o $@

# --- audio-routing target ------------------------------------------------------------
$(AUDIO_ROUTING_BIN): $(AUDIO_ROUTING_SOURCES) $(APP_DIR)/Froggers.hpp $(APP_DIR)/FroggersAppCore.hpp $(APP_DIR)/FroggersUiSurface.hpp $(APP_DIR)/FroggersModulation.hpp $(APP_DIR)/FroggersParameters.hpp $(DSP_HEADERS) $(SHEAF_LIB) | $(BUILD_DIR)
	$(CXX) $(TEST_CPPFLAGS) $(CXXFLAGS) $(AUDIO_ROUTING_SOURCES) $(SHEAF_LIB) -o $@

# --- visualizer target ---------------------------------------------------------
$(VISUALIZER_BIN): $(VISUALIZER_SOURCES) $(APP_DIR)/Froggers.hpp $(APP_DIR)/FroggersAppCore.hpp $(APP_DIR)/FroggersUiSurface.hpp $(APP_DIR)/FroggersModulation.hpp $(APP_DIR)/FroggersParameters.hpp $(APP_DIR)/FroggersTransferFunctionVisualizer.hpp $(DSP_HEADERS) $(SHEAF_LIB) | $(BUILD_DIR)
	$(CXX) $(TEST_CPPFLAGS) $(CXXFLAGS) $(VISUALIZER_SOURCES) $(SHEAF_LIB) -o $@

# --- scope-advance-index target ----------------------------------------------------------
$(SCOPE_ADVANCE_INDEX_BIN): $(SCOPE_ADVANCE_INDEX_SOURCES) $(APP_DIR)/Froggers.hpp $(APP_DIR)/FroggersAppCore.hpp $(APP_DIR)/FroggersUiSurface.hpp $(APP_DIR)/FroggersModulation.hpp $(APP_DIR)/FroggersParameters.hpp $(DSP_HEADERS) $(SHEAF_LIB) | $(BUILD_DIR)
	$(CXX) $(TEST_CPPFLAGS) $(CXXFLAGS) $(SCOPE_ADVANCE_INDEX_SOURCES) $(SHEAF_LIB) -o $@

# --- marbles-clock target ----------------------------------------------------------
$(MARBLES_CLOCK_BIN): $(MARBLES_CLOCK_SOURCES) $(APP_DIR)/FroggersModulation.hpp $(APP_DIR)/FroggersParameters.hpp $(APP_DIR)/FroggersRandomShVisualizer.hpp $(DSP_HEADERS) $(SHEAF_LIB) | $(BUILD_DIR)
	$(CXX) $(TEST_CPPFLAGS) $(CXXFLAGS) $(MARBLES_CLOCK_SOURCES) $(SHEAF_LIB) -o $@

# --- DSP parity suite target -----------------------------------------
# app/dsp/*.hpp's usual convention: NO Sheaf/JUCE dependency and no -I into
# Sheaf at all (pure, standalone C++). The VCO scope
# UIState and ResonantBump/Comb transfer-function UIState
# deliberately widen this by exactly one Sheaf -I: both mirror Sheaf's own
# DSP-struct-embeds-UIState convention (DspOscillators.hpp/DspFilters.hpp),
# which needs synth/DspScope.hpp, synth/Color.hpp, synth/AtomicColor.hpp,
# and synth/DspTransferFunction.hpp -- every one of them header-only with no
# further transitive Sheaf includes and no link-time dependency, so this
# still needs no SHEAF_LIB. See app/dsp/Vco.hpp's and app/dsp/FilterFx.hpp's
# own header comments for the full justification.
$(DSP_TEST_BIN): $(DSP_TEST_SOURCES) $(DSP_HEADERS) | $(BUILD_DIR)
	$(CXX) -I$(APP_DIR) $(CPPFLAGS) $(CXXFLAGS) $(DSP_TEST_SOURCES) -o $@

# --- stop-flush-repro target -----------------------------------------------------------

# --- surface target -----------------------------------------------------------
$(SURFACE_BIN): $(SURFACE_SOURCES) $(APP_DIR)/Froggers.hpp $(APP_DIR)/FroggersAppCore.hpp $(APP_DIR)/FroggersUiSurface.hpp $(APP_DIR)/FroggersParameters.hpp $(APP_DIR)/FroggersModulation.hpp $(DSP_HEADERS) $(SHEAF_LIB) | $(BUILD_DIR)
	$(CXX) $(TEST_CPPFLAGS) $(CXXFLAGS) $(SURFACE_SOURCES) $(SHEAF_LIB) -o $@

test: check-no-juce check-no-frozen-deps check-microphone-usage $(TEST_BIN) $(MONO_VALIDATION_BIN) $(DSP_TEST_BIN) $(PARAMETER_MODEL_BIN) $(MODULATION_BIN) $(AUDIO_ROUTING_BIN) $(VISUALIZER_BIN) $(SCOPE_ADVANCE_INDEX_BIN) $(MARBLES_CLOCK_BIN) $(SURFACE_BIN)
	$(TEST_BIN)
	$(MONO_VALIDATION_BIN)
	$(DSP_TEST_BIN)
	$(PARAMETER_MODEL_BIN)
	$(MODULATION_BIN)
	$(AUDIO_ROUTING_BIN)
	$(VISUALIZER_BIN)
	$(SCOPE_ADVANCE_INDEX_BIN)
	$(MARBLES_CLOCK_BIN)
	$(SURFACE_BIN)

clean:
	rm -rf $(BUILD_DIR)
