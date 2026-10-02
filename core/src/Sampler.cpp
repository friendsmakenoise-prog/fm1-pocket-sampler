#include "fm1/Sampler.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace fm1 {

void Sampler::setSample(const SampleBuffer* sample) noexcept {
    sample_ = sample;
    stopAllVoices();
}

void Sampler::setSlice(std::size_t index, Slice sliceValue) {
    if (index >= kMaxSlices) throw std::out_of_range("slice index");
    if (sample_ && sliceValue.endFrame > sample_->frames())
        sliceValue.endFrame = sample_->frames();
    slices_[index] = sliceValue;
}

const Slice& Sampler::slice(std::size_t index) const {
    if (index >= kMaxSlices) throw std::out_of_range("slice index");
    return slices_[index];
}

void Sampler::makeEqualSlices(std::size_t count) {
    if (!sample_ || sample_->empty()) return;
    count = std::clamp<std::size_t>(count, 1, kMaxSlices);
    const std::size_t block = sample_->frames() / count;
    for (std::size_t i = 0; i < count; ++i) {
        const auto start = i * block;
        const auto end = (i + 1 == count) ? sample_->frames() : (i + 1) * block;
        slices_[i] = Slice{start, end, 1.0f, 0.0f, PlaybackMode::OneShot};
    }
}

void Sampler::stopAllVoices() noexcept {
    for (auto& voice : voices_) voice.active = false;
}

void Sampler::noteOn(std::size_t sliceIndex, float velocity) {
    if (!sample_ || sliceIndex >= kMaxSlices || !slices_[sliceIndex].valid()) return;
    if (monophonic_) stopAllVoices();
    Voice& voice = voices_[nextVoice_++ % voices_.size()];
    voice.active = true;
    voice.sliceIndex = sliceIndex;
    voice.position = static_cast<double>(slices_[sliceIndex].startFrame);
    voice.velocity = std::clamp(velocity, 0.0f, 1.0f);
    voice.gateHeld = true;
}

void Sampler::noteOff(std::size_t sliceIndex) {
    for (auto& voice : voices_) {
        if (voice.active && voice.sliceIndex == sliceIndex) voice.gateHeld = false;
    }
}

void Sampler::render(float* output, std::size_t frames, uint32_t outputSampleRate) {
    renderInternal(output, frames, outputSampleRate, true);
}

void Sampler::renderAdd(float* output, std::size_t frames, uint32_t outputSampleRate) {
    renderInternal(output, frames, outputSampleRate, false);
}

void Sampler::renderInternal(float* output, std::size_t frames, uint32_t outputSampleRate, bool clearOutput) {
    if (clearOutput) std::fill(output, output + frames, 0.0f);
    if (!sample_ || sample_->empty() || outputSampleRate == 0) return;

    for (auto& voice : voices_) {
        if (!voice.active) continue;
        const Slice& s = slices_[voice.sliceIndex];
        const double pitchRatio = std::pow(2.0, static_cast<double>(s.semitones) / 12.0);
        const double increment = (static_cast<double>(sample_->sampleRate) / outputSampleRate) * pitchRatio;

        for (std::size_t frame = 0; frame < frames && voice.active; ++frame) {
            if (voice.position >= static_cast<double>(s.endFrame)) {
                if (s.mode == PlaybackMode::Loop && voice.gateHeld) {
                    voice.position = static_cast<double>(s.startFrame);
                } else {
                    voice.active = false;
                    break;
                }
            }
            if (s.mode == PlaybackMode::Gate && !voice.gateHeld) {
                voice.active = false;
                break;
            }

            const auto i0 = static_cast<std::size_t>(voice.position);
            const auto i1 = std::min(i0 + 1, sample_->frames() - 1);
            const float frac = static_cast<float>(voice.position - static_cast<double>(i0));
            const float value = sample_->mono[i0] + (sample_->mono[i1] - sample_->mono[i0]) * frac;
            output[frame] += value * s.gain * voice.velocity;
            voice.position += increment;
        }
    }

    if (clearOutput) {
        for (std::size_t i = 0; i < frames; ++i)
            output[i] = std::clamp(output[i], -1.0f, 1.0f);
    }
}

} // namespace fm1
