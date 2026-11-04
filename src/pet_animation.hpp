#pragma once

#include "pet_poses.hpp"
#include "pet_sequence.hpp"
#include <QRandomGenerator>
#include <QVector>
#include <algorithm>
#include <initializer_list>
#include <QtGlobal>
#include <utility>

namespace pet {

enum class Activity { Idle, Music, Typing };
enum class IdleAction : int {
    Blink, Peek, Smile, Surprise, Smug, Shy, Sleepy, Confused,
    Greeting, Fidget, Celebrate, Indignant, Patrol, Fetch
};
enum class MusicAction : int {
    HeadNod, TailWag, HandGroove, Bounce, Sway, HappyDance,
    LookToBeat, BigDance, EyesClosedNotes, SideStep, WinkGroove
};
constexpr int kIdleActionCount = 14;
constexpr int kMusicActionCount = 11;
constexpr int kMusicBeatMs = 280;
constexpr int kDefaultAutonomousSpeed = 100;
constexpr int kMinAutonomousSpeed = 50;
constexpr int kMaxAutonomousSpeed = 180;

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
            return {{F00, 600},{F01, 230},{F00, 700},{F01, 230},{F00, 900}};
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
            // Hold the happy face longer. JoyTail independently wags for
            // as long as any story displays the joyful > < eyes.
            return {{F00, 600},{F04, 1650},{F16, 750},{F04, 850},{F00, 1000}};
        case IdleAction::Indignant:
            return {{F00, 500},{F12, 850},{F05, 650},{F12, 700},{F00, 1000}};
        case IdleAction::Patrol:
            // Casual side steps, then back to the same resting spot.
            return {{F00, 700}, {IdleStepLeft, 600}, {F02, 650},
                    {IdleStepRight, 600}, {F00, 600},
                    {IdleStepLeft, 600}, {IdleStepRight, 600}, {F00, 950}};
        case IdleAction::Fetch:
            // Rare, deliberately complete mini-story. Shrinking and gift
            // are authored ONLY for these new pose IDs, not generic animation.
            return {{F00, 700}, {IdleStepLeft, 500}, {AwayFace, 650},
                    {AwayNear, 600}, {AwayFar, 600}, {AwayTiny, 900},
                    {AwayFar, 650}, {AwayReturn, 650}, {AwayFace, 500},
                    {GiftReturn, 1350}, {F04, 700}, {F00, 1000}};
    }
    return {};
}

// All music poses advance on the same beat, including the backside wag.
// The standard groove runs between randomly shuffled featured performances.
inline Story musicBeatStory(std::initializer_list<int> poses) {
    Story story;
    story.reserve(static_cast<qsizetype>(poses.size()));
    for (int pose : poses) story.append({pose, kMusicBeatMs});
    return story;
}

inline Story musicGrooveStory() {
    // Keep the baseline mouth neutral. Facial changes belong to the
    // featured dances, not a smile flashing twice every 2.24 seconds.
    return musicBeatStory({F00, NodDown, NodUp, F00,
                           NodDown, NodUp, F00, F00});
}

// Full featured performances: consistent movement tempo and a neutral exit.
// Each is selected once from ShuffleBag before the next shuffled cycle.
inline Story musicStory(MusicAction action) {
    switch (action) {
        case MusicAction::HeadNod:
            return musicBeatStory({F00, NodDown, NodUp, NodDown, NodUp,
                                   NodDown, NodUp, NodDown, NodUp,
                                   NodDown, NodUp, F00});
        case MusicAction::TailWag:
            // Preserve the historic backside silhouette and central tail.
            return musicBeatStory({F00, H03, B00, B01, B02, B01, B02,
                                   B01, B02, B01, B02, B00, H03, F00});
        case MusicAction::HandGroove:
            return musicBeatStory({H00, H01, H02, H03, H01, H02,
                                   H03, H01, H02, H03, H01, H00});
        case MusicAction::Bounce:
            return musicBeatStory({F00, BounceUp, BounceDown, BounceUp,
                                   BounceDown, BounceUp, BounceDown,
                                   BounceUp, BounceDown, BounceUp,
                                   BounceDown, F00});
        case MusicAction::Sway:
            return musicBeatStory({F00, SwayLeft, SwayRight, SwayLeft,
                                   SwayRight, SwayLeft, SwayRight,
                                   SwayLeft, SwayRight, SwayLeft,
                                   SwayRight, F00});
        case MusicAction::HappyDance:
            return musicBeatStory({F00, H04, H05, F16, H04, H05,
                                   F04, H04, H05, F16, H04, F00});
        case MusicAction::LookToBeat:
            // Hold occasional winks for two beats; not an eye flip every 280ms.
            return musicBeatStory({F00, F02, F02, F09, F09, F02,
                                   F02, F10, F10, F02, F02, F00});
        case MusicAction::BigDance:
            return musicBeatStory({F00, H03, BounceUp, H04,
                                   BounceDown, H05, H03, BounceUp,
                                   H04, BounceDown, H05, F00});
        case MusicAction::EyesClosedNotes:
            return musicBeatStory({F00, ListenNote0, ListenNote1,
                                   ListenNote2, ListenNote3, ListenNote0,
                                   ListenNote1, ListenNote2, ListenNote3,
                                   ListenNote0, ListenNote1, F00});
        case MusicAction::SideStep:
            return musicBeatStory({F00, StepLeft, StepRight, StepLeft,
                                   StepRight, StepLeft, StepRight, StepLeft,
                                   StepRight, StepLeft, StepRight, F00});
        case MusicAction::WinkGroove:
            // The arms stay on beat; eye changes get breathing room.
            return musicBeatStory({F00, F09, F09, H01, H02, F00,
                                   F10, F10, H01, H02, F00, F00});
    }
    return {};
}

