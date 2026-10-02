#pragma once

#include "fm1/SampleBuffer.h"
#include "fm1/Slice.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace fm1 {

class Sampler {
public:
    static constexpr std::size_t kMaxSlices = 24;
    static constexpr std::size_t kMaxVoices = 8;

    void setSample(const SampleBuffer* sample) noexcept;
    void setSlice(std::size_t index, Slice slice);
    [[nodiscard]] const Slice& slice(std::size_t index) const;

    void makeEqualSlices(std::size_t count);
    void noteOn(std::size_t sliceIndex, float velocity = 1.0f);
    void noteOff(std::size_t sliceIndex);
    void stopAllVoices() noexcept;
    void setMonophonic(bool enabled) noexcept { monophonic_ = enabled; }
    [[nodiscard]] bool monophonic() const noexcept { return monophonic_; }

    void render(float* output, std::size_t frames, uint32_t outputSampleRate);
    void renderAdd(float* output, std::size_t frames, uint32_t outputSampleRate);

private:
    struct Voice {
        bool active = false;
        std::size_t sliceIndex = 0;
        double position = 0.0;
        float velocity = 1.0f;
        bool gateHeld = false;
    };

    void renderInternal(float* output, std::size_t frames, uint32_t outputSampleRate, bool clearOutput);

    const SampleBuffer* sample_ = nullptr;
    std::array<Slice, kMaxSlices> slices_{};
    std::array<Voice, kMaxVoices> voices_{};
    std::size_t nextVoice_ = 0;
    bool monophonic_ = true;
};

} // namespace fm1
