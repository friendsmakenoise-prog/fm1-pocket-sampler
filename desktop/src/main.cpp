#include "fm1/Sampler.h"
#include "fm1/WavFile.h"

#include "raylib.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>

namespace {

constexpr int kInitialWidth = 1100;
constexpr int kInitialHeight = 760;
constexpr std::size_t kSliceCount = fm1::Sampler::kMaxSlices;
constexpr uint32_t kOutputSampleRate = 44100;

std::mutex gAudioMutex;
fm1::SampleBuffer gSample;
fm1::Sampler gSampler;

const std::array<int, kSliceCount> kTriggerKeys = {
    KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE, KEY_SIX,
    KEY_SEVEN, KEY_EIGHT, KEY_NINE, KEY_ZERO, KEY_MINUS, KEY_EQUAL,
    KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y,
    KEY_U, KEY_I, KEY_O, KEY_P, KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET
};

const std::array<const char*, kSliceCount> kTriggerLabels = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=",
    "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]"
};

void audioCallback(void* bufferData, unsigned int frames) {
    auto* output = static_cast<float*>(bufferData);
    std::lock_guard<std::mutex> lock(gAudioMutex);
    gSampler.render(output, frames, kOutputSampleRate);
}

std::string fileNameOnly(const std::string& path) {
    try {
        return std::filesystem::path(path).filename().string();
    } catch (...) {
        return path;
    }
}

std::string loadSample(const std::string& path) {
    try {
        auto loaded = fm1::WavFile::load16BitPcm(path);
        const auto frames = loaded.frames();
        const auto rate = loaded.sampleRate;

        {
            std::lock_guard<std::mutex> lock(gAudioMutex);
            gSample = std::move(loaded);
            gSampler.setSample(&gSample);
            gSampler.makeEqualSlices(kSliceCount);
        }

        std::ostringstream message;
        message << "Loaded " << fileNameOnly(path)
                << "  |  " << rate << " Hz"
                << "  |  " << std::fixed << std::setprecision(2)
                << (rate == 0 ? 0.0 : static_cast<double>(frames) / rate) << " sec";
        return message.str();
    } catch (const std::exception& e) {
        return std::string("Could not load WAV: ") + e.what();
    }
}

void triggerSlice(std::size_t index, std::size_t& selectedSlice) {
    selectedSlice = index;
    std::lock_guard<std::mutex> lock(gAudioMutex);
    gSampler.noteOn(index, 1.0f);
}

std::array<fm1::Slice, kSliceCount> snapshotSlices() {
    std::array<fm1::Slice, kSliceCount> result{};
    std::lock_guard<std::mutex> lock(gAudioMutex);
    for (std::size_t i = 0; i < result.size(); ++i) result[i] = gSampler.slice(i);
    return result;
}

void setSliceBoundary(std::size_t sliceIndex, std::size_t frame, bool setEnd) {
    std::lock_guard<std::mutex> lock(gAudioMutex);
    if (gSample.empty()) return;

    auto slice = gSampler.slice(sliceIndex);
    const auto lastFrame = gSample.frames();

    if (setEnd) {
        const auto minimumEnd = std::min(lastFrame, slice.startFrame + 1);
        slice.endFrame = std::clamp(frame, minimumEnd, lastFrame);
    } else {
        const auto maximumStart = slice.endFrame > 0 ? slice.endFrame - 1 : 0;
        slice.startFrame = std::min(frame, maximumStart);
    }

    gSampler.setSlice(sliceIndex, slice);
}

void tuneSelected(std::size_t sliceIndex, float semitoneDelta) {
    std::lock_guard<std::mutex> lock(gAudioMutex);
    auto slice = gSampler.slice(sliceIndex);
    slice.semitones = std::clamp(slice.semitones + semitoneDelta, -36.0f, 36.0f);
    gSampler.setSlice(sliceIndex, slice);
}

