#pragma once

#include <algorithm>

namespace pet {

// A rare Fetch idle action chooses one self-contained gift outcome.
// No real audio, particles, timers or Qt widgets live in this data type.
enum class GiftKind : int { StarFly, ArcFly, BurstSad, BurstCry, Pulse };
constexpr int kGiftKindCount = 5;

enum class GiftCue : int {
    Inactive, Carry, Present, Transfer, FlyStraight, FlyArc, Charge, Burst,
    TearLeft, TearRight, Pulse
};

// Progress advances continuously (0..1) inside an authored Frame.
// The painter is purely visual; the Animator owns all timing and priority.
struct GiftVisual {
    GiftCue cue = GiftCue::Inactive;
    GiftKind kind = GiftKind::StarFly;
    double progress = 0.0;
    int carryDx = 0; // the carried gift follows each approaching body pose
    [[nodiscard]] bool active() const { return cue != GiftCue::Inactive; }
};

inline bool isBurst(GiftKind kind) {
    return kind == GiftKind::BurstSad || kind == GiftKind::BurstCry;
}

} // namespace pet
