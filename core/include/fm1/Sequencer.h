#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace fm1 {

struct StepEvent {
    bool active = false;
    std::uint8_t sliceIndex = 0;
    std::uint8_t velocity = 127;
};

class SequenceTrack {
public:
    static constexpr std::size_t kMaxSteps = 64;

    void setLength(std::size_t steps) noexcept;
    [[nodiscard]] std::size_t length() const noexcept { return length_; }

    void setEvent(std::size_t step, std::size_t sliceIndex, std::uint8_t velocity = 127) noexcept;
    void clearEvent(std::size_t step) noexcept;
    void clearAll() noexcept;
    [[nodiscard]] const StepEvent& event(std::size_t step) const noexcept;

    void setMuted(bool muted) noexcept { muted_ = muted; }
    [[nodiscard]] bool muted() const noexcept { return muted_; }

private:
    std::array<StepEvent, kMaxSteps> events_{};
    std::size_t length_ = 16;
    bool muted_ = false;
};

class Sequencer {
public:
    static constexpr std::size_t kTrackCount = 3;
    static constexpr float kMinBpm = 40.0f;
    static constexpr float kMaxBpm = 240.0f;

    void setBpm(float bpm) noexcept;
    [[nodiscard]] float bpm() const noexcept { return bpm_; }

    [[nodiscard]] SequenceTrack& track(std::size_t index) noexcept;
    [[nodiscard]] const SequenceTrack& track(std::size_t index) const noexcept;

    [[nodiscard]] double framesPerStep(std::uint32_t sampleRate) const noexcept;

private:
    std::array<SequenceTrack, kTrackCount> tracks_{};
    float bpm_ = 90.0f;
};

} // namespace fm1