void drawWaveform(const Rectangle& bounds,
                  const fm1::SampleBuffer& sample,
                  const std::array<fm1::Slice, kSliceCount>& slices,
                  std::size_t selectedSlice) {
    DrawRectangleRec(bounds, Color{24, 26, 30, 255});
    DrawRectangleLinesEx(bounds, 1.0f, Color{78, 82, 91, 255});

    if (sample.empty()) {
        const char* prompt = "DROP A 16-BIT PCM WAV INTO THIS WINDOW";
        const int size = 24;
        const int width = MeasureText(prompt, size);
        DrawText(prompt,
                 static_cast<int>(bounds.x + (bounds.width - width) * 0.5f),
                 static_cast<int>(bounds.y + bounds.height * 0.5f - size),
                 size,
                 Color{170, 174, 184, 255});
        return;
    }

    const auto selected = slices[selectedSlice];
    if (selected.valid()) {
        const float x0 = bounds.x + bounds.width * static_cast<float>(selected.startFrame) / static_cast<float>(sample.frames());
        const float x1 = bounds.x + bounds.width * static_cast<float>(selected.endFrame) / static_cast<float>(sample.frames());
        DrawRectangle(static_cast<int>(x0), static_cast<int>(bounds.y),
                      std::max(1, static_cast<int>(x1 - x0)), static_cast<int>(bounds.height),
                      Color{74, 65, 35, 120});
    }

    const int columns = std::max(1, static_cast<int>(bounds.width));
    const float centreY = bounds.y + bounds.height * 0.5f;
    const float amplitudeScale = bounds.height * 0.45f;

    for (int x = 0; x < columns; ++x) {
        const std::size_t start = static_cast<std::size_t>(
            (static_cast<double>(x) / columns) * sample.frames());
        const std::size_t end = std::min(sample.frames(), static_cast<std::size_t>(
            (static_cast<double>(x + 1) / columns) * sample.frames()) + 1);

        float peak = 0.0f;
        for (std::size_t i = start; i < end; ++i) peak = std::max(peak, std::abs(sample.mono[i]));

        DrawLine(static_cast<int>(bounds.x) + x,
                 static_cast<int>(centreY - peak * amplitudeScale),
                 static_cast<int>(bounds.x) + x,
                 static_cast<int>(centreY + peak * amplitudeScale),
                 Color{205, 208, 214, 255});
    }

    for (std::size_t i = 0; i < kSliceCount; ++i) {
        if (!slices[i].valid()) continue;
        const float sx = bounds.x + bounds.width * static_cast<float>(slices[i].startFrame) / static_cast<float>(sample.frames());
        const Color marker = (i == selectedSlice) ? Color{255, 201, 73, 255} : Color{91, 149, 197, 220};
        DrawLine(static_cast<int>(sx), static_cast<int>(bounds.y),
                 static_cast<int>(sx), static_cast<int>(bounds.y + bounds.height), marker);
    }

    const float endX = bounds.x + bounds.width * static_cast<float>(selected.endFrame) / static_cast<float>(sample.frames());
    DrawLine(static_cast<int>(endX), static_cast<int>(bounds.y),
             static_cast<int>(endX), static_cast<int>(bounds.y + bounds.height), Color{255, 201, 73, 255});
}

} // namespace

