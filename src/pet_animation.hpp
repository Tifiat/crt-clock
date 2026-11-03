#pragma once

#include "pet_poses.hpp"
#include "pet_sequence.hpp"
#include <QRandomGenerator>
#include <QVector>
#include <QtGlobal>
#include <utility>

namespace pet {

enum class Activity { Idle, Music, Typing };
enum class IdleAction : int {
    Blink, Peek, Smile, Surprise, Smug, Shy, Sleepy, Confused,
    Greeting, Fidget, Celebrate, Indignant
};
enum class MusicAction : int {
    HeadNod, TailWag, HandGroove, Bounce, Sway, HappyDance,
    LookToBeat, BigDance
};
constexpr int kIdleActionCount = 12;
constexpr int kMusicActionCount = 8;

struct Frame {
    int pose;
    int durationMs;
};
using Story = QVector<Frame>;

inline int duration(const Story &story) {
    int sum = 0;
    for (const Frame &f : story) sum += f.durationMs;
    return sum;
}

// Authored multi-second activities. The 100-200ms blink is a brief
// moment INSIDE a complete story, never an independent glitchy action.
inline Story idleStory(IdleAction action) {
    switch (action) {
        case IdleAction::Blink:
            return {{F00, 600},{F01, 160},{F00, 700},{F01, 160},{F00, 900}};
        case IdleAction::Peek:
            return {{F00, 450},{F02, 650},{F00, 550},
                    {F02, 700},{F00, 1400}};
        case IdleAction::Smile:
            return {{F00, 450},{F17, 850},{F04, 850},{F17, 850},{F00, 1000}};
        case IdleAction::Surprise:
            return {{F00, 500},{F05, 900},{F15, 750},{F01, 170},{F00, 1100}};
        case IdleAction::Smug:
            return {{F00, 550},{F06, 950},{F09, 450},{F06, 1050},{F00, 1000}};
        case IdleAction::Shy:
            return {{F00, 450},{F08, 750},{F07, 850},{F08, 700},{F00, 850}};
        case IdleAction::Sleepy:
            return {{F00, 800},{F13, 1150},{F01, 190},{F13, 1450},{F00, 1400}};
        case IdleAction::Confused:
            return {{F00, 550},{F11, 800},{F02, 700},{F11, 700},{F00, 900}};
        case IdleAction::Greeting:
            return {{H00, 450},{H01, 500},{H00, 500},{H01, 500},{H00, 1050}};
        case IdleAction::Fidget:
            return {{H00, 500},{H02, 650},{H01, 650},
                    {H02, 650},{H00, 1050}};
        case IdleAction::Celebrate:
            return {{F00, 600},{F04, 950},{F16, 750},{F04, 850},{F00, 1000}};
        case IdleAction::Indignant:
            return {{F00, 500},{F12, 850},{F05, 650},{F12, 700},{F00, 1000}};
    }
    return {};
}

// One full performance per selected action. Each music action is eventually
// selected because the Animator runs a permutation of all eight actions.
inline Story musicStory(MusicAction action) {
    switch (action) {
        case MusicAction::HeadNod:
            return {{F00, 400},{NodDown, 400},{NodUp, 400},
                    {NodDown, 400},{NodUp, 400},{NodDown, 400},
                    {NodUp, 400},{F00, 850}};
        case MusicAction::TailWag:
            // The original 4fc9d30 dance turns the pet around BEFORE the
            // central bunny tail starts wagging, then returns to face front.
            return {{F00, 360},{H03, 260},{B00, 360},
                    {B01, 145},{B02, 145},{B01, 145},{B02, 145},
                    {B01, 145},{B02, 145},{B01, 145},{B02, 145},
                    {B01, 145},{B02, 145},{B00, 380},
                    {H03, 260},{F00, 360}};
        case MusicAction::HandGroove:
            return {{H00, 400},{H01, 450},{H02, 450},{H03, 450},
                    {H01, 450},{H02, 450},{H03, 450},{H00, 700}};
        case MusicAction::Bounce:
            return {{F00, 350},{BounceUp, 420},{BounceDown, 420},
                    {BounceUp, 420},{BounceDown, 420},
                    {BounceUp, 420},{BounceDown, 420},{F00, 620}};
        case MusicAction::Sway:
            return {{F00, 550},{SwayLeft, 500},{SwayRight, 500},
                    {SwayLeft, 500},{SwayRight, 500},
                    {SwayLeft, 500},{F00, 700}};
        case MusicAction::HappyDance:
            return {{F00, 450},{H04, 470},{H05, 470},{F16, 600},
                    {H04, 470},{H05, 470},{F04, 600},{F00, 600}};
        case MusicAction::LookToBeat:
            return {{F00, 480},{F02, 480},{F00, 480},
                    {F02, 480},{F00, 480},{F02, 480},{F00, 680}};
        case MusicAction::BigDance:
            return {{F00, 450},{H03, 500},{BounceUp, 480},
                    {H04, 480},{BounceDown, 480},{H05, 480},
                    {H03, 500},{F00, 780}};
    }
    return {};
}

inline Story typingStory() {
    return {{T00, 115},{T01, 115},{T02, 115},{T01, 115}};
}

class Animator {
public:
    bool advance(qint64 nowMs, bool typing, bool playing) {
        const Activity desired = typing ? Activity::Typing :
                                 playing ? Activity::Music : Activity::Idle;
        if (desired != activity_) {
            activity_ = desired;
            story_.clear();
            frameIndex_ = 0;
            idleDue_ = nowMs + 1200;
            if (desired == Activity::Typing) begin(typingStory(), nowMs);
            else if (desired == Activity::Music) nextMusic(nowMs);
            else pose_ = F00;
        }

        if (activity_ == Activity::Idle && story_.isEmpty() && nowMs >= idleDue_)
            begin(idleStory(static_cast<IdleAction>(idleCycle_.next())), nowMs);

        if (!story_.isEmpty() && nowMs >= nextFrame_) {
            ++frameIndex_;
            if (frameIndex_ >= story_.size()) {
                if (activity_ == Activity::Music) nextMusic(nowMs);
                else if (activity_ == Activity::Typing) begin(typingStory(), nowMs);
                else {
                    story_.clear();
                    pose_ = F00;
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
        begin(musicStory(static_cast<MusicAction>(musicCycle_.next())), nowMs);
    }
    void begin(Story sequence, qint64 nowMs) {
        story_ = std::move(sequence);
        frameIndex_ = 0;
        if (story_.isEmpty()) { pose_ = F00; return; }
        pose_ = story_[0].pose;
        nextFrame_ = nowMs + story_[0].durationMs;
    }

    Activity activity_ = Activity::Idle;
    Story story_;
    int frameIndex_ = 0;
    int pose_ = F00;
    int lastPose_ = -1;
    ShuffleBag musicCycle_{kMusicActionCount};
    ShuffleBag idleCycle_{kIdleActionCount};
    qint64 nextFrame_ = 0;
    qint64 idleDue_ = 8500;
};
} // namespace pet