inline Story typingStory() {
    return {{T00, 115},{T01, 115},{T02, 115},{T01, 115}};
}

// The slider scales idle/music cadence and idle intervals; the typing
// timeline is deliberately kept at its fixed 115ms responsiveness.
class Animator {
public:
    void setAutonomousSpeed(int percent, qint64 nowMs) {
        const int next = std::clamp(percent, kMinAutonomousSpeed, kMaxAutonomousSpeed);
        if (next == speed_) return;
        if (activity_ != Activity::Typing) {
            if (!story_.isEmpty() && nextFrame_ > nowMs)
                nextFrame_ = nowMs +
                    std::max<qint64>(1, (nextFrame_ - nowMs) * speed_ / next);
            if (activity_ == Activity::Idle && idleDue_ > nowMs)
                idleDue_ = nowMs +
                    std::max<qint64>(1, (idleDue_ - nowMs) * speed_ / next);
        }
        speed_ = next;
    }

    bool advance(qint64 nowMs, bool typing, bool playing) {
        const Activity desired = typing ? Activity::Typing :
                                 playing ? Activity::Music : Activity::Idle;
        if (desired != activity_) {
            // A featured dance is only considered completed after its final
            // frame. Typing/pausing must not silently consume a bag action.
            if (activity_ == Activity::Music && inFeaturedMusic_)
                pendingFeatured_ = currentFeatured_;
            activity_ = desired;
            story_.clear();
            frameIndex_ = 0;
            inFeaturedMusic_ = false;
            currentFeatured_ = -1;
            idleDue_ = nowMs + scaledDuration(1200);
            if (desired == Activity::Typing) begin(typingStory(), nowMs);
            else if (desired == Activity::Music) begin(musicGrooveStory(), nowMs);
            else pose_ = F00;
        }

        if (activity_ == Activity::Idle && story_.isEmpty() && nowMs >= idleDue_)
            begin(idleStory(static_cast<IdleAction>(idleCycle_.next())), nowMs);

        if (!story_.isEmpty() && nowMs >= nextFrame_) {
            ++frameIndex_;
            if (frameIndex_ >= story_.size()) {
                if (activity_ == Activity::Music) {
                    if (inFeaturedMusic_) {
                        inFeaturedMusic_ = false;
                        currentFeatured_ = -1;
                        begin(musicGrooveStory(), nowMs);
                    } else {
                        currentFeatured_ = pendingFeatured_ >= 0
                            ? pendingFeatured_ : musicCycle_.next();
                        pendingFeatured_ = -1;
                        inFeaturedMusic_ = true;
                        begin(musicStory(static_cast<MusicAction>(currentFeatured_)),
                              nowMs);
                    }
                } else if (activity_ == Activity::Typing) {
                    begin(typingStory(), nowMs);
                } else {
                    story_.clear();
                    pose_ = F00;
                    idleDue_ = nowMs +
                        scaledDuration(QRandomGenerator::global()->bounded(6500, 14500));
                }
            } else {
                pose_ = story_[frameIndex_].pose;
                nextFrame_ = nowMs + frameDuration(story_[frameIndex_].durationMs);
            }
        }

        const bool changed = pose_ != lastPose_;
        lastPose_ = pose_;
        return changed;
    }

    [[nodiscard]] int pose() const { return pose_; }
    [[nodiscard]] Activity activity() const { return activity_; }
    [[nodiscard]] bool idleResting() const {
        return activity_ == Activity::Idle && story_.isEmpty();
    }
    [[nodiscard]] int autonomousSpeed() const { return speed_; }
    [[nodiscard]] bool featuredMusic() const {
        return activity_ == Activity::Music && inFeaturedMusic_;
    }
    [[nodiscard]] int currentFeaturedAction() const {
        return featuredMusic() ? currentFeatured_ : -1;
    }

private:
    [[nodiscard]] int scaledDuration(int durationMs) const {
        return std::max(40, (durationMs * 100 + speed_ / 2) / speed_);
    }
    [[nodiscard]] int frameDuration(int durationMs) const {
        return activity_ == Activity::Typing ? durationMs : scaledDuration(durationMs);
    }
    void begin(Story sequence, qint64 nowMs) {
        story_ = std::move(sequence);
        frameIndex_ = 0;
        if (story_.isEmpty()) { pose_ = F00; return; }
        pose_ = story_[0].pose;
        nextFrame_ = nowMs + frameDuration(story_[0].durationMs);
    }

    Activity activity_ = Activity::Idle;
    Story story_;
    int frameIndex_ = 0;
    int pose_ = F00;
    int lastPose_ = -1;
    int speed_ = kDefaultAutonomousSpeed;
    bool inFeaturedMusic_ = false;
    int currentFeatured_ = -1;
    int pendingFeatured_ = -1;
    ShuffleBag musicCycle_{kMusicActionCount};
    ShuffleBag idleCycle_{kIdleActionCount};
    qint64 nextFrame_ = 0;
    qint64 idleDue_ = 8500;
};
} // namespace pet
