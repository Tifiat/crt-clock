#pragma once

#include "pet_sequence.hpp"

#include <QRandomGenerator>
#include <QVector>
#include <QtGlobal>
#include <algorithm>
#include <utility>

namespace pet {
enum class Activity { Idle, Music, Typing };
enum class IdleAction { Blink, Curious, Wave, Stretch, Nap, Sway, Surprise };
enum class MusicAction { HeadNod, TailWag, ArmWave, SideStep, Bounce };
constexpr int kIdleActionCount = 7;
constexpr int kMusicActionCount = 5;

struct Frame {
    int pose;
    int durationMs;
};
using Story = QVector<Frame>;

inline int duration(const Story &story) {
    int total = 0;
    for (const Frame &frame : story) total += frame.durationMs;
    return total;
}

// Idle actions are complete multi-second stories, not single-frame twitching.
// A blink itself can be fast, but the complete action retains a coherent rhythm.
inline Story idleStory(IdleAction action) {
    switch (action) {
        case IdleAction::Blink:
            return {{0, 650},{1, 175},{0, 700},{2, 650},{0, 450}};
        case IdleAction::Curious:
            return {{0, 450},{2, 900},{0, 350},{3, 900},{0, 450}};
        case IdleAction::Wave:
            return {{0, 450},{4, 500},{5, 500},{4, 500},{5, 500},{0, 550}};
        case IdleAction::Stretch:
            return {{0, 550},{6, 1100},{16, 800},{0, 800}};
        case IdleAction::Nap:
            return {{1, 600},{7, 1500},{1, 550},{0, 550}};
        case IdleAction::Sway:
            return {{0, 450},{22, 520},{23, 520},{22, 520},{23, 520},{0, 550}};
        case IdleAction::Surprise:
            return {{0, 500},{17, 950},{16, 750},{0, 700}};
    }
    return {};
}

// The tail stays in a fixed drawing slot. The older transient backside
// silhouette is deliberately removed: it resembled a rendering glitch.
inline Story musicStory(MusicAction action) {
    switch (action) {
        case MusicAction::HeadNod:
            return {{0, 400},{18, 450},{19, 450},{18, 450},
                    {19, 450},{18, 450},{19, 450},{0, 450}};
        case MusicAction::TailWag:
            return {{8, 500},{9, 145},{10, 145},{9, 145},{10, 145},
                    {9, 145},{10, 145},{9, 145},{10, 145},
                    {9, 145},{10, 145},{9, 145},{10, 145},
                    {8, 550},{0, 500}};
        case MusicAction::ArmWave:
            return {{0, 450},{24, 450},{25, 450},{11, 450},
                    {24, 450},{25, 450},{12, 450},{0, 450}};
        case MusicAction::SideStep:
            return {{0, 450},{20, 520},{21, 520},{20, 520},
                    {21, 520},{20, 520},{0, 550}};
        case MusicAction::Bounce:
            return {{0, 500},{22, 500},{23, 500},{22, 500},
                    {23, 500},{16, 550},{0, 500}};
    }
    return {};
}

// User preferred this quick tactile animation; it keeps priority over music.
inline Story typingStory() {
    return {{13, 115}, {14, 115}, {15, 115}, {14, 115}};
}

class Animator {
public:
    // Typing overrides music, and music overrides idle.
    bool advance(qint64 nowMs, bool typing, bool playing) {
        const Activity desired = typing ? Activity::Typing :
                                 playing ? Activity::Music : Activity::Idle;
        if (desired != activity_) {
            activity_ = desired;
            story_.clear();
            frameIndex_ = 0;
            nextFrame_ = nowMs;
            idleDue_ = nowMs + 1200;
            if (desired == Activity::Typing) begin(typingStory(), nowMs);
            else if (desired == Activity::Music) nextMusic(nowMs);
            else pose_ = 0;
        }

        if (activity_ == Activity::Idle && story_.isEmpty() && nowMs >= idleDue_) {
            const auto action = static_cast<IdleAction>(idleCycle_.next());
            begin(idleStory(action), nowMs);
        }

        if (!story_.isEmpty() && nowMs >= nextFrame_) {
            ++frameIndex_;
            if (frameIndex_ >= story_.size()) {
                if (activity_ == Activity::Music) nextMusic(nowMs);
                else if (activity_ == Activity::Typing) begin(typingStory(), nowMs);
                else {
                    story_.clear();
                    pose_ = 0;
                    idleDue_ = nowMs + QRandomGenerator::global()->bounded(6500, 14500);
                }
            } else {
                pose_ = story_[frameIndex_].pose;
                nextFrame_ = nowMs + story_[frameIndex_].durationMs;
            }
        }

        const bool changed = pose_ != lastPose_;
        lastPose_ = pose_;
        return changed;
    }

    [[nodiscard]] int pose() const { return pose_; }
    [[nodiscard]] Activity activity() const { return activity_; }

private:
    void nextMusic(qint64 nowMs) {
        // Each music action appears once per shuffled cycle.
        begin(musicStory(static_cast<MusicAction>(musicCycle_.next())), nowMs);
    }

    void begin(Story sequence, qint64 nowMs) {
        story_ = std::move(sequence);
        frameIndex_ = 0;
        if (story_.isEmpty()) {
            pose_ = 0;
            return;
        }
        pose_ = story_[0].pose;
        nextFrame_ = nowMs + story_[0].durationMs;
    }

    Activity activity_ = Activity::Idle;
    Story story_;
    int frameIndex_ = 0;
    int pose_ = 0;
    int lastPose_ = -1;
    ShuffleBag musicCycle_{kMusicActionCount};
    ShuffleBag idleCycle_{kIdleActionCount};
    qint64 nextFrame_ = 0;
    qint64 idleDue_ = 8500;
};
} // namespace pet
