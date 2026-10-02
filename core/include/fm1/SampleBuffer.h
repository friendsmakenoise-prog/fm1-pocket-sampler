#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace fm1 {

struct SampleBuffer {
    std::string name;
    uint32_t sampleRate = 44100;
    std::vector<float> mono;

    [[nodiscard]] bool empty() const noexcept { return mono.empty(); }
    [[nodiscard]] std::size_t frames() const noexcept { return mono.size(); }
    [[nodiscard]] double durationSeconds() const noexcept {
        return sampleRate == 0 ? 0.0 : static_cast<double>(mono.size()) / sampleRate;
    }
};

} // namespace fm1
