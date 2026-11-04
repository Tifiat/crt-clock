#pragma once

#include "pet_poses.hpp"
#include "pet_gift.hpp"
#include "pet_sequence.hpp"
#include <QRandomGenerator>
#include <QVector>
#include <algorithm>
#include <initializer_list>
#include <cmath>
#include <numbers>
#include <QtGlobal>
#include <utility>

namespace pet {

enum class Activity { Idle, Music, Typing };
enum class IdleAction : int {
    Blink, Peek, Smile, Surprise, Smug, Shy, Sleepy, Confused,
    Greeting, Fidget, Celebrate, Indignant, Patrol, Fetch, WinkLeftRight
};
enum class MusicAction : int {
    HeadNod, TailWag, HandGroove, Bounce, Sway, HappyDance,
    BigDance, EyesClosedNotes, SideStep
};
constexpr int kIdleActionCount = 15;
constexpr int kMusicActionCount = 9;
constexpr int kMusicBeatMs = 280;
constexpr int kDefaultAutonomousSpeed = 100;
constexpr int kMinAutonomousSpeed = 50;
constexpr int kMaxAutonomousSpeed = 180;

// Breathing only changes position at phases 2 and 5. The other phases
// have the same visible position and should not trigger duplicate paints.
constexpr int breathOffset(int phase) {
    return phase == 2 ? -1 : phase == 5 ? 1 : 0;
}

struct Frame {
    int pose;
    int durationMs;
    GiftCue cue = GiftCue::Inactive;
    int moveFrom = 0;
    int moveTo = 0;
    bool mirror = false;
    int streak = 0;
    bool absoluteDuration = false; // hold/read emotion; speed affects gestures
    int carryDx = 0; // carried gift offset while entering the canvas
    bool blush = false; // only the held shy expression
};

// Eyegaze and sleep need a readable minimum independent of fast limb tempo.
// Preserves the existing 50..180% slider for active motion.
inline Frame hold(int pose, int ms, GiftCue cue = GiftCue::Inactive) {
    Frame frame{pose, ms, cue};
    frame.absoluteDuration = true;
    return frame;
}

inline Frame shyHold(int pose, int ms) {
    Frame f = hold(pose, ms);
    f.blush = true;
    return f;
}

inline Frame walk(int pose, int ms, int from, int to, bool mirror = false) {
    Frame frame{pose, ms};
    frame.moveFrom = from;
    frame.moveTo = to;
    frame.mirror = mirror;
    return frame;
}

inline Frame restAt(int pose, int ms, int offset, bool mirror = false) {
    Frame f = hold(pose, ms);
    f.moveFrom = offset;
    f.moveTo = offset;
    f.mirror = mirror;
    return f;
}

inline Frame gesture(int pose, int ms, int from, int to, int streak = 0) {
    Frame frame = walk(pose, ms, from, to);
    frame.streak = streak;
    return frame;
}
using Story = QVector<Frame>;

inline int duration(const Story &story) {
    int sum = 0;
    for (const Frame &f : story) sum += f.durationMs;
    return sum;
}

inline Frame carryingWalk(int pose, int ms, int from, int to) {
    Frame f = walk(pose, ms, from, to);
    f.cue = GiftCue::Carry;
    f.absoluteDuration = true;
    // Both the pet and carried prop move on the same interpolated track.
    return f;
}

// One deliberate idle mini-story with five independent endings.
// The departure ends in a genuinely invisible frame; returning from the
// LEFT uses regular front-facing glyphs at full size, not the rear silhouette.
inline Story fetchStory(GiftKind kind) {
    Story story = {
        {F00, 750}, {IdleStepLeft, 650}, {AwayFace, 730},
        {AwayNear, 680}, {AwayFar, 650}, {AwayTiny, 600},
        hold(AwayGone, 3300), // stay gone a full 3.3s at any gesture speed
        // The gift is already IN HIS HAND as he enters from the left.
        // The first frame is COMPLETELY off the left edge. Gift at x=-53
        // peeks out just before the pet's rightmost paw enters.
        carryingWalk(ReturnPeek, 1000, -420, -275),
        carryingWalk(ReturnSide, 950, -275, -160),
        carryingWalk(ReturnApproach, 850, -160, -70),
        carryingWalk(ReturnCenter, 750, -70, 0),
        hold(ReturnCenter, 1300, GiftCue::Present)
    };
    switch (kind) {
        case GiftKind::StarFly:
            story += Story{hold(H01, 1600, GiftCue::Transfer),
                           hold(F02, 5400, GiftCue::FlyStraight),
                           hold(SadSoft, 2200),hold(F13, 1000),
                           {F00, 1100}};
            break;
        case GiftKind::ArcFly:
            story += Story{hold(H01, 1650, GiftCue::Transfer),
                           hold(F02, 5650, GiftCue::FlyArc),
                           hold(SadSoft, 2300),hold(F13, 1100),
                           {F00, 1150}};
            break;
        case GiftKind::Pulse:
            story += Story{hold(F04, 5100, GiftCue::Pulse),
                           hold(F17, 2000), {F00, 1100}};
            break;
        case GiftKind::BurstSad:
            story += Story{hold(ReturnCenter, 1700, GiftCue::Charge),
                           hold(F05, 1700, GiftCue::Burst),
                           hold(SadComic, 1950),
                           hold(SadComic, 1350, GiftCue::TearLeft),
                           hold(SadComic, 1350, GiftCue::TearRight),
                           hold(SadComic, 1350, GiftCue::TearLeft),
                           hold(SadComic, 2100),
                           hold(SadSoft, 1900),hold(F13, 1200),
                           {F00, 1300}};
            break;
        case GiftKind::BurstCry:
            story += Story{hold(ReturnCenter, 1700, GiftCue::Charge),
                           hold(F05, 1850, GiftCue::Burst),
                           hold(SadCry, 2150),
                           hold(SadCry, 1450, GiftCue::TearLeft),
                           hold(SadCry, 1450, GiftCue::TearRight),
                           hold(SadCry, 1450, GiftCue::TearLeft),
                           hold(SadCry, 1450, GiftCue::TearRight),
                           hold(SadCry, 2300),
                           hold(SadSoft, 1950),hold(F13, 1350),
                           {F00, 1350}};
            break;
    }
    return story;
}

// A gift flight shows two deliberate, slow expressions rather than a
// neutral face for the entire 4.4s. No extra story frames are consumed, so
// the gift trajectory and its comet tail stay continuous.
inline int visiblePose(int scheduled, const GiftVisual &effect) {
    if ((effect.cue == GiftCue::FlyStraight ||
         effect.cue == GiftCue::FlyArc) && effect.progress >= .68)
        return F07; // follows the star, then looks wistful as it leaves
    return scheduled;
}

// Authored multi-second activities. The 100-200ms blink is a brief
// moment INSIDE a complete story, never an independent glitchy action.
inline Story idleStory(IdleAction action) {
    switch (action) {
        case IdleAction::Blink:
            return {{F00, 600},{F01, 230},{F00, 700},{F01, 230},{F00, 900}};
        case IdleAction::Peek:
            // One single sustained glance, not two rapid look-back cycles.
            return {{F00, 650}, hold(F02, 2050), hold(F00, 1250)};
        case IdleAction::Smile:
            return {{F00, 500},hold(F17, 1100),hold(F04, 1000),
                    hold(F17, 1100),{F00, 1100}};
        case IdleAction::Surprise:
            return {{F00, 550},hold(F05, 1750),hold(F15, 1150),
                    {F01, 240},{F00, 1100}};
        case IdleAction::Smug:
            return {{F00, 550},hold(F06, 1750),hold(F09, 800),
                    hold(F06, 1600),{F00, 1050}};
        case IdleAction::Shy:
            // One bashful pose, with action-only cheek blush and a small
            // bowed-head bob. No dot/roof/dot eye switching.
            return {{F00, 650}, shyHold(F08, 3000), {F00, 1200}};
        case IdleAction::Sleepy:
            return {{F00, 950},hold(F13, 3400),{F01, 260},
                    hold(F13, 3200),{F00, 1400}};
        case IdleAction::Confused:
            return {{F00, 550},hold(F11, 1400),hold(F02, 1100),
                    hold(F11, 1300),{F00, 1100}};
        case IdleAction::Greeting:
            // More complete waves and slight whole-body sway while waving.
            return {{H00, 650},
                    gesture(H01, 550, 0, -5),
                    gesture(H00, 550, -5, 4),
                    gesture(H01, 550, 4, -5),
                    gesture(H00, 550, -5, 4),
                    gesture(H01, 550, 4, -5),
                    gesture(H00, 550, -5, 4),
                    gesture(H01, 550, 4, 0), {H00, 1100}};
        case IdleAction::Fidget:
            return {{H00, 600},
                    gesture(H02, 540, 0, -6),
                    gesture(H01, 540, -6, 6),
                    gesture(H02, 540, 6, -6),
                    gesture(H01, 540, -6, 6),
                    gesture(H02, 540, 6, -6),
                    gesture(H01, 540, -6, 6),
                    gesture(H02, 540, 6, -6),
                    gesture(H01, 540, -6, 0),
                    {H00, 1050}};
        case IdleAction::Celebrate:
            return {{F00, 650},hold(F04, 1800),{F16, 950},
                    hold(F04, 1500),{F16, 750},hold(F04, 1200),
                    {F00, 1150}};
        case IdleAction::Indignant:
            return {{F00, 650}, {F05, 650}, hold(F12, 2550),
                    hold(F13, 950), {F00, 1200}};
        case IdleAction::Patrol:
            // Continuous linear travel between gait beats. No stationary
            // rests at either edge and no per-step ease-in/out that made
            // the walk look like a stop-motion robot.
            return {{F00, 650},
                    walk(IdleStepRight, 620, 0, 80),
                    walk(IdleStepLeft, 620, 80, 160),
                    walk(IdleStepLeft, 620, 160, 85, true),
                    walk(IdleStepRight, 620, 85, 10, true),
                    walk(IdleStepLeft, 620, 10, -65, true),
                    walk(IdleStepRight, 620, -65, -125, true),
                    walk(IdleStepRight, 620, -125, -65),
                    walk(IdleStepLeft, 620, -65, -25),
                    walk(F00, 600, -25, 0),
                    {F00, 1150}};
        case IdleAction::Fetch:
            return fetchStory(GiftKind::StarFly);
        case IdleAction::WinkLeftRight:
            // Winks belong in calm idle, once per eye, never in music.
            return {{F00, 650}, {F09, 310}, hold(F00, 800),
                    {F10, 310}, hold(F00, 1200)};
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
            // Same 280ms beat, but an actual sustained nod instead of a
            // 2-second glimpse at 146% speed.
            return musicBeatStory({F00, NodDown, NodUp, NodDown, NodUp,
                                   NodDown, NodUp, NodDown, NodUp,
                                   NodDown, NodUp, NodDown, NodUp,
                                   NodDown, NodUp, NodDown, NodUp,
                                   NodDown, NodUp, NodDown, NodUp,
                                   NodDown, NodUp, F00});
        case MusicAction::TailWag:
            return musicBeatStory({F00, H03, B00, B01, B02, B01, B02,
                                   B01, B02, B01, B02, B01, B02,
                                   B01, B02, B00, H03, F00});
        case MusicAction::HandGroove: {
            auto story = musicBeatStory({H00, H01, H02, H03, H01, H02,
                                        H03, H01, H02, H03, H01, H02,
                                        H03, H01, H02, H03, H01, H00});
            int last = 0;
            for (auto &frame : story) {
                const int x = frame.pose == H00 ? 0 :
                              frame.pose == H01 ? -6 : 6;
                frame.moveFrom = last;
                frame.moveTo = x;
                last = x;
            }
            return story;
        }
        case MusicAction::Bounce:
            return musicBeatStory({F00, BounceUp, BounceDown, BounceUp,
                                   BounceDown, BounceUp, BounceDown,
                                   BounceUp, BounceDown, BounceUp,
                                   BounceDown, BounceUp, BounceDown,
                                   BounceUp, BounceDown, F00});
        case MusicAction::Sway: {
            auto story = musicBeatStory({F00, SwayLeft, SwayRight, SwayLeft,
                                        SwayRight, SwayLeft, SwayRight,
                                        SwayLeft, SwayRight, SwayLeft,
                                        SwayRight, SwayLeft, SwayRight,
                                        SwayLeft, SwayRight, SwayLeft,
                                        SwayRight, F00});
            for (auto &frame : story)
                frame.streak = frame.pose == SwayLeft ? -1 :
                               frame.pose == SwayRight ? 1 : 0;
            return story;
        }
        case MusicAction::HappyDance:
            return musicBeatStory({F00, H04, H05, F16, H04, H05,
                                   F04, H04, H05, F16, H04, H05,
                                   F04, H04, H05, F16, H04, F00});
        case MusicAction::BigDance:
            return musicBeatStory({F00, H03, BounceUp, H04,
                                   BounceDown, H05, H03, BounceUp,
                                   H04, BounceDown, H05, H03,
                                   BounceUp, H04, BounceDown, H05,
                                   H03, F00});
        case MusicAction::EyesClosedNotes:
            return musicBeatStory({F00, ListenNote0, ListenNote1,
                                   ListenNote2, ListenNote3, ListenNote0,
                                   ListenNote1, ListenNote2, ListenNote3,
                                   ListenNote0, ListenNote1, ListenNote2,
                                   ListenNote3, ListenNote0, ListenNote1,
                                   ListenNote2, ListenNote3, F00});
        case MusicAction::SideStep:
            return musicBeatStory({F00, StepLeft, StepRight, StepLeft,
                                   StepRight, StepLeft, StepRight, StepLeft,
                                   StepRight, StepLeft, StepRight, StepLeft,
                                   StepRight, StepLeft, StepRight, F00});
    }
    return {};
}

inline Story typingStory() {
    return {{T00, 115},{T01, 115},{T02, 115},{T01, 115}};
}

// The slider scales gesture cadence, not held emotions/idle gaps; typing
// remains at a fixed 115ms for responsive global keyboard activity.
class Animator {
public:
    // One-shot previews do not consume the normal idle/music shuffle queues.
    // The GUI can explicitly play every authored action, including typing,
    // without waiting for chance or an actual MPRIS player.
    void previewStory(Story sequence, Activity mode, qint64 nowMs) {
        if (activity_ == Activity::Music && inFeaturedMusic_)
            pendingFeatured_ = currentFeatured_;
        previewActive_ = true;
        previewActivity_ = mode;
        activity_ = mode;
        inFeaturedMusic_ = false;
        currentFeatured_ = -1;
        begin(std::move(sequence), nowMs);
    }
    void previewGift(GiftKind kind, qint64 nowMs) {
        giftKind_ = kind;
        previewStory(fetchStory(kind), Activity::Idle, nowMs);
    }
    void previewIdle(IdleAction kind, qint64 nowMs) {
        if (kind == IdleAction::Fetch) {
            previewGift(GiftKind::StarFly, nowMs);
        } else {
            previewStory(idleStory(kind), Activity::Idle, nowMs);
        }
    }
    void previewIdleBreathing(qint64 nowMs) {
        previewStory({{F00, 6500}}, Activity::Idle, nowMs);
    }
    void previewMusic(MusicAction kind, qint64 nowMs) {
        previewStory(musicStory(kind), Activity::Music, nowMs);
    }
    void previewGroove(qint64 nowMs) {
        previewStory(musicGrooveStory(), Activity::Music, nowMs);
    }
    void previewTyping(qint64 nowMs) {
        Story typing;
        for (int repeat = 0; repeat < 5; ++repeat)
            typing += typingStory(); // 2.3s to inspect paw motion.
        previewStory(std::move(typing), Activity::Typing, nowMs);
    }
    [[nodiscard]] bool previewing(Activity mode) const {
        return previewActive_ && activity_ == mode;
    }
    void stopPreview(qint64 nowMs) {
        if (!previewActive_) return;
        previewActive_ = false;
        story_.clear();
        inFeaturedMusic_ = false;
        currentFeatured_ = -1;
        pose_ = F00;
        activity_ = Activity::Idle;
        idleDue_ = nowMs + scaledDuration(2500);
    }

