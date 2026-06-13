#include "ParamDisplayNames.hpp"
#include "WasmSimHost.hpp"

#include <cstdlib>
#include <cstring>

extern "C" {

WasmSimHost* froggers_create()
{
    WasmSimHost* host = new WasmSimHost();
    return host;
}

void froggers_destroy(WasmSimHost* host)
{
    delete host;
}

void froggers_set_sample_rate(WasmSimHost* host, float sampleRate)
{
    if (host)
    {
        host->setSampleRate(sampleRate);
    }
}

void froggers_set_knob(WasmSimHost* host, int index, float value)
{
    if (host && index >= 0 && index < 8)
    {
        host->io.SetKnob(static_cast<size_t>(index), value);
    }
}

void froggers_set_vco_morph(WasmSimHost* host, int index, float value)
{
    if (host)
    {
        host->io.SetVcoMorph(static_cast<size_t>(index), value);
    }
}

float froggers_get_vco_morph(WasmSimHost* host, int index)
{
    if (!host)
    {
        return 0.0f;
    }
    return host->io.GetVcoMorph(static_cast<size_t>(index));
}

float froggers_get_vco_display_morph(WasmSimHost* host, int index)
{
    if (!host)
    {
        return 0.0f;
    }
    return host->io.GetVcoDisplayMorph(static_cast<size_t>(index));
}

void froggers_randomize_vco_morphs(WasmSimHost* host)
{
    if (host)
    {
        host->io.RandomizeVcoMorphs();
    }
}

void froggers_nudge_vco3_morph(WasmSimHost* host)
{
    if (host)
    {
        host->io.NudgeVco3Morph();
    }
}

void froggers_cycle_vco_morph(WasmSimHost* host, int index)
{
    if (host)
    {
        host->io.CycleVcoMorph(static_cast<size_t>(index));
    }
}

void froggers_set_row_mod_source(WasmSimHost* host, int row, int modIndex)
{
    if (host)
    {
        host->io.SetRowModSource(static_cast<size_t>(row), static_cast<uint8_t>(modIndex));
    }
}

int froggers_get_row_mod_source(WasmSimHost* host, int row)
{
    if (!host)
    {
        return 255;
    }
    return static_cast<int>(host->io.GetRowModSource(static_cast<size_t>(row)));
}

void froggers_set_row_mod_depth(WasmSimHost* host, int row, float depth)
{
    if (host)
    {
        host->io.SetRowModDepth(static_cast<size_t>(row), depth);
    }
}

float froggers_get_row_mod_depth(WasmSimHost* host, int row)
{
    if (!host)
    {
        return 0.0f;
    }
    return host->io.GetRowModDepth(static_cast<size_t>(row));
}

float froggers_mod_level(WasmSimHost* host, int modIndex)
{
    if (!host)
    {
        return 0.0f;
    }
    return host->io.GetCvOut(static_cast<size_t>(modIndex));
}

void froggers_page_next(WasmSimHost* host)
{
    if (host)
    {
        host->io.PulsePageNext();
    }
}

void froggers_page_prev(WasmSimHost* host)
{
    if (host)
    {
        host->io.PulsePagePrevious();
    }
}

void froggers_select_page(WasmSimHost* host, int page)
{
    if (host && page >= 0 && page < static_cast<int>(host->io.GetNumPages()))
    {
        host->selectPage(static_cast<uint8_t>(page));
    }
}

void froggers_marbles(WasmSimHost* host)
{
    if (host)
    {
        host->io.PulseMarbles();
    }
}

void froggers_randomize_all_pages(WasmSimHost* host)
{
    if (host)
    {
        host->randomizeAllIncludingDelay();
    }
}

void froggers_randomize_all_mod(WasmSimHost* host)
{
    if (host)
    {
        host->randomizeAllModIncludingDelay();
    }
}

void froggers_randomize_page(WasmSimHost* host, int page)
{
    if (host && page >= 0 && page < static_cast<int>(host->io.GetNumPages()))
    {
        host->io.RandomizePage(static_cast<uint8_t>(page));
    }
}

void froggers_randomize_page_mod(WasmSimHost* host, int page)
{
    if (host && page >= 0 && page < static_cast<int>(host->io.GetNumPages()))
    {
        host->io.RandomizePageMod(static_cast<uint8_t>(page));
    }
}

void froggers_process(WasmSimHost* host, const float* in, float* out, int n)
{
    if (!host || !in || !out || n <= 0)
    {
        return;
    }
    host->processBlock(in, out, out, static_cast<size_t>(n), 1);
}

void froggers_process_stereo(WasmSimHost* host, const float* in, float* outL, float* outR, int n)
{
    if (!host || !in || !outL || n <= 0)
    {
        return;
    }
    host->processBlock(in, outL, outR, static_cast<size_t>(n), outR ? 2 : 1);
}

const char* froggers_row_name(WasmSimHost* host, int row)
{
    if (!host)
    {
        return "";
    }
    return ParamDisplayNames::forHostPageRow(
        static_cast<uint8_t>(host->io.GetCurrentPage()), static_cast<uint8_t>(row));
}

float froggers_row_value(WasmSimHost* host, int row)
{
    if (!host)
    {
        return 0.0f;
    }
    return host->io.GetRowValue(static_cast<size_t>(row));
}

char froggers_row_badge(WasmSimHost* host, int row)
{
    if (!host)
    {
        return ' ';
    }
    return host->io.GetRowTrackingBadge(static_cast<size_t>(row));
}

int froggers_current_page(WasmSimHost* host)
{
    if (!host)
    {
        return 0;
    }
    return static_cast<int>(host->io.GetCurrentPage());
}

int froggers_num_pages(WasmSimHost* host)
{
    if (!host)
    {
        return 0;
    }
    return static_cast<int>(host->io.GetNumPages());
}

void froggers_delay_set_knob(WasmSimHost* host, int row, float value)
{
    if (host && row >= 0 && row < 8)
    {
        host->delay.setKnob(static_cast<uint8_t>(row), value);
    }
}

float froggers_delay_get_knob(WasmSimHost* host, int row)
{
    if (!host || row < 0 || row >= 8)
    {
        return 0.0f;
    }
    return host->delay.getKnob(static_cast<uint8_t>(row));
}

void froggers_delay_set_row_mod_source(WasmSimHost* host, int row, int modIndex)
{
    if (host && row >= 0 && row < 8)
    {
        host->delay.setModSource(static_cast<uint8_t>(row), static_cast<uint8_t>(modIndex));
    }
}

int froggers_delay_get_row_mod_source(WasmSimHost* host, int row)
{
    if (!host || row < 0 || row >= 8)
    {
        return 255;
    }
    return static_cast<int>(host->delay.getModSource(static_cast<uint8_t>(row)));
}

void froggers_delay_set_row_mod_depth(WasmSimHost* host, int row, float depth)
{
    if (host && row >= 0 && row < 8)
    {
        host->delay.setModDepth(static_cast<uint8_t>(row), depth);
    }
}

float froggers_delay_get_row_mod_depth(WasmSimHost* host, int row)
{
    if (!host || row < 0 || row >= 8)
    {
        return 0.0f;
    }
    return host->delay.getModDepth(static_cast<uint8_t>(row));
}

const char* froggers_delay_row_name(WasmSimHost* host, int row)
{
    if (!host || row < 0 || row >= 8)
    {
        return "";
    }
    return ParamDisplayNames::forHostPageRow(ParamDisplayNames::kDelayHostPage, static_cast<uint8_t>(row));
}

void froggers_delay_randomize_knobs(WasmSimHost* host)
{
    if (host)
    {
        host->delay.randomizeKnobs();
    }
}

void froggers_delay_randomize_mod(WasmSimHost* host)
{
    if (host)
    {
        host->delay.randomizeMod();
    }
}

int froggers_copy_scope_samples(WasmSimHost* host, int modIndex, float* out, int maxCount)
{
    if (!host || !out || maxCount <= 0)
    {
        return 0;
    }
    return static_cast<int>(host->copyScopeSamples(static_cast<uint8_t>(modIndex), out,
                                                   static_cast<size_t>(maxCount)));
}

}
