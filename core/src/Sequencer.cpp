#include "fm1/Sequencer.h"

#include <algorithm>

namespace fm1 {
namespace {
const StepEvent kEmptyEvent{};
}

void SequenceTrack::setLength(std::size_t steps) noexcept {
    length_ = std::clamp<std::size_t>(steps, 1, kMaxSteps);
}

void SequenceTrack::setEvent(std::size_t step, std::size_t sliceIndex, std::uint8_t velocity) noexcept {
    if (step >= kMaxSteps || sliceIndex >= 24) return;
    events_[step].active = true;
    events_[step].sliceIndex = static_cast<std::uint8_t>(sliceIndex);
    events_[step].velocity = std::clamp<std::uint8_t>(velocity, 1, 127);
}

void SequenceTrack::clearEvent(std::size_t step) noexcept {
    if (step >= kMaxSteps) return;
    events_[step] = StepEvent{};
}

void SequenceTrack::clearAll() noexcept {
    events_.fill(StepEvent{});
}

const StepEvent& SequenceTrack::event(std::size_t step) const noexcept {
    if (step >= kMaxSteps) return kEmptyEvent;
    return events_[step];
}

void Sequencer::setBpm(float bpm) noexcept {
    bpm_ = std::clamp(bpm, kMinBpm, kMaxBpm);
}

SequenceTrack& Sequencer::track(std::size_t index) noexcept {
    return tracks_[std::min<std::size_t>(index, kTrackCount - 1)];
}

const SequenceTrack& Sequencer::track(std::size_t index) const noexcept {
    return tracks_[std::min<std::size_t>(index, kTrackCount - 1)];
}

double Sequencer::framesPerStep(std::uint32_t sampleRate) const noexcept {
    if (sampleRate == 0 || bpm_ <= 0.0f) return 0.0;
    // v0.3 uses a shared 1/16-note grid: four sequencer steps per quarter note.
    return static_cast<double>(sampleRate) * 60.0 / (static_cast<double>(bpm_) * 4.0);
}

} // namespace fm1
