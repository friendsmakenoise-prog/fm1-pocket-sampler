#include "fm1/Sampler.h"
#include "fm1/WavFile.h"

#include <iomanip>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
    std::cout << "FM-1 Pocket Sampler desktop harness v0.1\n";
    if (argc < 2) {
        std::cout << "Usage: fm1_desktop <sample.wav>\n";
        std::cout << "Loads a WAV and creates 8 equal slices. Audio-device output comes next.\n";
        return 0;
    }

    try {
        auto sample = fm1::WavFile::load16BitPcm(argv[1]);
        fm1::Sampler sampler;
        sampler.setSample(&sample);
        sampler.makeEqualSlices(8);

        std::cout << "Loaded: " << sample.name << "\n"
                  << "Sample rate: " << sample.sampleRate << " Hz\n"
                  << "Frames: " << sample.frames() << "\n"
                  << "Duration: " << std::fixed << std::setprecision(2)
                  << sample.durationSeconds() << " sec\n"
                  << "Float memory: " << (sample.frames() * sizeof(float)) / 1024.0 << " KiB\n\n";

        for (std::size_t i = 0; i < 8; ++i) {
            const auto& s = sampler.slice(i);
            std::cout << "Slice " << (i + 1) << ": " << s.startFrame << " -> " << s.endFrame << "\n";
        }

        sampler.noteOn(0);
        std::vector<float> scratch(512);
        sampler.render(scratch.data(), scratch.size(), 44100);
        std::cout << "\nRendered first slice into a 512-frame scratch buffer successfully.\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
