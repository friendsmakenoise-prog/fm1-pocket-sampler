#include "fm1/Sampler.h"
#include "fm1/WavFile.h"

#include "raylib.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr int kInitialWidth = 1320;
constexpr int kInitialHeight = 840;
constexpr std::size_t kMaxSlices = fm1::Sampler::kMaxSlices;
constexpr std::size_t kTrackCount = 3;
constexpr uint32_t kOutputSampleRate = 44100;
constexpr double kBootDurationSeconds = 1.65;

// Orange FM-1 palette.
constexpr Color kBodyOrange{244, 74, 26, 255};
constexpr Color kBodyOrangeDark{193, 54, 25, 255};
constexpr Color kButtonOrange{167, 55, 34, 255};
constexpr Color kButtonOrangeActive{111, 39, 28, 255};
constexpr Color kPanelInk{35, 24, 21, 255};
constexpr Color kCream{247, 234, 219, 255};
constexpr Color kScreenBg{14, 20, 19, 255};
constexpr Color kScreenGreen{171, 209, 184, 255};
constexpr Color kScreenDim{91, 125, 103, 255};
constexpr Color kMarker{255, 211, 78, 255};

std::recursive_mutex gAudioMutex;
std::atomic<float> gMasterVolume{0.82f};

enum class ChopMode : int { Equal8 = 0, Equal16, Equal24, Manual };
enum class ScreenPage : int { Home = 0, Edit, Fx, Env, Lfo, Arp, Seq, Global };
enum class EditStage : int { Trim = 0, Chop };
enum class AuditionMode : int { None = 0, Pre, Gate, OneShot, Loop, Tail };

struct TrackState {
    fm1::SampleBuffer sample;
    fm1::Sampler sampler;
    std::size_t activeSlices = 16;
    ChopMode chopMode = ChopMode::Equal16;
    std::vector<std::size_t> manualMarkers;
    std::size_t masterStart = 0;
    std::size_t masterEnd = 0;
    std::size_t selectedSlice = 0;
    std::size_t viewStart = 0;
    std::size_t viewEnd = 0;
    bool mono = true;
    bool linkLength = false;
    std::atomic<bool> previewPlaying{false};
    double previewPosition = 0.0;
    std::size_t previewStart = 0;
    std::size_t previewEnd = 0;
    AuditionMode auditionMode = AuditionMode::None;
    bool auditionGateHeld = false;
    std::string sourceName = "EMPTY";
};

struct UiState {
    std::size_t activeTrack = 0;
    ScreenPage page = ScreenPage::Edit;
    EditStage editStage = EditStage::Trim;
    bool punchArmed = false;
    bool autoAudition = true;
    bool panning = false;
    Vector2 panAnchorMouse{};
    std::size_t panAnchorStart = 0;
    std::size_t panAnchorEnd = 0;
    int auditionKeyCode = 0;
    bool auditionFromMouse = false;
    std::string status = "Drop audio onto SAMPLE A to begin.";
};

std::array<TrackState, kTrackCount> gTracks;
UiState gUi;

const std::array<int, kMaxSlices> kTriggerKeys = {
    // Mirrors the FM-1's F-based chromatic keybed on a two-row QWERTY layout.
    // F F# G G# A A# B C C# D D# E | F F# G G# A A# B C C# D D# E
    KEY_Z, KEY_S, KEY_X, KEY_D, KEY_C, KEY_F,
    KEY_V, KEY_B, KEY_H, KEY_N, KEY_J, KEY_M,
    KEY_Q, KEY_TWO, KEY_W, KEY_THREE, KEY_E, KEY_FIVE,
    KEY_R, KEY_T, KEY_SIX, KEY_Y, KEY_SEVEN, KEY_U
};

const std::array<std::size_t, 5> kMasterAuditionPhysicalKeys = {0, 2, 4, 6, 7};
const std::array<const char*, 5> kMasterAuditionLabels = {"PRE", "GATE", "1SHOT", "LOOP", "TAIL"};

TrackState& activeTrack() { return gTracks[gUi.activeTrack]; }
const TrackState& activeTrackConst() { return gTracks[gUi.activeTrack]; }

const char* trackLetter(std::size_t index) {
    static constexpr std::array<const char*, 3> labels{"A", "B", "C"};
    return labels[std::min<std::size_t>(index, 2)];
}

const char* chopModeLabel(ChopMode mode) {
    switch (mode) {
        case ChopMode::Equal8: return "8";
        case ChopMode::Equal16: return "16";
        case ChopMode::Equal24: return "24";
        case ChopMode::Manual: return "MAN";
    }
    return "?";
}

const char* auditionModeLabel(AuditionMode mode) {
    switch (mode) {
        case AuditionMode::Pre: return "PRE";
        case AuditionMode::Gate: return "GATE";
        case AuditionMode::OneShot: return "1SHOT";
        case AuditionMode::Loop: return "LOOP";
        case AuditionMode::Tail: return "TAIL";
        case AuditionMode::None: break;
    }
    return "STOP";
}

int masterAuditionOrdinal(std::size_t physicalKeyIndex) {
    for (std::size_t i = 0; i < kMasterAuditionPhysicalKeys.size(); ++i)
        if (kMasterAuditionPhysicalKeys[i] == physicalKeyIndex) return static_cast<int>(i);
    return -1;
}

AuditionMode auditionModeForOrdinal(int ordinal) {
    switch (ordinal) {
        case 0: return AuditionMode::Pre;
        case 1: return AuditionMode::Gate;
        case 2: return AuditionMode::OneShot;
        case 3: return AuditionMode::Loop;
        case 4: return AuditionMode::Tail;
        default: return AuditionMode::None;
    }
}

std::string fileNameOnly(const std::string& path) {
    try { return std::filesystem::path(path).filename().string(); }
    catch (...) { return path; }
}

std::string extensionLower(const std::string& path) {
    std::string ext;
    try { ext = std::filesystem::path(path).extension().string(); }
    catch (...) { return {}; }
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return ext;
}

bool isSupportedDesktopAudio(const std::string& path) {
    const auto ext = extensionLower(path);
    return ext == ".wav" || ext == ".mp3" || ext == ".flac" || ext == ".ogg";
}

fm1::SampleBuffer loadWithRaylib(const std::string& path) {
    Wave wave = LoadWave(path.c_str());
    if (wave.data == nullptr || wave.frameCount == 0 || wave.sampleRate == 0 || wave.channels == 0) {
        if (wave.data != nullptr) UnloadWave(wave);
        throw std::runtime_error("Raylib could not decode this audio file");
    }
    float* samples = LoadWaveSamples(wave);
    if (samples == nullptr) {
        UnloadWave(wave);
        throw std::runtime_error("Decoded audio could not be converted to float samples");
    }

    fm1::SampleBuffer result;
    result.name = path;
    result.sampleRate = wave.sampleRate;
    result.mono.resize(wave.frameCount);
    const std::size_t channels = std::max<unsigned int>(1U, wave.channels);
    for (std::size_t frame = 0; frame < wave.frameCount; ++frame) {
        double sum = 0.0;
        for (std::size_t channel = 0; channel < channels; ++channel)
            sum += samples[frame * channels + channel];
        result.mono[frame] = static_cast<float>(sum / static_cast<double>(channels));
    }
    UnloadWaveSamples(samples);
    UnloadWave(wave);
    return result;
}

void invalidateUnusedSlices(TrackState& t, std::size_t count) {
    for (std::size_t i = count; i < kMaxSlices; ++i) t.sampler.setSlice(i, fm1::Slice{});
}

void makeEqualSlices(TrackState& t, std::size_t count) {
    std::lock_guard<std::recursive_mutex> lock(gAudioMutex);
    if (t.sample.empty()) return;
    count = std::clamp<std::size_t>(count, 1, kMaxSlices);
    const std::size_t start = std::min(t.masterStart, t.sample.frames());
    const std::size_t end = std::clamp(t.masterEnd, start + 1, t.sample.frames());
    const std::size_t length = end - start;

    for (std::size_t i = 0; i < count; ++i) {
        auto old = t.sampler.slice(i);
        const std::size_t s = start + (length * i) / count;
        const std::size_t e = (i + 1 == count) ? end : start + (length * (i + 1)) / count;
        old.startFrame = s;
        old.endFrame = std::max(s + 1, e);
        if (old.gain <= 0.0f) old.gain = 1.0f;
        t.sampler.setSlice(i, old);
    }
    t.activeSlices = count;
    invalidateUnusedSlices(t, count);
    t.selectedSlice = std::min(t.selectedSlice, count - 1);
}

