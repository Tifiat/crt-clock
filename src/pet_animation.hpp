#pragma once

#include <QRandomGenerator>
#include <QVector>
#include <QtGlobal>
#include <algorithm>

namespace pet {
enum class Activity { Idle, Music, Typing };
enum class IdleAction { Blink, Curious, Wave, Stretch, Nap, Sway, Surprise };

struct Frame {
    int pose;
    int durationMs;
};
using Story = QVector<Frame>;

// All animation content is data here. The state machine below knows nothing
// about characters, fonts, Qt widgets, keyboards or music players.
inline Story idleStory(IdleAction action) {
    switch (action) {
        case IdleAction::Blink:    return {{1, 180}, {0, 200}};
        case IdleAction::Curious:  return {{2, 440}, {0, 280}, {3, 470}, {0, 180}};
        case IdleAction::Wave:     return {{4, 320}, {5, 300}, {4, 320}, {0, 200}};
        case IdleAction::Stretch:  return {{6, 700}, {16, 370}, {0, 300}};
        case IdleAction::Nap:      return {{1, 600}, {7, 1200}, {1, 350}, {0, 250}};
        case IdleAction::Sway:     return {{5, 420}, {4, 370}, {5, 360}, {0, 220}};
        case IdleAction::Surprise: return {{17, 500}, {1, 180}, {0, 150}};
    }
    return {};
}

// Tail animation belongs exclusively to DANCE, never to idle.
// The round dot is a bunny tail. Fast 120-170ms frames make it wag visibly.
inline Story musicStory() {
    return {
        {11, 160}, {12, 150}, {11, 150}, {8, 140},
        {9, 125}, {10, 125}, {9, 125}, {10, 125},
        {8, 140}, {12, 165}, {11, 150}, {0, 150}
    };
}

inline Story typingStory() {
    return {{13, 115}, {14, 115}, {15, 115}, {14, 115}};
}

class Animator {
public:
    // Typed input always interrupts music; music interrupts only idle.
    // Idle stories run ONLY when there is no typing and no music.
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
            else if (desired == Activity::Music) begin(musicStory(), nowMs);
            else pose_ = 0;
        }

        if (activity_ == Activity::Idle && story_.isEmpty() && nowMs >= idleDue_) {
            const auto action = static_cast<IdleAction>(
                QRandomGenerator::global()->bounded(7));
            begin(idleStory(action), nowMs);
        }

        if (!story_.isEmpty() && nowMs >= nextFrame_) {
            ++frameIndex_;
            if (frameIndex_ >= story_.size()) {
                if (activity_ == Activity::Music) begin(musicStory(), nowMs);
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
    qint64 nextFrame_ = 0;
    qint64 idleDue_ = 8500;
};
} // namespace pet
