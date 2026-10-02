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

    std::cout << "Sampler tests passed\n";
    return 0;
}