void syncManualSlices(TrackState& t) {
    std::lock_guard<std::recursive_mutex> lock(gAudioMutex);
    if (t.sample.empty()) return;
    const std::size_t start = t.masterStart;
    const std::size_t end = t.masterEnd;
    if (t.manualMarkers.size() < 2) t.manualMarkers = {start, end};

    for (auto& marker : t.manualMarkers) marker = std::clamp(marker, start, end);
    std::sort(t.manualMarkers.begin(), t.manualMarkers.end());
    t.manualMarkers.erase(std::unique(t.manualMarkers.begin(), t.manualMarkers.end()), t.manualMarkers.end());
    if (t.manualMarkers.empty() || t.manualMarkers.front() != start) t.manualMarkers.insert(t.manualMarkers.begin(), start);
    if (t.manualMarkers.back() != end) t.manualMarkers.push_back(end);
    while (t.manualMarkers.size() > kMaxSlices + 1) t.manualMarkers.erase(t.manualMarkers.end() - 2);

    t.activeSlices = std::min<std::size_t>(kMaxSlices, t.manualMarkers.size() - 1);
    for (std::size_t i = 0; i < t.activeSlices; ++i) {
        auto old = t.sampler.slice(i);
        old.startFrame = t.manualMarkers[i];
        old.endFrame = t.manualMarkers[i + 1];
        if (old.gain <= 0.0f) old.gain = 1.0f;
        t.sampler.setSlice(i, old);
    }
    invalidateUnusedSlices(t, t.activeSlices);
    t.selectedSlice = std::min(t.selectedSlice, t.activeSlices - 1);
}

void rebuildChops(TrackState& t) {
    std::lock_guard<std::recursive_mutex> lock(gAudioMutex);
    if (t.sample.empty()) return;
    switch (t.chopMode) {
        case ChopMode::Equal8: makeEqualSlices(t, 8); break;
        case ChopMode::Equal16: makeEqualSlices(t, 16); break;
        case ChopMode::Equal24: makeEqualSlices(t, 24); break;
        case ChopMode::Manual: syncManualSlices(t); break;
    }
}

void setChopMode(TrackState& t, ChopMode mode) {
    std::lock_guard<std::recursive_mutex> lock(gAudioMutex);
    t.chopMode = mode;
    gUi.punchArmed = false;
    if (t.sample.empty()) return;
    if (mode == ChopMode::Manual) t.manualMarkers = {t.masterStart, t.masterEnd};
    rebuildChops(t);
    gUi.status = std::string("SAMPLE ") + trackLetter(gUi.activeTrack) + "  " + chopModeLabel(mode) + " chop mode.";
}

void cycleChopMode(int delta) {
    auto& t = activeTrack();
    int mode = static_cast<int>(t.chopMode);
    mode = (mode + delta) % 4;
    if (mode < 0) mode += 4;
    setChopMode(t, static_cast<ChopMode>(mode));
}

std::array<fm1::Slice, kMaxSlices> snapshotSlices(const TrackState& t) {
    std::lock_guard<std::recursive_mutex> lock(gAudioMutex);
    std::array<fm1::Slice, kMaxSlices> result{};
    for (std::size_t i = 0; i < result.size(); ++i) result[i] = t.sampler.slice(i);
    return result;
}

void resetView(TrackState& t) {
    t.viewStart = 0;
    t.viewEnd = t.sample.frames();
}

std::size_t selectedFocusFrame(const TrackState& t) {
    std::lock_guard<std::recursive_mutex> lock(gAudioMutex);
    if (t.sample.empty()) return 0;
    if (gUi.editStage == EditStage::Trim) return (t.masterStart + t.masterEnd) / 2;
    if (t.selectedSlice < t.activeSlices) {
        const auto s = t.sampler.slice(t.selectedSlice);
        if (s.valid()) return (s.startFrame + s.endFrame) / 2;
    }
    return (t.masterStart + t.masterEnd) / 2;
}

void centerViewOn(TrackState& t, std::size_t frame) {
    if (t.sample.empty()) return;
    const std::size_t total = t.sample.frames();
    if (t.viewEnd <= t.viewStart || t.viewEnd > total) resetView(t);
    const std::size_t span = t.viewEnd - t.viewStart;
    if (span >= total) { resetView(t); return; }
    frame = std::min(frame, total);
    long long start = static_cast<long long>(frame) - static_cast<long long>(span / 2);
    start = std::clamp<long long>(start, 0, static_cast<long long>(total - span));
    t.viewStart = static_cast<std::size_t>(start);
    t.viewEnd = t.viewStart + span;
}

void zoomView(TrackState& t, double factor, std::size_t anchorFrame) {
    if (t.sample.empty()) return;
    const std::size_t total = t.sample.frames();
    if (t.viewEnd <= t.viewStart || t.viewEnd > total) resetView(t);
    const std::size_t currentSpan = t.viewEnd - t.viewStart;
    const std::size_t minSpan = std::min<std::size_t>(total, std::max<std::size_t>(128, total / 2000));
    const std::size_t newSpan = static_cast<std::size_t>(std::clamp<double>(
        currentSpan * factor, static_cast<double>(minSpan), static_cast<double>(total)));
    if (newSpan >= total) { resetView(t); return; }
    anchorFrame = std::min(anchorFrame, total);
    long long start = static_cast<long long>(anchorFrame) - static_cast<long long>(newSpan / 2);
    start = std::clamp<long long>(start, 0, static_cast<long long>(total - newSpan));
    t.viewStart = static_cast<std::size_t>(start);
    t.viewEnd = t.viewStart + newSpan;
}

void panView(TrackState& t, long long deltaFrames) {
    if (t.sample.empty() || t.viewEnd <= t.viewStart) return;
    const std::size_t total = t.sample.frames();
    const std::size_t span = t.viewEnd - t.viewStart;
    long long start = static_cast<long long>(t.viewStart) + deltaFrames;
    start = std::clamp<long long>(start, 0, static_cast<long long>(total - std::min(span, total)));
    t.viewStart = static_cast<std::size_t>(start);
    t.viewEnd = std::min(total, t.viewStart + span);
}

std::size_t visibleBoundaryStep(const TrackState& t) {
    if (t.sample.empty()) return 1;
    const std::size_t span = t.viewEnd > t.viewStart ? t.viewEnd - t.viewStart : t.sample.frames();
    return std::max<std::size_t>(1, span / 240);
}

void selectTrack(std::size_t index) {
    if (index >= kTrackCount) return;
    gUi.activeTrack = index;
    gUi.punchArmed = false;
    auto& t = activeTrack();
    if (!t.sample.empty()) centerViewOn(t, selectedFocusFrame(t));
    gUi.status = std::string("SAMPLE ") + trackLetter(index) + " selected" + (t.sample.empty() ? " (empty)." : ".");
}

void triggerSlice(std::size_t index) {
    std::lock_guard<std::recursive_mutex> lock(gAudioMutex);
    auto& t = activeTrack();
    if (index >= t.activeSlices || t.sample.empty()) return;
    t.selectedSlice = index;
    t.sampler.noteOn(index, 1.0f);

    // On the hardware, pressing a chop key while on the edit page is the most
    // natural way to say "edit this chop". Switch to CHOP automatically so
    // the zoom anchor follows the selected slice instead of the master trim.
    if (gUi.page == ScreenPage::Edit) {
        gUi.editStage = EditStage::Chop;
        centerViewOn(t, selectedFocusFrame(t));
        gUi.status = TextFormat("S%02i selected - zoom follows chop.", static_cast<int>(index + 1));
    }
}

void autoAuditionSelected() {
    std::lock_guard<std::recursive_mutex> lock(gAudioMutex);
    auto& t = activeTrack();
    if (gUi.autoAudition && !t.sample.empty() && t.selectedSlice < t.activeSlices) t.sampler.noteOn(t.selectedSlice, 1.0f);
}

void nudgeSliceBoundary(long long deltaFrames, bool moveEnd) {
    std::lock_guard<std::recursive_mutex> lock(gAudioMutex);
    auto& t = activeTrack();
    if (t.sample.empty() || t.selectedSlice >= t.activeSlices) return;
    auto slice = t.sampler.slice(t.selectedSlice);
    if (!slice.valid()) return;
    const std::size_t length = slice.endFrame - slice.startFrame;

    if (t.linkLength) {
        if (!moveEnd) {
            long long newStart = static_cast<long long>(slice.startFrame) + deltaFrames;
            newStart = std::clamp<long long>(newStart, static_cast<long long>(t.masterStart),
                static_cast<long long>(t.masterEnd >= length ? t.masterEnd - length : t.masterStart));
            slice.startFrame = static_cast<std::size_t>(newStart);
            slice.endFrame = slice.startFrame + length;
        } else {
            long long newEnd = static_cast<long long>(slice.endFrame) + deltaFrames;
            newEnd = std::clamp<long long>(newEnd, static_cast<long long>(t.masterStart + length),
                                           static_cast<long long>(t.masterEnd));
            slice.endFrame = static_cast<std::size_t>(newEnd);
            slice.startFrame = slice.endFrame - length;
        }
    } else if (!moveEnd) {
        long long newStart = static_cast<long long>(slice.startFrame) + deltaFrames;
        newStart = std::clamp<long long>(newStart, static_cast<long long>(t.masterStart),
                                         static_cast<long long>(slice.endFrame - 1));
        slice.startFrame = static_cast<std::size_t>(newStart);
    } else {
        long long newEnd = static_cast<long long>(slice.endFrame) + deltaFrames;
        newEnd = std::clamp<long long>(newEnd, static_cast<long long>(slice.startFrame + 1),
                                       static_cast<long long>(t.masterEnd));
        slice.endFrame = static_cast<std::size_t>(newEnd);
    }
    t.sampler.setSlice(t.selectedSlice, slice);
    centerViewOn(t, moveEnd ? slice.endFrame : slice.startFrame);
    autoAuditionSelected();
}