int main(int argc, char** argv) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(kInitialWidth, kInitialHeight, "FM-1 Pocket Sampler v0.2");
    SetTargetFPS(60);

    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(512);
    AudioStream stream = LoadAudioStream(kOutputSampleRate, 32, 1);
    SetAudioStreamCallback(stream, audioCallback);
    PlayAudioStream(stream);

    std::size_t selectedSlice = 0;
    std::string status = "Drop a WAV into the window to begin.";

    if (argc > 1) status = loadSample(argv[1]);

    while (!WindowShouldClose()) {
        if (IsFileDropped()) {
            FilePathList dropped = LoadDroppedFiles();
            if (dropped.count > 0) {
                const std::string path = dropped.paths[0];
                if (IsFileExtension(path.c_str(), ".wav;.WAV")) {
                    status = loadSample(path);
                    selectedSlice = 0;
                } else {
                    status = "Please drop a .wav file (16-bit PCM for v0.2).";
                }
            }
            UnloadDroppedFiles(dropped);
        }

        for (std::size_t i = 0; i < kTriggerKeys.size(); ++i) {
            if (IsKeyPressed(kTriggerKeys[i])) triggerSlice(i, selectedSlice);
        }

        if (IsKeyPressed(KEY_SPACE)) triggerSlice(selectedSlice, selectedSlice);

        if (IsKeyPressed(KEY_UP)) tuneSelected(selectedSlice, 1.0f);
        if (IsKeyPressed(KEY_DOWN)) tuneSelected(selectedSlice, -1.0f);

        if (IsKeyPressed(KEY_R)) {
            std::lock_guard<std::mutex> lock(gAudioMutex);
            if (!gSample.empty()) {
                gSampler.makeEqualSlices(kSliceCount);
                status = "Reset to 24 equal slices.";
            }
        }

        const int screenWidth = GetScreenWidth();
        const int screenHeight = GetScreenHeight();
        const Rectangle waveform{
            34.0f,
            112.0f,
            static_cast<float>(std::max(200, screenWidth - 68)),
            static_cast<float>(std::max(180, screenHeight - 380))
        };

        if (!gSample.empty() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(GetMousePosition(), waveform)) {
            const float position = std::clamp((GetMouseX() - waveform.x) / waveform.width, 0.0f, 1.0f);
            const std::size_t frame = std::min(gSample.frames(), static_cast<std::size_t>(position * gSample.frames()));
            const bool setEnd = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
            setSliceBoundary(selectedSlice, frame, setEnd);
            status = setEnd ? "Selected slice END moved." : "Selected slice START moved.";
        }

        auto slices = snapshotSlices();

        BeginDrawing();
        ClearBackground(Color{13, 14, 17, 255});

        DrawText("FM-1 POCKET SAMPLER", 34, 24, 30, Color{239, 240, 242, 255});
        DrawText("v0.2 desktop sandbox", 36, 62, 18, Color{132, 137, 148, 255});
        DrawText(status.c_str(), 34, 88, 16, Color{185, 189, 198, 255});

        drawWaveform(waveform, gSample, slices, selectedSlice);

        const int infoY = static_cast<int>(waveform.y + waveform.height + 18);
        if (!gSample.empty()) {
            const auto& selected = slices[selectedSlice];
            const double startSeconds = static_cast<double>(selected.startFrame) / gSample.sampleRate;
            const double endSeconds = static_cast<double>(selected.endFrame) / gSample.sampleRate;
            DrawText(TextFormat("SLICE %02i/24   START %.3fs   END %.3fs   TUNE %+.0f st",
                                static_cast<int>(selectedSlice + 1), startSeconds, endSeconds, selected.semitones),
                     34, infoY, 18, Color{238, 205, 105, 255});
        }

        DrawText("Click waveform = start  |  Shift-click = end  |  Up/Down = tune  |  R = equal chops  |  Space = retrigger",
                 34, infoY + 28, 15, Color{139, 144, 154, 255});

        const int padTop = infoY + 64;
        const float padGap = 5.0f;
        const float usableWidth = static_cast<float>(screenWidth - 68);
        const float padWidth = (usableWidth - padGap * 11.0f) / 12.0f;
        const float padHeight = 58.0f;

        for (std::size_t i = 0; i < kSliceCount; ++i) {
            const int row = static_cast<int>(i / 12);
            const int column = static_cast<int>(i % 12);
            Rectangle pad{
                34.0f + column * (padWidth + padGap),
                static_cast<float>(padTop) + row * (padHeight + padGap),
                padWidth,
                padHeight
            };

            const bool selected = i == selectedSlice;
            const bool keyHeld = IsKeyDown(kTriggerKeys[i]);
            const Color fill = keyHeld ? Color{111, 92, 38, 255}
                                       : selected ? Color{58, 55, 43, 255}
                                                  : Color{30, 33, 39, 255};
            DrawRectangleRec(pad, fill);
            DrawRectangleLinesEx(pad, selected ? 2.0f : 1.0f,
                                 selected ? Color{255, 201, 73, 255} : Color{76, 81, 91, 255});

            DrawText(TextFormat("%02i", static_cast<int>(i + 1)),
                     static_cast<int>(pad.x + 8), static_cast<int>(pad.y + 7), 18,
                     Color{226, 228, 232, 255});
            DrawText(kTriggerLabels[i],
                     static_cast<int>(pad.x + pad.width - MeasureText(kTriggerLabels[i], 18) - 8),
                     static_cast<int>(pad.y + 31), 18,
                     Color{143, 149, 159, 255});

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), pad)) {
                triggerSlice(i, selectedSlice);
            }
        }

        EndDrawing();
    }

    StopAudioStream(stream);
    UnloadAudioStream(stream);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