    void setAutonomousSpeed(int percent, qint64 nowMs) {
        const int next = std::clamp(percent, kMinAutonomousSpeed, kMaxAutonomousSpeed);
        if (next == speed_) return;
        if (activity_ != Activity::Typing) {
            if (!story_.isEmpty() && nextFrame_ > nowMs &&
                !story_[frameIndex_].absoluteDuration)
                nextFrame_ = nowMs +
                    std::max<qint64>(1, (nextFrame_ - nowMs) * speed_ / next);
            // Long quiet gaps between idle stories do not change with
            // gesture tempo. Only the active gesture frame is rescaled.
        }
        speed_ = next;
    }

    bool advance(qint64 nowMs, bool typing, bool playing) {
        bool frameChanged = false;
        // The explicit preview plays through even with music in the
        // background, but typing retains its normal highest priority.
        if (typing) previewActive_ = false;
        const Activity desired = typing ? Activity::Typing :
                                 previewActive_ ? previewActivity_ :
                                 playing ? Activity::Music : Activity::Idle;
        if (desired != activity_) {
            frameChanged = true;
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

        if (activity_ == Activity::Idle && !previewActive_ &&
            story_.isEmpty() && nowMs >= idleDue_) {
            frameChanged = true;
            const auto action = static_cast<IdleAction>(idleCycle_.next());
            if (action == IdleAction::Fetch) {
                giftKind_ = static_cast<GiftKind>(giftCycle_.next());
                begin(fetchStory(giftKind_), nowMs);
            } else {
                begin(idleStory(action), nowMs);
            }
        }

        if (!story_.isEmpty() && nowMs >= nextFrame_) {
            frameChanged = true;
            ++frameIndex_;
            if (frameIndex_ >= story_.size()) {
                if (previewActive_) {
                    // Finish the selected test cleanly. Never switch straight
                    // into another dance or repeat typing ad infinitum.
                    stopPreview(nowMs);
                } else if (activity_ == Activity::Music) {
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
                    previewActive_ = false;
                    // Atmosphere/idle spacing is independent of rapid
                    // gestures: higher speed must not spam every idle event.
                    idleDue_ = nowMs +
                        QRandomGenerator::global()->bounded(7500, 16000);
                }
            } else {
                pose_ = story_[frameIndex_].pose;
                frameStartMs_ = nowMs;
                nextFrame_ = nowMs + frameDuration(story_[frameIndex_]);
            }
        }

        const bool crying = activity_ == Activity::Idle && !story_.isEmpty() &&
            isBurst(giftKind_) && (pose_ == SadComic || pose_ == SadCry);
        if (crying) {
            if (cryStartMs_ < 0) cryStartMs_ = nowMs;
        } else {
            cryStartMs_ = -1;
        }

        const bool changed = pose_ != lastPose_;
        lastPose_ = pose_;
        // Continuous whole-body movement must repaint on every 40ms tick,
        // even when the face/pose ID has not changed. Otherwise Patrol
        // "teleports" between stops rather than walking across the canvas.
        const bool interpolating = !story_.isEmpty() &&
            story_[frameIndex_].moveFrom != story_[frameIndex_].moveTo;
        const bool particles = activity_ == Activity::Idle && !story_.isEmpty() &&
            (story_[frameIndex_].cue != GiftCue::Inactive || crying);
        return changed || frameChanged || interpolating || particles;
    }

    [[nodiscard]] int pose() const { return pose_; }
    [[nodiscard]] Activity activity() const { return activity_; }
    [[nodiscard]] bool idleResting() const {
        return activity_ == Activity::Idle && story_.isEmpty();
    }
    [[nodiscard]] bool idleBreathing() const {
        // A faint 1px breath in long still frames, but no breathing on
        // retreat, walking, disappearing, returning or during gift effects.
        if (activity_ != Activity::Idle) return false;
        if (story_.isEmpty()) return true;
        const int pose = pose_;
        const Frame &f = story_[frameIndex_];
        if (pose == AwayGone || (pose >= AwayFace && pose <= AwayReturn) ||
            (pose >= ReturnPeek && pose <= ReturnApproach) ||
            (f.moveFrom != f.moveTo) ||
            pose == IdleStepLeft || pose == IdleStepRight) return false;
        // Present, Charge, Pulse, flying-star emotions and tears can all
        // coexist with subtle breathing. Only physically active reactions
        // (carrying, tossing, explosion) suspend it.
        return f.cue != GiftCue::Carry && f.cue != GiftCue::Transfer &&
               f.cue != GiftCue::Burst;
    }
    [[nodiscard]] MotionVisual motionVisual(qint64 nowMs) const {
        if (story_.isEmpty()) return {};
        const Frame &f = story_[frameIndex_];
        const double t = std::clamp(double(nowMs - frameStartMs_) /
                                    frameDuration(f), 0.0, 1.0);
        // Constant horizontal speed across short gait frames. Each step
        // briefly lifts the body and returns to y=0 at the next boundary.
        const bool moving = f.moveFrom != f.moveTo;
        const double y = moving ? -3.0*std::sin(std::numbers::pi*t)
                         : f.blush ? 1.6*std::sin(std::numbers::pi*t) : 0.0;
        return {f.moveFrom + (f.moveTo - f.moveFrom) * t,
                y, f.mirror, f.streak,
                moving && (f.pose == IdleStepLeft || f.pose == IdleStepRight),
                f.blush};
    }
    [[nodiscard]] GiftVisual giftVisual(qint64 nowMs) const {
        if (activity_ != Activity::Idle || story_.isEmpty()) return {};
        const Frame &frame = story_[frameIndex_];
        const int ms = frameDuration(frame);
        // All SadComic/SadCry frames form one continuous crying phase,
        // including the initial and final holds without explicit tear cues.
        if (cryStartMs_ >= 0 && isBurst(giftKind_) &&
            (frame.pose == SadComic || frame.pose == SadCry)) {
            const GiftCue cue = frame.cue == GiftCue::TearRight
                ? GiftCue::TearRight : GiftCue::TearLeft;
            return {cue, giftKind_,
                    std::clamp(double(nowMs - frameStartMs_) / ms, 0.0, 1.0),
                    0.0, double(std::max<qint64>(0, nowMs - cryStartMs_))};
        }
        if (frame.cue == GiftCue::Inactive) return {};
        const auto motion = motionVisual(nowMs);
        return {frame.cue, giftKind_,
                std::clamp(double(nowMs - frameStartMs_) / ms, 0.0, 1.0),
                frame.cue == GiftCue::Carry ? motion.offsetX : frame.carryDx};
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
    [[nodiscard]] int frameDuration(const Frame &frame) const {
        return (activity_ == Activity::Typing || frame.absoluteDuration)
            ? frame.durationMs : scaledDuration(frame.durationMs);
    }
    void begin(Story sequence, qint64 nowMs) {
        story_ = std::move(sequence);
        frameIndex_ = 0;
        if (story_.isEmpty()) { pose_ = F00; return; }
        pose_ = story_[0].pose;
        cryStartMs_ = -1;
        frameStartMs_ = nowMs;
        nextFrame_ = nowMs + frameDuration(story_[0]);
    }

    Activity activity_ = Activity::Idle;
    Story story_;
    int frameIndex_ = 0;
    int pose_ = F00;
    int lastPose_ = -1;
    int speed_ = kDefaultAutonomousSpeed;
    bool inFeaturedMusic_ = false;
    bool previewActive_ = false;
    Activity previewActivity_ = Activity::Idle;
    int currentFeatured_ = -1;
    int pendingFeatured_ = -1;
    ShuffleBag musicCycle_{kMusicActionCount};
    ShuffleBag idleCycle_{kIdleActionCount};
    ShuffleBag giftCycle_{kGiftKindCount};
    GiftKind giftKind_ = GiftKind::StarFly;
    qint64 frameStartMs_ = 0;
    qint64 nextFrame_ = 0;
    qint64 cryStartMs_ = -1;
    qint64 idleDue_ = 8500;
};
} // namespace pet
