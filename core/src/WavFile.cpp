#include "fm1/WavFile.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace fm1 {
namespace {
uint16_t readU16(std::istream& in) {
    uint8_t b[2]{};
    in.read(reinterpret_cast<char*>(b), 2);
    return static_cast<uint16_t>(b[0] | (b[1] << 8));
}

uint32_t readU32(std::istream& in) {
    uint8_t b[4]{};
    in.read(reinterpret_cast<char*>(b), 4);
    return static_cast<uint32_t>(b[0]) |
           (static_cast<uint32_t>(b[1]) << 8) |
           (static_cast<uint32_t>(b[2]) << 16) |
           (static_cast<uint32_t>(b[3]) << 24);
}
}

SampleBuffer WavFile::load16BitPcm(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Could not open WAV file: " + path);

    char riff[4]{};
    in.read(riff, 4);
    (void)readU32(in);
    char wave[4]{};
    in.read(wave, 4);
    if (std::strncmp(riff, "RIFF", 4) != 0 || std::strncmp(wave, "WAVE", 4) != 0)
        throw std::runtime_error("Not a RIFF/WAVE file");

    uint16_t audioFormat = 0;
    uint16_t channels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    std::vector<int16_t> pcm;

    while (in && !in.eof()) {
        char id[4]{};
        in.read(id, 4);
        if (in.gcount() != 4) break;
        const uint32_t size = readU32(in);

        if (std::strncmp(id, "fmt ", 4) == 0) {
            audioFormat = readU16(in);
            channels = readU16(in);
            sampleRate = readU32(in);
            (void)readU32(in); // byte rate
            (void)readU16(in); // block align
            bitsPerSample = readU16(in);
            if (size > 16) in.seekg(size - 16, std::ios::cur);
        } else if (std::strncmp(id, "data", 4) == 0) {
            if (size % 2 != 0) throw std::runtime_error("Odd-sized 16-bit PCM data chunk");
            pcm.resize(size / 2);
            in.read(reinterpret_cast<char*>(pcm.data()), size);
        } else {
            in.seekg(size, std::ios::cur);
        }
        if (size & 1U) in.seekg(1, std::ios::cur);
    }

    if (audioFormat != 1 || (channels != 1 && channels != 2) || bitsPerSample != 16 || sampleRate == 0 || pcm.empty())
        throw std::runtime_error("v0.1 supports mono/stereo 16-bit PCM WAV only");

    SampleBuffer result;
    result.name = path;
    result.sampleRate = sampleRate;

    const std::size_t frameCount = pcm.size() / channels;
    result.mono.resize(frameCount);
    constexpr float scale = 1.0f / 32768.0f;

    for (std::size_t i = 0; i < frameCount; ++i) {
        if (channels == 1) {
            result.mono[i] = static_cast<float>(pcm[i]) * scale;
        } else {
            const float l = static_cast<float>(pcm[i * 2]) * scale;
            const float r = static_cast<float>(pcm[i * 2 + 1]) * scale;
            result.mono[i] = (l + r) * 0.5f;
        }
    }
    return result;
}

} // namespace fm1