void setSliceBoundaryAbsolute(std::size_t frame, bool moveEnd) {
    auto& t = activeTrack();
    if (t.selectedSlice >= t.activeSlices) return;
    const auto s = t.sampler.slice(t.selectedSlice);
    const std::size_t old = moveEnd ? s.endFrame : s.startFrame;
    nudgeSliceBoundary(static_cast<long long>(frame) - static_cast<long long>(old), moveEnd);
}

void nudgeMasterBoundary(long long deltaFrames, bool moveEnd) {
    std::lock_guard<std::recursive_mutex> lock(gAudioMutex);
    auto& t = activeTrack();
    if (t.sample.empty()) return;
    if (!moveEnd) {
        long long value = static_cast<long long>(t.masterStart) + deltaFrames;
        value = std::clamp<long long>(value, 0, static_cast<long long>(t.masterEnd - 1));
        t.masterStart = static_cast<std::size_t>(value);
        centerViewOn(t, t.masterStart);
    } else {
        long long value = static_cast<long long>(t.masterEnd) + deltaFrames;
        value = std::clamp<long long>(value, static_cast<long long>(t.masterStart + 1), static_cast<long long>(t.sample.frames()));
        t.masterEnd = static_cast<std::size_t>(value);
        centerViewOn(t, t.masterEnd);
    }
    if (t.chopMode == ChopMode::Manual) {
        if (t.manualMarkers.empty()) t.manualMarkers = {t.masterStart, t.masterEnd};
        syncManualSlices(t);
    } else {
        rebuildChops(t);
    }
    gUi.status = moveEnd ? "MASTER END adjusted." : "MASTER START adjusted.";
}

void tuneSelected(float delta) {
    std::lock_guard<std::recursive_mutex> lock(gAudioMutex);
    auto& t = activeTrack();
    if (t.selectedSlice >= t.activeSlices) return;
    auto s = t.sampler.slice(t.selectedSlice);
    s.semitones = std::clamp(s.semitones + delta, -36.0f, 36.0f);
    t.sampler.setSlice(t.selectedSlice, s);
    autoAuditionSelected();
}

void gainSelected(float delta) {
    std::lock_guard<std::recursive_mutex> lock(gAudioMutex);
    auto& t = activeTrack();
    if (t.selectedSlice >= t.activeSlices) return;
    auto s = t.sampler.slice(t.selectedSlice);
    s.gain = std::clamp(s.gain + delta, 0.0f, 2.0f);
    t.sampler.setSlice(t.selectedSlice, s);
    autoAuditionSelected();
}

void stopPreview(TrackState& t) {
    t.previewPlaying.store(false);
    t.auditionMode = AuditionMode::None;
    t.auditionGateHeld = false;
}

void stopAllPreviews() {
    for (auto& t : gTracks) stopPreview(t);
}

void startMasterAudition(AuditionMode mode, int keyCode = 0, bool fromMouse = false) {
    std::lock_guard<std::recursive_mutex> lock(gAudioMutex);
    auto& t = activeTrack();
    if (t.sample.empty() || mode == AuditionMode::None) return;

    stopAllPreviews();
    const std::size_t total = t.sample.frames();
    const std::size_t contextFrames = static_cast<std::size_t>(t.sample.sampleRate); // 1 second PRE/TAIL context.

    t.auditionMode = mode;
    t.auditionGateHeld = mode == AuditionMode::Gate || mode == AuditionMode::Loop;
    t.previewStart = t.masterStart;
    t.previewEnd = t.masterEnd;

    if (mode == AuditionMode::Pre) {
        if (t.masterStart == 0) {
            gUi.status = "PRE: no audio before MASTER START.";
            return;
        }
        t.previewStart = t.masterStart > contextFrames ? t.masterStart - contextFrames : 0;
        t.previewEnd = t.masterStart;
    } else if (mode == AuditionMode::Tail) {
        if (t.masterEnd >= total) {
            gUi.status = "TAIL: no audio after MASTER END.";
            return;
        }
        t.previewStart = t.masterEnd;
        t.previewEnd = std::min(total, t.masterEnd + contextFrames);
    }

    if (t.previewEnd <= t.previewStart) return;
    t.previewPosition = static_cast<double>(t.previewStart);
    t.previewPlaying.store(true);
    gUi.auditionKeyCode = keyCode;
    gUi.auditionFromMouse = fromMouse;
    gUi.status = std::string("MASTER AUDITION: ") + auditionModeLabel(mode) + ".";
}

void releaseMasterAuditionGateIfNeeded() {
    std::lock_guard<std::recursive_mutex> lock(gAudioMutex);
    auto& t = activeTrack();
    if (!t.previewPlaying.load()) return;
    if (t.auditionMode != AuditionMode::Gate && t.auditionMode != AuditionMode::Loop) return;

    const bool released = gUi.auditionFromMouse
        ? IsMouseButtonReleased(MOUSE_BUTTON_LEFT)
        : (gUi.auditionKeyCode != 0 && IsKeyReleased(gUi.auditionKeyCode));
    if (released) {
        t.auditionGateHeld = false;
        gUi.auditionKeyCode = 0;
        gUi.auditionFromMouse = false;
    }
}

void togglePreview() {
    std::lock_guard<std::recursive_mutex> lock(gAudioMutex);
    auto& t = activeTrack();
    if (t.sample.empty()) return;
    if (t.previewPlaying.load()) {
        stopPreview(t);
        gUi.status = "Preview stopped.";
    } else {
        startMasterAudition(AuditionMode::OneShot);
    }
}

std::size_t previewFrameSnapshot(const TrackState& t) {
    std::lock_guard<std::recursive_mutex> lock(gAudioMutex);
    if (t.sample.empty()) return 0;
    return std::min<std::size_t>(t.sample.frames(), static_cast<std::size_t>(t.previewPosition));
}

void insertManualMarker(std::size_t frame) {
    std::lock_guard<std::recursive_mutex> lock(gAudioMutex);
    auto& t = activeTrack();
    if (t.sample.empty()) return;
    if (t.chopMode != ChopMode::Manual) setChopMode(t, ChopMode::Manual);
    if (t.manualMarkers.size() >= kMaxSlices + 1) {
        gUi.status = "Manual chop limit reached (24 slices).";
        return;
    }
    frame = std::clamp(frame, t.masterStart + 1, t.masterEnd > 0 ? t.masterEnd - 1 : t.masterEnd);
    const std::size_t tolerance = std::max<std::size_t>(1, (t.masterEnd - t.masterStart) / 10000);
    for (auto marker : t.manualMarkers) {
        const auto distance = marker > frame ? marker - frame : frame - marker;
        if (distance <= tolerance) return;
    }
    t.manualMarkers.push_back(frame);
    std::sort(t.manualMarkers.begin(), t.manualMarkers.end());
    auto it = std::lower_bound(t.manualMarkers.begin(), t.manualMarkers.end(), frame);
    const std::size_t boundaryIndex = static_cast<std::size_t>(std::distance(t.manualMarkers.begin(), it));
    syncManualSlices(t);
    t.selectedSlice = boundaryIndex == 0 ? 0 : std::min<std::size_t>(t.activeSlices - 1, boundaryIndex - 1);
    centerViewOn(t, frame);
    gUi.status = "Punch marker added.";
}

void audioCallback(void* bufferData, unsigned int frames) {
    auto* output = static_cast<float*>(bufferData);
    std::lock_guard<std::recursive_mutex> lock(gAudioMutex);
    std::fill(output, output + frames, 0.0f);

    for (auto& t : gTracks) t.sampler.renderAdd(output, frames, kOutputSampleRate);

    for (auto& t : gTracks) {
        if (t.sample.empty() || !t.previewPlaying.load()) continue;
        const double increment = static_cast<double>(t.sample.sampleRate) / static_cast<double>(kOutputSampleRate);
        for (unsigned int frame = 0; frame < frames; ++frame) {
            if ((t.auditionMode == AuditionMode::Gate || t.auditionMode == AuditionMode::Loop) && !t.auditionGateHeld) {
                stopPreview(t);
                break;
            }
            if (t.previewPosition >= static_cast<double>(t.previewEnd)) {
                if (t.auditionMode == AuditionMode::Loop && t.auditionGateHeld) {
                    t.previewPosition = static_cast<double>(t.previewStart);
                } else {
                    stopPreview(t);
                    break;
                }
            }
            if (!t.previewPlaying.load()) break;
            const auto i0 = std::min<std::size_t>(static_cast<std::size_t>(t.previewPosition), t.sample.frames() - 1);
            const auto i1 = std::min(i0 + 1, t.sample.frames() - 1);
            const float frac = static_cast<float>(t.previewPosition - static_cast<double>(i0));
            const float value = t.sample.mono[i0] + (t.sample.mono[i1] - t.sample.mono[i0]) * frac;
            output[frame] += value * 0.75f;
            t.previewPosition += increment;
        }
    }

    for (unsigned int frame = 0; frame < frames; ++frame)
        output[frame] = std::clamp(output[frame] * gMasterVolume.load(), -1.0f, 1.0f);
}

