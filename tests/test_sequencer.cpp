#include "fm1/Sequencer.h"

#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    fm1::Sequencer seq;
    assert(std::abs(seq.bpm() - 90.0f) < 0.001f);
    assert(seq.track(0).length() == 16);

    seq.track(0).setLength(7);
    seq.track(1).setLength(12);
    seq.track(2).setLength(64);
    assert(seq.track(0).length() == 7);
    assert(seq.track(1).length() == 12);
    assert(seq.track(2).length() == 64);

    seq.track(0).setEvent(3, 11, 96);
    const auto& event = seq.track(0).event(3);
    assert(event.active);
    assert(event.sliceIndex == 11);
    assert(event.velocity == 96);

    seq.track(0).clearEvent(3);
    assert(!seq.track(0).event(3).active);

    seq.track(1).setMuted(true);
    assert(seq.track(1).muted());

    seq.setBpm(120.0f);
    const double expected = 44100.0 * 60.0 / (120.0 * 4.0);
    assert(std::abs(seq.framesPerStep(44100) - expected) < 0.001);

    // Hardware-minded clamps.
    seq.track(0).setLength(0);
    seq.track(1).setLength(999);
    assert(seq.track(0).length() == 1);
    assert(seq.track(1).length() == 64);
    seq.setBpm(5.0f);
    assert(std::abs(seq.bpm() - fm1::Sequencer::kMinBpm) < 0.001f);

    std::cout << "Sequencer tests passed\n";
    return 0;
}
