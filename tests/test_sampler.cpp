#include "fm1/Sampler.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

int main() {
    fm1::SampleBuffer sample;
    sample.sampleRate = 44100;
    sample.mono.resize(44100);
    for (std::size_t i = 0; i < sample.mono.size(); ++i)
        sample.mono[i] = std::sin(2.0 * 3.141592653589793 * 440.0 * static_cast<double>(i) / 44100.0);

    fm1::Sampler sampler;
    sampler.setSample(&sample);
    sampler.makeEqualSlices(4);

    assert(sampler.slice(0).startFrame == 0);
    assert(sampler.slice(0).endFrame == 11025);
    assert(sampler.slice(3).endFrame == 44100);

    sampler.noteOn(0);
    std::vector<float> out(256, 0.0f);
    sampler.render(out.data(), out.size(), 44100);

    bool nonZero = false;
    for (float v : out) if (std::abs(v) > 0.0001f) nonZero = true;
    assert(nonZero);

    // renderAdd must preserve an existing mix buffer rather than clearing it.
    std::vector<float> mixed(256, 0.25f);
    sampler.noteOn(0);
    sampler.renderAdd(mixed.data(), mixed.size(), 44100);
    bool changedAboveBase = false;
    for (float v : mixed) if (std::abs(v - 0.25f) > 0.0001f) changedAboveBase = true;
    assert(changedAboveBase);

    // The B-Boy sampler defaults to mono per track, but can be switched to poly.
    sampler.setMonophonic(true);
    assert(sampler.monophonic());
    sampler.noteOn(0);
    sampler.noteOn(1); // should steal/cut the previous voice on this sampler track.
    sampler.setMonophonic(false);
    assert(!sampler.monophonic());
    sampler.noteOn(0);
    sampler.noteOn(1); // now both voices may overlap.

    // Master/track tuning is additive to per-slice tuning and clamped to a
    // sensible hardware-oriented range.
    sampler.setGlobalSemitones(12.0f);
    assert(std::abs(sampler.globalSemitones() - 12.0f) < 0.0001f);
    sampler.setGlobalSemitones(99.0f);
    assert(std::abs(sampler.globalSemitones() - 36.0f) < 0.0001f);
    sampler.setGlobalSemitones(0.0f);

    sampler.stopAllVoices();

    std::cout << "Sampler tests passed\n";
    return 0;
}