std::string loadSampleIntoActiveTrack(const std::string& path) {
    try {
        auto loaded = loadWithRaylib(path);
        auto& t = activeTrack();
        const auto frames = loaded.frames();
        const auto rate = loaded.sampleRate;
        std::lock_guard<std::recursive_mutex> lock(gAudioMutex);
        t.sample = std::move(loaded);
        t.sampler.setSample(&t.sample);
        t.sampler.setMonophonic(t.mono);
        t.masterStart = 0;
        t.masterEnd = frames;
        t.selectedSlice = 0;
        t.manualMarkers.clear();
        stopPreview(t);
        t.previewPosition = 0.0;
        t.previewStart = 0;
        t.previewEnd = frames;
        t.viewStart = 0;
        t.viewEnd = frames;
        t.sourceName = fileNameOnly(path);
        rebuildChops(t);
        gUi.editStage = EditStage::Trim;
        gUi.page = ScreenPage::Edit;
        gUi.punchArmed = false;

        std::ostringstream message;
        message << "Loaded " << t.sourceName << " into SAMPLE " << trackLetter(gUi.activeTrack)
                << "  " << rate << "Hz  " << std::fixed << std::setprecision(2)
                << (rate == 0 ? 0.0 : static_cast<double>(frames) / rate) << "s";
        return message.str();
    } catch (const std::exception& e) {
        return std::string("Could not load audio: ") + e.what();
    }
}

bool pointInCircle(Vector2 point, Vector2 centre, float radius) {
    const float dx = point.x - centre.x;
    const float dy = point.y - centre.y;
    return dx * dx + dy * dy <= radius * radius;
}

int knobDelta(Vector2 centre, float radius, float mouseWheel) {
    const Vector2 mouse = GetMousePosition();
    if (!pointInCircle(mouse, centre, radius + 5.0f)) return 0;
    if (mouseWheel > 0.0f) return 1;
    if (mouseWheel < 0.0f) return -1;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return 1;
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) return -1;
    return 0;
}

