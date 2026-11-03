#pragma once

#include <algorithm>

// 0 disables two-pulse clock flicker. Higher values reduce the interval.
// 1 = ~2 minutes, 25 = ~72 seconds (legacy default), 100 = 4 seconds.
// The caller may apply slight jitter to make the CRT effect less mechanical.
namespace crt {
constexpr int kDefaultFlickerFrequency = 25;

inline int flickerIntervalMs(int frequency) {
    const int level = std::clamp(frequency, 0, 100);
    if (level == 0) return 0;
    const int remaining = 100 - level;
    return 4000 + 12 * remaining * remaining;
}
} // namespace crt
