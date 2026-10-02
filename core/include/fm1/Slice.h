#pragma once

#include <cstddef>
#include <cstdint>

namespace fm1 {

enum class PlaybackMode : uint8_t {
    OneShot,
    Gate,
    Loop
};

struct Slice {
    std::size_t startFrame = 0;
    std::size_t endFrame = 0; // exclusive
    float gain = 1.0f;
    float semitones = 0.0f;
    PlaybackMode mode = PlaybackMode::OneShot;

    [[nodiscard]] bool valid() const noexcept { return endFrame > startFrame; }
};

} // namespace fm1