bool buttonPressed(Rectangle r) {
    return CheckCollisionPointRec(GetMousePosition(), r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void drawButton(Rectangle r, const char* label, bool active = false, bool rec = false) {
    const bool hovered = CheckCollisionPointRec(GetMousePosition(), r);
    Color fill = active ? kButtonOrangeActive : kButtonOrange;
    if (rec && active) fill = Color{95, 28, 29, 255};
    if (hovered) fill = active ? Color{128, 45, 31, 255} : Color{188, 67, 43, 255};
    DrawRectangleRounded(r, 0.18f, 5, fill);
    DrawRectangleRoundedLinesEx(r, 0.18f, 5, 1.0f, Color{102, 39, 29, 255});
    const int fontSize = std::string(label).size() > 6 ? 9 : 12;
    const int textWidth = MeasureText(label, fontSize);
    DrawText(label, static_cast<int>(r.x + (r.width - textWidth) * 0.5f),
             static_cast<int>(r.y + (r.height - fontSize) * 0.5f - 1), fontSize, kCream);
}

void drawKnob(Vector2 centre, float radius, const char* label, float normalized = 0.5f, bool highlighted = false) {
    DrawCircleV(centre, radius + 3.0f, Color{92, 34, 25, 255});
    DrawCircleV(centre, radius, highlighted ? Color{39, 34, 29, 255} : Color{24, 24, 24, 255});
    DrawCircleLines(static_cast<int>(centre.x), static_cast<int>(centre.y), radius, Color{8, 8, 8, 255});
    const float angle = (-135.0f + std::clamp(normalized, 0.0f, 1.0f) * 270.0f) * DEG2RAD;
    const Vector2 p2{centre.x + std::cos(angle) * radius * 0.72f,
                     centre.y + std::sin(angle) * radius * 0.72f};
    DrawLineEx(centre, p2, 2.3f, kCream);
    const int fs = 12;
    const int tw = MeasureText(label, fs);
    DrawText(label, static_cast<int>(centre.x - tw * 0.5f), static_cast<int>(centre.y + radius + 7.0f), fs, kCream);
}

void drawGraffitiText(const char* textValue, Vector2 position, float fontSize, float reveal) {
    const Font font = GetFontDefault();
    const Vector2 size = MeasureTextEx(font, textValue, fontSize, 1.0f);
    const Rectangle clip{position.x - 16.0f, position.y - 18.0f,
                         std::max(0.0f, (size.x + 32.0f) * std::clamp(reveal, 0.0f, 1.0f)), size.y + 42.0f};
    BeginScissorMode(static_cast<int>(clip.x), static_cast<int>(clip.y),
                     static_cast<int>(clip.width), static_cast<int>(clip.height));
    for (int ox = -4; ox <= 4; ox += 2) {
        for (int oy = -4; oy <= 4; oy += 2) {
            if (ox == 0 && oy == 0) continue;
            DrawTextEx(font, textValue, Vector2{position.x + static_cast<float>(ox), position.y + static_cast<float>(oy)},
                       fontSize, 1.0f, Color{7, 7, 7, 255});
        }
    }
    DrawTextEx(font, textValue, position, fontSize, 1.0f, Color{244, 80, 29, 255});
    const int dotCount = 70;
    const float revealedWidth = size.x * std::clamp(reveal, 0.0f, 1.0f);
    for (int i = 0; i < dotCount; ++i) {
        const unsigned int seed = static_cast<unsigned int>(i * 1103515245U + 12345U);
        const float fx = static_cast<float>((seed >> 8U) % 1000U) / 1000.0f;
        const float fy = static_cast<float>((seed >> 18U) % 1000U) / 1000.0f;
        const float x = position.x - 10.0f + fx * (size.x + 20.0f);
        if (x > position.x + revealedWidth) continue;
        const float y = position.y - 12.0f + fy * (size.y + 30.0f);
        DrawCircleV(Vector2{x, y}, 0.8f + static_cast<float>(seed % 3U) * 0.55f, Color{244, 80, 29, 150});
    }
    EndScissorMode();
}

void drawBootScreen(double elapsed, int screenWidth, int screenHeight) {
    ClearBackground(Color{8, 9, 9, 255});
    const char* base = "M-VAVE FM-1";
    const int baseSize = 38;
    const int baseWidth = MeasureText(base, baseSize);
    const int centreX = screenWidth / 2;
    const int centreY = screenHeight / 2;
    DrawText(base, centreX - baseWidth / 2, centreY - 76, baseSize, Color{224, 224, 221, 255});
    if (elapsed > 0.38) {
        const float reveal = static_cast<float>(std::clamp((elapsed - 0.38) / 0.88, 0.0, 1.0));
        const char* graffiti = "B-BOY EDITION";
        const float size = 46.0f;
        const Vector2 measured = MeasureTextEx(GetFontDefault(), graffiti, size, 1.0f);
        const Vector2 pos{static_cast<float>(centreX) - measured.x * 0.5f + 10.0f,
                          static_cast<float>(centreY) - 5.0f};
        drawGraffitiText(graffiti, pos, size, reveal);
        if (reveal < 1.0f) {
            const float sprayX = pos.x + measured.x * reveal;
            DrawCircleV(Vector2{sprayX + 8.0f, pos.y + measured.y * 0.50f}, 4.5f, Color{244, 80, 29, 180});
            DrawCircleV(Vector2{sprayX + 15.0f, pos.y + measured.y * 0.37f}, 2.0f, Color{244, 80, 29, 130});
        }
    }
}

void drawWaveform(Rectangle bounds, const TrackState& t,
                  const std::array<fm1::Slice, kMaxSlices>& slices, bool punchArmed) {
    DrawRectangleRec(bounds, kScreenBg);
    DrawRectangleLinesEx(bounds, 1.0f, Color{72, 105, 86, 255});
    if (t.sample.empty()) {
        DrawText("DROP AUDIO", static_cast<int>(bounds.x + 16), static_cast<int>(bounds.y + bounds.height * 0.40f),
                 22, kScreenGreen);
        DrawText("WAV  MP3  FLAC  OGG", static_cast<int>(bounds.x + 16), static_cast<int>(bounds.y + bounds.height * 0.58f),
                 12, kScreenDim);
        return;
    }

    std::size_t viewStart = t.viewStart;
    std::size_t viewEnd = std::min(t.viewEnd, t.sample.frames());
    if (viewEnd <= viewStart) { viewStart = 0; viewEnd = t.sample.frames(); }
    const std::size_t span = viewEnd - viewStart;

    // Dim material outside the master trim window.
    if (t.masterStart > viewStart) {
        const auto edge = std::min(t.masterStart, viewEnd);
        const float x = bounds.x + bounds.width * static_cast<float>(edge - viewStart) / static_cast<float>(span);
        DrawRectangleRec(Rectangle{bounds.x, bounds.y, std::max(0.0f, x - bounds.x), bounds.height}, Color{4, 6, 6, 125});
    }
    if (t.masterEnd < viewEnd) {
        const auto edge = std::max(t.masterEnd, viewStart);
        const float x = bounds.x + bounds.width * static_cast<float>(edge - viewStart) / static_cast<float>(span);
        DrawRectangleRec(Rectangle{x, bounds.y, std::max(0.0f, bounds.x + bounds.width - x), bounds.height}, Color{4, 6, 6, 125});
    }

    if (gUi.editStage == EditStage::Chop && t.selectedSlice < t.activeSlices && slices[t.selectedSlice].valid()) {
        const auto s = slices[t.selectedSlice];
        const auto s0 = std::max(s.startFrame, viewStart);
        const auto s1 = std::min(s.endFrame, viewEnd);
        if (s1 > s0) {
            const float x0 = bounds.x + bounds.width * static_cast<float>(s0 - viewStart) / static_cast<float>(span);
            const float x1 = bounds.x + bounds.width * static_cast<float>(s1 - viewStart) / static_cast<float>(span);
            DrawRectangle(static_cast<int>(x0), static_cast<int>(bounds.y), std::max(1, static_cast<int>(x1 - x0)),
                          static_cast<int>(bounds.height), Color{93, 73, 24, 105});
        }
    }

    const int columns = std::max(1, static_cast<int>(bounds.width));
    const float centreY = bounds.y + bounds.height * 0.5f;
    const float amplitudeScale = bounds.height * 0.43f;
    for (int x = 0; x < columns; ++x) {
        const std::size_t start = viewStart + static_cast<std::size_t>((static_cast<double>(x) / columns) * span);
        const std::size_t end = std::min(viewEnd, viewStart + static_cast<std::size_t>((static_cast<double>(x + 1) / columns) * span) + 1);
        float peak = 0.0f;
        for (std::size_t i = start; i < end; ++i) peak = std::max(peak, std::abs(t.sample.mono[i]));
        DrawLine(static_cast<int>(bounds.x) + x, static_cast<int>(centreY - peak * amplitudeScale),
                 static_cast<int>(bounds.x) + x, static_cast<int>(centreY + peak * amplitudeScale), kScreenGreen);
    }

    auto drawFrameLine = [&](std::size_t frame, Color color, float thick = 1.0f) {
        if (frame < viewStart || frame > viewEnd) return;
        const float x = bounds.x + bounds.width * static_cast<float>(frame - viewStart) / static_cast<float>(span);
        DrawLineEx(Vector2{x, bounds.y}, Vector2{x, bounds.y + bounds.height}, thick, color);
    };

    drawFrameLine(t.masterStart, Color{246, 113, 63, 255}, 2.0f);
    drawFrameLine(t.masterEnd, Color{246, 113, 63, 255}, 2.0f);

    // Always show the chop map, even while MASTER TRIM is selected. That way
    // the user never loses sight of the 8/16/24/manual divisions. In CHOP edit
    // the selected slice is promoted to the yellow focus markers.
    for (std::size_t i = 0; i < t.activeSlices; ++i) {
        if (!slices[i].valid()) continue;
        const bool selected = gUi.editStage == EditStage::Chop && i == t.selectedSlice;
        const Color marker = selected ? kMarker
                                      : (gUi.editStage == EditStage::Chop
                                             ? Color{93, 145, 112, 225}
                                             : Color{76, 116, 94, 170});
        drawFrameLine(slices[i].startFrame, marker, selected ? 2.0f : 1.0f);
    }
    if (gUi.editStage == EditStage::Chop && t.selectedSlice < t.activeSlices && slices[t.selectedSlice].valid())
        drawFrameLine(slices[t.selectedSlice].endFrame, kMarker, 2.0f);

    const std::size_t preview = previewFrameSnapshot(t);
    if (t.previewPlaying.load() && preview >= viewStart && preview <= viewEnd)
        drawFrameLine(preview, punchArmed ? Color{226, 81, 73, 255} : Color{230, 232, 225, 220}, 2.0f);
}

struct PianoKeyVisual {
    std::size_t index = 0;
    Rectangle rect{};
    bool upper = false;
    int upperOrdinal = -1;
};

std::vector<PianoKeyVisual> buildPianoKeys(Rectangle bounds) {
    const std::array<int, 7> naturalPcs{0, 2, 4, 5, 7, 9, 11};
    auto isNatural = [&](int midi) {
        const int pc = midi % 12;
        return std::find(naturalPcs.begin(), naturalPcs.end(), pc) != naturalPcs.end();
    };
    constexpr int lowerCount = 16;
    const float lowerStep = bounds.width / static_cast<float>(lowerCount);
    const float lowerWidth = lowerStep * 0.78f;
    const float lowerHeight = bounds.height * 0.47f;
    const float lowerY = bounds.y + bounds.height * 0.48f;
    const float upperWidth = lowerStep * 0.68f;
    const float upperHeight = bounds.height * 0.30f;
    const float upperY = bounds.y + bounds.height * 0.05f;

    std::vector<PianoKeyVisual> keys;
    keys.reserve(27);
    int naturalOrdinal = 0;
    int upperOrdinal = 0;
    for (int i = 0; i < 27; ++i) {
        const int midi = 53 + i;
        if (isNatural(midi)) {
            const float cx = bounds.x + (naturalOrdinal + 0.5f) * lowerStep;
            keys.push_back(PianoKeyVisual{static_cast<std::size_t>(i),
                Rectangle{cx - lowerWidth * 0.5f, lowerY, lowerWidth, lowerHeight}, false, -1});
            ++naturalOrdinal;
        } else {
            const float cx = bounds.x + static_cast<float>(naturalOrdinal) * lowerStep;
            keys.push_back(PianoKeyVisual{static_cast<std::size_t>(i),
                Rectangle{cx - upperWidth * 0.5f, upperY, upperWidth, upperHeight}, true, upperOrdinal++});
        }
    }
    return keys;
}

bool handlePianoMouse(const std::vector<PianoKeyVisual>& keys, std::size_t& pressedIndex) {
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return false;
    const Vector2 mouse = GetMousePosition();
    for (const auto& key : keys) {
        if (CheckCollisionPointRec(mouse, key.rect)) { pressedIndex = key.index; return true; }
    }
    return false;
}

void drawPiano(const std::vector<PianoKeyVisual>& keys, const TrackState& t) {
    static constexpr std::array<const char*, 11> upperLabels = {
        "OP1", "OP2", "OP3", "OP4", "OP5", "OP6", "PIT", "GLO", "MONO", "POLY", ""
    };

    for (const auto& key : keys) {
        if (!key.upper) continue;
        const float ledY = key.rect.y + key.rect.height + 10.0f;
        bool selected = false;
        if (key.index < kMaxSlices) selected = key.index == t.selectedSlice && key.index < t.activeSlices;
        else selected = (key.index - kMaxSlices) == gUi.activeTrack;
        DrawCircleV(Vector2{key.rect.x + key.rect.width * 0.5f, ledY}, 2.8f,
                    selected ? kMarker : Color{87, 31, 24, 255});
        if (key.upperOrdinal >= 0 && key.upperOrdinal < static_cast<int>(upperLabels.size())) {
            const char* label = upperLabels[static_cast<std::size_t>(key.upperOrdinal)];
            if (label[0] != '\0') {
                const int fs = 9;
                DrawText(label, static_cast<int>(key.rect.x + (key.rect.width - MeasureText(label, fs)) * 0.5f),
                         static_cast<int>(key.rect.y - 13.0f), fs, kCream);
            }
        }
    }

    for (const auto& key : keys) {
        const bool trackKey = key.index >= kMaxSlices;
        const bool sliceAssigned = !trackKey && key.index < t.activeSlices;
        const int auditionOrdinal = gUi.editStage == EditStage::Trim && !trackKey ? masterAuditionOrdinal(key.index) : -1;
        const AuditionMode keyAuditionMode = auditionModeForOrdinal(auditionOrdinal);
        const bool auditionActive = auditionOrdinal >= 0 && t.previewPlaying.load() && t.auditionMode == keyAuditionMode;
        const bool selected = trackKey ? (key.index - kMaxSlices) == gUi.activeTrack
                                       : (gUi.editStage == EditStage::Trim ? auditionActive
                                                                          : (key.index == t.selectedSlice && sliceAssigned));
        Color fill = key.upper ? Color{148, 52, 34, 255} : Color{161, 57, 35, 255};
        if (!sliceAssigned && !trackKey) fill = Color{130, 48, 34, 255};
        if (trackKey) fill = selected ? Color{58, 44, 35, 255} : Color{83, 46, 37, 255};
        if (selected && !trackKey) fill = Color{72, 51, 31, 255};

        DrawRectangleRounded(key.rect, 0.46f, 10, fill);
        DrawRectangleRoundedLinesEx(key.rect, 0.46f, 10, selected ? 2.0f : 1.0f,
                                    selected ? kMarker : Color{111, 40, 29, 255});
        Rectangle stripe{key.rect.x + key.rect.width * 0.47f, key.rect.y + key.rect.height * 0.23f,
                         std::max(2.0f, key.rect.width * 0.07f), key.rect.height * 0.45f};
        DrawRectangleRounded(stripe, 0.8f, 4, trackKey ? Color{236, 218, 196, 185}
                                                       : (sliceAssigned ? Color{229, 190, 166, 210} : Color{174, 112, 91, 160}));

        const int fs = 9;
        if (trackKey) {
            const std::size_t ti = key.index - kMaxSlices;
            const char* label = trackLetter(ti);
            DrawText(label, static_cast<int>(key.rect.x + 5.0f), static_cast<int>(key.rect.y + key.rect.height - 14.0f),
                     11, selected ? kMarker : kCream);
        } else {
            const int auditionOrdinal = gUi.editStage == EditStage::Trim ? masterAuditionOrdinal(key.index) : -1;
            if (auditionOrdinal >= 0) {
                const char* label = kMasterAuditionLabels[static_cast<std::size_t>(auditionOrdinal)];
                const int auditionFs = std::string(label).size() > 4 ? 7 : 8;
                DrawText(label, static_cast<int>(key.rect.x + (key.rect.width - MeasureText(label, auditionFs)) * 0.5f),
                         static_cast<int>(key.rect.y + key.rect.height - 13.0f), auditionFs,
                         Color{255, 230, 207, 235});
            } else {
                const char* label = TextFormat("%02i", static_cast<int>(key.index + 1));
                DrawText(label, static_cast<int>(key.rect.x + 4.0f), static_cast<int>(key.rect.y + key.rect.height - 13.0f), fs,
                         Color{242, 221, 206, 220});
            }
        }
    }

    DrawText("SAMPLE A / B / C", static_cast<int>(keys.back().rect.x - 124.0f),
             static_cast<int>(keys.back().rect.y + keys.back().rect.height + 18.0f), 10, kCream);
}

void drawPlaceholderScreen(Rectangle screen, const char* title, const char* line1, const char* line2 = "") {
    DrawRectangleRec(screen, kScreenBg);
    DrawRectangleLinesEx(screen, 2.0f, Color{63, 91, 77, 255});
    DrawText(title, static_cast<int>(screen.x + 12), static_cast<int>(screen.y + 14), 18, kCream);
    DrawLine(static_cast<int>(screen.x + 10), static_cast<int>(screen.y + 40),
             static_cast<int>(screen.x + screen.width - 10), static_cast<int>(screen.y + 40), kScreenDim);
    DrawText(line1, static_cast<int>(screen.x + 12), static_cast<int>(screen.y + 76), 13, kScreenGreen);
    if (line2[0] != '\0') DrawText(line2, static_cast<int>(screen.x + 12), static_cast<int>(screen.y + 100), 13, kScreenDim);
}

void drawSamplerScreen(Rectangle screen, const TrackState& t,
                       const std::array<fm1::Slice, kMaxSlices>& slices) {
    DrawRectangleRec(screen, kScreenBg);
    DrawRectangleLinesEx(screen, 2.0f, Color{58, 91, 75, 255});
    const int topY = static_cast<int>(screen.y + 7);
    DrawText(TextFormat("SAMPLE %s  %s", trackLetter(gUi.activeTrack), gUi.editStage == EditStage::Trim ? "TRIM" : "CHOP"),
             static_cast<int>(screen.x + 8), topY, 12, kCream);
    DrawText(TextFormat("%s", chopModeLabel(t.chopMode)), static_cast<int>(screen.x + screen.width - 42), topY, 12, kMarker);

    const Rectangle waveform{screen.x + 7.0f, screen.y + 28.0f, screen.width - 14.0f, screen.height - 86.0f};
    drawWaveform(waveform, t, slices, gUi.punchArmed);

    if (!t.sample.empty()) {
        if (gUi.editStage == EditStage::Trim) {
            const double start = static_cast<double>(t.masterStart) / t.sample.sampleRate;
            const double end = static_cast<double>(t.masterEnd) / t.sample.sampleRate;
            DrawText(TextFormat("MASTER %.2f-%.2fs", start, end), static_cast<int>(screen.x + 8),
                     static_cast<int>(screen.y + screen.height - 50), 11, kCream);
            DrawText("K1 START K2 END | PRE GATE 1SHOT LOOP TAIL", static_cast<int>(screen.x + 8),
                     static_cast<int>(screen.y + screen.height - 34), 9, kScreenGreen);
        } else if (t.selectedSlice < t.activeSlices) {
            const auto& s = slices[t.selectedSlice];
            const double start = static_cast<double>(s.startFrame) / t.sample.sampleRate;
            const double end = static_cast<double>(s.endFrame) / t.sample.sampleRate;
            DrawText(TextFormat("S%02i %.2f-%.2fs", static_cast<int>(t.selectedSlice + 1), start, end),
                     static_cast<int>(screen.x + 8), static_cast<int>(screen.y + screen.height - 50), 11, kCream);
            DrawText(TextFormat("T%+.0f LV%.0f%% %s", s.semitones, s.gain * 100.0f, t.linkLength ? "LINK" : "FREE"),
                     static_cast<int>(screen.x + 8), static_cast<int>(screen.y + screen.height - 34), 11,
                     t.linkLength ? kMarker : kScreenGreen);
        }
    }

    const char* voice = t.mono ? "MONO" : "POLY";
    DrawText(voice, static_cast<int>(screen.x + screen.width - MeasureText(voice, 11) - 8),
             static_cast<int>(screen.y + screen.height - 50), 11, t.mono ? kMarker : kScreenGreen);
    const char* transport = gUi.punchArmed ? "REC PUNCH" : (t.previewPlaying.load() ? auditionModeLabel(t.auditionMode) : "STOP");
    DrawText(transport, static_cast<int>(screen.x + screen.width - MeasureText(transport, 11) - 8),
             static_cast<int>(screen.y + screen.height - 34), 11, gUi.punchArmed ? Color{233, 84, 72, 255} : kScreenGreen);
}

void setPage(ScreenPage page, const char* status) {
    gUi.page = page;
    gUi.status = status;
}

} // namespace

int main(int argc, char** argv) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(kInitialWidth, kInitialHeight, "M-VAVE FM-1 B-Boy Edition v0.2.5");
    SetTargetFPS(60);

    for (auto& t : gTracks) t.sampler.setMonophonic(true);

    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(512);
    AudioStream stream = LoadAudioStream(kOutputSampleRate, 32, 1);
    SetAudioStreamCallback(stream, audioCallback);
    PlayAudioStream(stream);

    if (argc > 1 && isSupportedDesktopAudio(argv[1])) gUi.status = loadSampleIntoActiveTrack(argv[1]);

    const double bootStart = GetTime();
    bool bootComplete = false;

    while (!WindowShouldClose()) {
        const double elapsed = GetTime() - bootStart;
        if (!bootComplete) {
            if (elapsed >= kBootDurationSeconds || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) bootComplete = true;
            BeginDrawing();
            drawBootScreen(elapsed, GetScreenWidth(), GetScreenHeight());
            EndDrawing();
            if (!bootComplete) continue;
        }

        if (IsFileDropped()) {
            FilePathList dropped = LoadDroppedFiles();
            if (dropped.count > 0) {
                const std::string path = dropped.paths[0];
                gUi.status = isSupportedDesktopAudio(path) ? loadSampleIntoActiveTrack(path)
                                                           : "Supported desktop imports: WAV, MP3, FLAC, OGG.";
            }
            UnloadDroppedFiles(dropped);
        }

        const int screenWidth = GetScreenWidth();
        const int screenHeight = GetScreenHeight();
        const float scale = std::min(static_cast<float>(screenWidth) / 1320.0f,
                                     static_cast<float>(screenHeight) / 840.0f);
        const float deviceW = 1240.0f * scale;
        const float deviceH = 740.0f * scale;
        const float deviceX = (screenWidth - deviceW) * 0.5f;
        const float deviceY = (screenHeight - deviceH) * 0.5f;
        const Rectangle device{deviceX, deviceY, deviceW, deviceH};
        const float mouseWheel = GetMouseWheelMove();

        const Vector2 masterC{device.x + 88.0f * scale, device.y + 90.0f * scale};
        const Vector2 selectC{device.x + 205.0f * scale, device.y + 90.0f * scale};
        const Vector2 presetsC{device.x + 88.0f * scale, device.y + 200.0f * scale};
        const Vector2 algorithmC{device.x + 205.0f * scale, device.y + 200.0f * scale};
        const float leftKnobR = 29.0f * scale;
        const Rectangle screen{device.x + 365.0f * scale, device.y + 48.0f * scale, 300.0f * scale, 300.0f * scale};
        const std::array<Vector2, 4> kC = {
            Vector2{device.x + 765.0f * scale, device.y + 82.0f * scale},
            Vector2{device.x + 885.0f * scale, device.y + 82.0f * scale},
            Vector2{device.x + 1005.0f * scale, device.y + 82.0f * scale},
            Vector2{device.x + 1125.0f * scale, device.y + 82.0f * scale}
        };
        const float kR = 27.0f * scale;

        auto buttonRect = [&](int column, float y0) {
            const float width = 61.0f * scale;
            const float height = 30.0f * scale;
            const float gap = 7.0f * scale;
            const float total = 6 * width + 5 * gap;
            const float x0 = device.x + 748.0f * scale + (410.0f * scale - total) * 0.5f;
            return Rectangle{x0 + column * (width + gap), device.y + y0 * scale, width, height};
        };
        const std::array<Rectangle, 6> topButtons = {buttonRect(0,145),buttonRect(1,145),buttonRect(2,145),buttonRect(3,145),buttonRect(4,145),buttonRect(5,145)};
        const std::array<Rectangle, 6> bottomButtons = {buttonRect(0,205),buttonRect(1,205),buttonRect(2,205),buttonRect(3,205),buttonRect(4,205),buttonRect(5,205)};
        const Rectangle octMinus{device.x + 90.0f * scale, device.y + 292.0f * scale, 62.0f * scale, 30.0f * scale};
        const Rectangle octPlus{device.x + 166.0f * scale, device.y + 292.0f * scale, 62.0f * scale, 30.0f * scale};
        const Rectangle pianoBounds{device.x + 48.0f * scale, device.y + 435.0f * scale, device.width - 96.0f * scale, 265.0f * scale};
        const auto pianoKeys = buildPianoKeys(pianoBounds);

        // The final three physical keys are SAMPLE A/B/C. Desktop F1-F3 mirror them.
        if (IsKeyPressed(KEY_F1)) selectTrack(0);
        if (IsKeyPressed(KEY_F2)) selectTrack(1);
        if (IsKeyPressed(KEY_F3)) selectTrack(2);

        std::size_t mousePianoIndex = 0;
        if (handlePianoMouse(pianoKeys, mousePianoIndex)) {
            if (mousePianoIndex >= kMaxSlices) {
                selectTrack(mousePianoIndex - kMaxSlices);
            } else {
                auto& keyTrack = activeTrack();
                if (gUi.editStage == EditStage::Trim) {
                    const int ordinal = masterAuditionOrdinal(mousePianoIndex);
                    if (ordinal >= 0) startMasterAudition(auditionModeForOrdinal(ordinal), 0, true);
                    else gUi.status = "MASTER TRIM: use PRE / GATE / 1SHOT / LOOP / TAIL white keys.";
                } else if (gUi.punchArmed && keyTrack.previewPlaying.load()) {
                    insertManualMarker(previewFrameSnapshot(keyTrack));
                } else {
                    triggerSlice(mousePianoIndex);
                }
            }
        }

        auto& t = activeTrack();

        // 24 chromatic shortcuts mirror the FM-1's F-based keybed. In MASTER
        // TRIM only the first five white keys are audition functions; no key
        // press can accidentally kick the user back into CHOP edit.
        for (std::size_t i = 0; i < kTriggerKeys.size(); ++i) {
            if (!IsKeyPressed(kTriggerKeys[i])) continue;
            if (gUi.editStage == EditStage::Trim) {
                const int ordinal = masterAuditionOrdinal(i);
                if (ordinal >= 0) startMasterAudition(auditionModeForOrdinal(ordinal), kTriggerKeys[i], false);
            } else if (gUi.punchArmed && t.previewPlaying.load()) {
                insertManualMarker(previewFrameSnapshot(t));
            } else {
                triggerSlice(i);
            }
        }
        releaseMasterAuditionGateIfNeeded();
        if (IsKeyPressed(KEY_SPACE) && !t.sample.empty()) {
            if (gUi.editStage == EditStage::Trim) startMasterAudition(AuditionMode::OneShot);
            else triggerSlice(t.selectedSlice);
        }
        if (IsKeyPressed(KEY_ENTER)) togglePreview();

        if (IsKeyPressed(KEY_BACKSPACE) && t.chopMode == ChopMode::Manual && t.manualMarkers.size() > 2) {
            if (t.selectedSlice + 1 < t.manualMarkers.size() - 1) {
                t.manualMarkers.erase(t.manualMarkers.begin() + static_cast<long long>(t.selectedSlice + 1));
                syncManualSlices(t);
                t.selectedSlice = std::min(t.selectedSlice, t.activeSlices - 1);
                gUi.status = "Manual marker deleted.";
            }
        }

        const int masterDelta = knobDelta(masterC, leftKnobR, mouseWheel);
        if (masterDelta != 0) {
            const float v = std::clamp(gMasterVolume.load() + masterDelta * 0.04f, 0.0f, 1.0f);
            gMasterVolume.store(v);
            gUi.status = TextFormat("Master %.0f%%", v * 100.0f);
        }

        const int selectDelta = knobDelta(selectC, leftKnobR, mouseWheel);
        if (selectDelta != 0) {
            gUi.editStage = gUi.editStage == EditStage::Trim ? EditStage::Chop : EditStage::Trim;
            centerViewOn(t, selectedFocusFrame(t));
            gUi.status = gUi.editStage == EditStage::Trim ? "MASTER TRIM page." : "CHOP EDIT page.";
        }

        const int presetsDelta = knobDelta(presetsC, leftKnobR, mouseWheel);
        if (presetsDelta != 0) cycleChopMode(presetsDelta);

        const int algorithmDelta = knobDelta(algorithmC, leftKnobR, mouseWheel);
        if (algorithmDelta != 0 && !t.sample.empty()) {
            zoomView(t, algorithmDelta > 0 ? 0.75 : 1.333333, selectedFocusFrame(t));
            gUi.status = algorithmDelta > 0 ? "Zoom in (auto-centred)." : "Zoom out (auto-centred).";
        }

        const std::size_t editStep = visibleBoundaryStep(t);
        const int k1Delta = knobDelta(kC[0], kR, mouseWheel);
        const int k2Delta = knobDelta(kC[1], kR, mouseWheel);
        const int k3Delta = knobDelta(kC[2], kR, mouseWheel);
        const int k4Delta = knobDelta(kC[3], kR, mouseWheel);
        if (gUi.editStage == EditStage::Trim) {
            if (k1Delta != 0) nudgeMasterBoundary(static_cast<long long>(editStep) * k1Delta, false);
            if (k2Delta != 0) nudgeMasterBoundary(static_cast<long long>(editStep) * k2Delta, true);
        } else {
            if (k1Delta != 0) nudgeSliceBoundary(static_cast<long long>(editStep) * k1Delta, false);
            if (k2Delta != 0) nudgeSliceBoundary(static_cast<long long>(editStep) * k2Delta, true);
            if (k3Delta != 0) tuneSelected(static_cast<float>(k3Delta));
            if (k4Delta != 0) gainSelected(static_cast<float>(k4Delta) * 0.05f);
        }

        if (buttonPressed(octMinus)) {
            t.linkLength = !t.linkLength;
            gUi.status = t.linkLength ? "LINK LENGTH ON." : "LINK LENGTH OFF.";
        }
        if (buttonPressed(octPlus)) {
            t.mono = !t.mono;
            t.sampler.setMonophonic(t.mono);
            gUi.status = std::string("SAMPLE ") + trackLetter(gUi.activeTrack) + (t.mono ? " MONO." : " POLY.");
        }

        // Mouse waveform manipulation remains as a desktop convenience; hardware zoom is auto-centred.
        if (!t.sample.empty() && (gUi.page == ScreenPage::Edit || gUi.page == ScreenPage::Home)) {
            const Rectangle waveform{screen.x + 7.0f, screen.y + 28.0f, screen.width - 14.0f, screen.height - 86.0f};
            const Vector2 mouse = GetMousePosition();
            if (CheckCollisionPointRec(mouse, waveform)) {
                if (mouseWheel != 0.0f) {
                    const double rel = std::clamp((mouse.x - waveform.x) / waveform.width, 0.0f, 1.0f);
                    const std::size_t span = t.viewEnd > t.viewStart ? t.viewEnd - t.viewStart : t.sample.frames();
                    const std::size_t anchor = t.viewStart + static_cast<std::size_t>(rel * span);
                    zoomView(t, mouseWheel > 0.0f ? 0.78 : 1.28, anchor);
                }
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    const double rel = std::clamp((mouse.x - waveform.x) / waveform.width, 0.0f, 1.0f);
                    const std::size_t span = t.viewEnd > t.viewStart ? t.viewEnd - t.viewStart : t.sample.frames();
                    const std::size_t frame = std::min(t.sample.frames(), t.viewStart + static_cast<std::size_t>(rel * span));
                    const bool moveEnd = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
                    if (gUi.editStage == EditStage::Trim) {
                        const long long old = static_cast<long long>(moveEnd ? t.masterEnd : t.masterStart);
                        nudgeMasterBoundary(static_cast<long long>(frame) - old, moveEnd);
                    } else {
                        setSliceBoundaryAbsolute(frame, moveEnd);
                    }
                }
                if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                    gUi.panning = true;
                    gUi.panAnchorMouse = mouse;
                    gUi.panAnchorStart = t.viewStart;
                    gUi.panAnchorEnd = t.viewEnd;
                }
            }
        }
        if (gUi.panning && IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !t.sample.empty()) {
            const float dx = GetMousePosition().x - gUi.panAnchorMouse.x;
            const std::size_t span = gUi.panAnchorEnd - gUi.panAnchorStart;
            const long long delta = static_cast<long long>(-dx / std::max(1.0f, screen.width - 14.0f) * span);
            t.viewStart = gUi.panAnchorStart;
            t.viewEnd = gUi.panAnchorEnd;
            panView(t, delta);
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) gUi.panning = false;

        if (buttonPressed(topButtons[0])) setPage(ScreenPage::Fx, "FX page reserved for sampler/filter effects.");
        if (buttonPressed(topButtons[1])) {
            t.linkLength = !t.linkLength;
            gUi.status = t.linkLength ? "LINK LENGTH ON." : "LINK LENGTH OFF.";
        }
        if (buttonPressed(topButtons[2])) setPage(ScreenPage::Env, "ENV page reserved for amp/filter envelopes.");
        if (buttonPressed(topButtons[3])) setPage(ScreenPage::Lfo, "LFO page reserved for modulation.");
        if (buttonPressed(topButtons[4])) setPage(ScreenPage::Edit, "Sample trim / chop edit.");
        if (buttonPressed(topButtons[5])) setPage(ScreenPage::Global, "Global sampler settings placeholder.");

        if (buttonPressed(bottomButtons[0])) setPage(ScreenPage::Home, "Sampler home.");
        if (buttonPressed(bottomButtons[1])) gUi.status = "SAVE reserved for bank/project save.";
        if (buttonPressed(bottomButtons[2])) setPage(ScreenPage::Arp, "ARP reserved for note-repeat / performance tools.");
        if (buttonPressed(bottomButtons[3])) setPage(ScreenPage::Seq, "SEQ reserved for B-Boy sequencer.");
        if (buttonPressed(bottomButtons[4])) {
            togglePreview();
        }
        if (buttonPressed(bottomButtons[5])) {
            auto& at = activeTrack();
            if (at.chopMode != ChopMode::Manual) setChopMode(at, ChopMode::Manual);
            gUi.editStage = EditStage::Chop;
            gUi.punchArmed = !gUi.punchArmed;
            gUi.status = gUi.punchArmed ? "Punch armed. PLAY then tap chop keys." : "Punch disarmed.";
        }

        const auto slices = snapshotSlices(activeTrack());

        BeginDrawing();
        ClearBackground(Color{24, 17, 15, 255});
        DrawRectangleRounded(device, 0.035f, 10, kBodyOrange);
        DrawRectangleRoundedLinesEx(device, 0.035f, 10, 2.0f, Color{116, 39, 24, 255});

        DrawText("M-VAVE", static_cast<int>(device.x + 45.0f * scale), static_cast<int>(device.y + 24.0f * scale),
                 static_cast<int>(18.0f * scale), kCream);
        DrawText("FM-1", static_cast<int>(device.x + 287.0f * scale), static_cast<int>(device.y + 24.0f * scale),
                 static_cast<int>(15.0f * scale), Color{255, 214, 191, 255});
        DrawText("B-BOY EDITION", static_cast<int>(device.x + 760.0f * scale), static_cast<int>(device.y + 25.0f * scale),
                 static_cast<int>(15.0f * scale), kPanelInk);

        drawKnob(masterC, leftKnobR, "MASTER", gMasterVolume.load());
        drawKnob(selectC, leftKnobR, "SELECT", gUi.editStage == EditStage::Chop ? 1.0f : 0.0f);
        drawKnob(presetsC, leftKnobR, "PRESETS", static_cast<float>(static_cast<int>(activeTrack().chopMode)) / 3.0f);
        const float zoomNorm = activeTrack().sample.empty() || activeTrack().viewEnd <= activeTrack().viewStart
            ? 0.0f : 1.0f - static_cast<float>(activeTrack().viewEnd - activeTrack().viewStart) / static_cast<float>(activeTrack().sample.frames());
        drawKnob(algorithmC, leftKnobR, "ALGORITHM", zoomNorm);

        drawButton(octMinus, "OCT-", activeTrack().linkLength);
        drawButton(octPlus, "OCT+", activeTrack().mono);
        DrawText("LINK", static_cast<int>(octMinus.x + 17.0f * scale), static_cast<int>(octMinus.y + 36.0f * scale),
                 std::max(8, static_cast<int>(9.0f * scale)), kPanelInk);
        DrawText(activeTrack().mono ? "MONO" : "POLY", static_cast<int>(octPlus.x + 13.0f * scale), static_cast<int>(octPlus.y + 36.0f * scale),
                 std::max(8, static_cast<int>(9.0f * scale)), kPanelInk);

        if (gUi.page == ScreenPage::Home || gUi.page == ScreenPage::Edit) {
            drawSamplerScreen(screen, activeTrack(), slices);
        } else if (gUi.page == ScreenPage::Fx) {
            drawPlaceholderScreen(screen, "FX", "Filter / drive / delay", "planned for later build");
        } else if (gUi.page == ScreenPage::Env) {
            drawPlaceholderScreen(screen, "ENV", "Amp + filter envelope", "planned for later build");
        } else if (gUi.page == ScreenPage::Lfo) {
            drawPlaceholderScreen(screen, "LFO", "Modulation", "planned for later build");
        } else if (gUi.page == ScreenPage::Arp) {
            drawPlaceholderScreen(screen, "ARP", "Note repeat / stutter", "planned performance page");
        } else if (gUi.page == ScreenPage::Seq) {
            drawPlaceholderScreen(screen, "SEQ", "A / B / C + FM sequencer", "planned next major phase");
        } else {
            drawPlaceholderScreen(screen, "GLO", "Sampler globals", "storage / MIDI / quality later");
        }

        float n1 = 0.0f, n2 = 0.0f, n3 = 0.5f, n4 = 0.5f;
        const auto& at = activeTrack();
        if (!at.sample.empty()) {
            if (gUi.editStage == EditStage::Trim) {
                n1 = static_cast<float>(at.masterStart) / static_cast<float>(at.sample.frames());
                n2 = static_cast<float>(at.masterEnd) / static_cast<float>(at.sample.frames());
            } else if (at.selectedSlice < at.activeSlices) {
                const auto& s = slices[at.selectedSlice];
                n1 = static_cast<float>(s.startFrame) / static_cast<float>(at.sample.frames());
                n2 = static_cast<float>(s.endFrame) / static_cast<float>(at.sample.frames());
                n3 = (s.semitones + 36.0f) / 72.0f;
                n4 = s.gain / 2.0f;
            }
        }
        drawKnob(kC[0], kR, gUi.editStage == EditStage::Trim ? "K1 M.START" : "K1 START", n1, true);
        drawKnob(kC[1], kR, gUi.editStage == EditStage::Trim ? "K2 M.END" : "K2 END", n2, true);
        drawKnob(kC[2], kR, gUi.editStage == EditStage::Trim ? "K3 --" : "K3 TUNE", n3, gUi.editStage == EditStage::Chop);
        drawKnob(kC[3], kR, gUi.editStage == EditStage::Trim ? "K4 --" : "K4 LEVEL", n4, gUi.editStage == EditStage::Chop);

        drawButton(topButtons[0], "FX", gUi.page == ScreenPage::Fx);
        drawButton(topButtons[1], "SEL", at.linkLength);
        drawButton(topButtons[2], "ENV", gUi.page == ScreenPage::Env);
        drawButton(topButtons[3], "LFO", gUi.page == ScreenPage::Lfo);
        drawButton(topButtons[4], "EDIT", gUi.page == ScreenPage::Edit);
        drawButton(topButtons[5], "GLO", gUi.page == ScreenPage::Global);
        drawButton(bottomButtons[0], "HOME", gUi.page == ScreenPage::Home);
        drawButton(bottomButtons[1], "SAVE", false);
        drawButton(bottomButtons[2], "ARP", gUi.page == ScreenPage::Arp);
        drawButton(bottomButtons[3], "SEQ", gUi.page == ScreenPage::Seq);
        drawButton(bottomButtons[4], "PLAY/STOP", at.previewPlaying.load());
        drawButton(bottomButtons[5], "REC", gUi.punchArmed, true);

        drawPiano(pianoKeys, at);

        const int statusSize = std::max(10, static_cast<int>(12.0f * scale));
        DrawText(gUi.status.c_str(), static_cast<int>(device.x + 48.0f * scale),
                 static_cast<int>(device.y + 708.0f * scale), statusSize, kCream);
        DrawText("SELECT=TRIM/CHOP  PRESETS=8/16/24/MAN  ALGORITHM=ZOOM  OCT-=LINK  OCT+=MONO/POLY",
                 static_cast<int>(device.x + 565.0f * scale), static_cast<int>(device.y + 708.0f * scale),
                 std::max(8, static_cast<int>(9.0f * scale)), kPanelInk);

        EndDrawing();
    }

    StopAudioStream(stream);
    UnloadAudioStream(stream);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
