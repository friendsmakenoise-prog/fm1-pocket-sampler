#pragma once

#include "fm1/SampleBuffer.h"
#include <string>

namespace fm1 {

class WavFile {
public:
    static SampleBuffer load16BitPcm(const std::string& path);
};

} // namespace fm1
