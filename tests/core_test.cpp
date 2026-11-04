#include <QPointF>
#include <QGuiApplication>
#include <QString>
#include <QFile>
#include <QTemporaryDir>
#include <QByteArray>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "pet_animation.hpp"
#include "pet_poses.hpp"
#include "pet_render.hpp"
#include "pet_sprite_cache.hpp"
#include "pet_gift_render.hpp"
#include "pet_tail.hpp"
#include "crt_style.hpp"
#include "matrix_rain.hpp"
#include "perspective.hpp"
#include "perspective_raster_cache.hpp"
// Include Xlib-facing header after all Qt headers: Xlib defines None.
#include "desktop_workspace.hpp"
#include "clock_flicker.hpp"
#include "desktop_autostart.hpp"

static void require(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}
static void close(double a, double b) {
    require(std::abs(a-b) < .001, "coordinates differ");
}

int main(int argc, char **argv) {
    // Sprite raster tests use QFontDatabase. Run headless in CI while
    // preserving the platform selected by an explicit test environment.
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);
    const QString original = QStringLiteral("(づ｡◕‿‿◕｡)づ");
    require(pet::frame(pet::F00).line == original, "original face must be retained");
    require(pet::kFrontColumns == 10 && pet::kBackColumns == 11,
            "approved pose grid changed");
    require(pet::kFrontColumns * pet::kFrontCellPixels == 310,
            "silhouette is no longer compact");
    require(pet::kDefaultFontPixels == 43 && pet::kDefaultStrokeStrength == 25,
            "default pet appearance changed");

    // Compare every approved pose, including historical backside tail frames.
    for (int id = 0; id < pet::kPoseCount; ++id) {
        const pet::Pose p = pet::frame(id);
        require(p.line.size() == (p.back ? pet::kBackColumns : pet::kFrontColumns),
                "unexpected pose column count");
        require(p.headDx >= -3 && p.headDx <= 3, "head moves too far");
        require(p.headDy >= -5 && p.headDy <= 5, "head moves too far");
        require(p.bodyDx >= -9 && p.bodyDx <= 9,
                "pose body must use fixed grid, movement belongs to Animator");
        require(p.bodyDy >= -6 && p.bodyDy <= 6, "body moves too far");
        if (p.back) {
            const bool historicMusicBack = id >= pet::B00 && id <= pet::B02;
            const bool rareIdleBack = id >= pet::AwayFace && id <= pet::AwayReturn;
            require(historicMusicBack || rareIdleBack, "unexpected backside pose");
            require(p.line.at(1) == QChar('(') &&
                    p.line.at(9) == QChar(')'), "backside body geometry changed");
            if (historicMusicBack)
                require(p.line.contains(QStringLiteral("●")), "historic backside tail missing");
            else
                require(!p.line.contains(QStringLiteral("◦")) &&
                        p.line.contains(QStringLiteral("       ")),
                        "departure must not use a central dot as a tail");
        } else {
            require(p.line.at(0) == QChar('('), "left front anchor moved");
            require(p.line.at(2) == QChar(0xFF61), "left cheek moved");
            require(p.line.at(7) == QChar(0xFF61), "right cheek moved");
            require(p.line.at(8) == QChar(')'), "right front anchor moved");
        }
    }
    require(pet::frame(pet::F02).line == QStringLiteral("(づ｡◔‿‿◔｡)づ"),
            "both eyes must look right");
    require(pet::frame(pet::F05).line == QStringLiteral("(づ｡0‿‿0｡)づ"),
            "surprise uses digit zero, not letter O");
    require(pet::frame(pet::F04).line == QStringLiteral("(づ｡>‿‿<｡)づ"),
            "favorite happy expression changed");
    require(pet::frame(pet::B01).line != pet::frame(pet::B02).line,
            "tail must wag inside backside body");
    for (int id = pet::T00; id <= pet::T02; ++id)
        require(pet::frame(id).typing, "typing activity pose missing");

    // Happy eyes drive the same tail animation in ANY story, not just a
    // handful of dedicated poses in Celebrate.
    require(pet::kPoseCount == 62, "extended pose count");
    require(pet::JoyTailLeft == 37 && pet::JoyTailRight == 38,
            "historic pose IDs were renumbered");
    require(pet::ListenNote0 == 39 && pet::StepRight == 44,
            "new music poses must append after historic IDs");
    require(pet::frame(pet::SwayLeft).bodyDx == -8 &&
            pet::frame(pet::SwayRight).bodyDx == 8, "sway remains too subtle");
    require(pet::frame(pet::BounceUp).bodyDy == -6 &&
            pet::frame(pet::BounceDown).bodyDy == 6, "bounce remains too subtle");
    require(pet::frame(pet::StepLeft).bodyDx == -9 &&
            pet::frame(pet::StepRight).bodyDx == 9, "side step amplitude");
    for (int id = pet::ListenNote0; id <= pet::ListenNote3; ++id) {
        const pet::Pose p = pet::frame(id);
        require(p.notePhase == id - pet::ListenNote0,
                "notes must advance through fixed authored phases");
        require(p.line == QStringLiteral("(づ｡-‿‿-｡)づ"),
                "music listening eyes must remain closed");
    }
    require(pet::frame(pet::F00).notePhase == -1 &&
            pet::frame(pet::T00).notePhase == -1,
            "notes must be absent from idle and typing");
    require(pet::IdleStepLeft == 45 && pet::GiftReturn == 52,
            "idle-only pose IDs must append without renumbering old dances");
    for (int id = pet::AwayFace; id <= pet::AwayReturn; ++id)
        require(pet::frame(id).back, "idle fetch retreat lost backside orientation");
    require(pet::frame(pet::AwayNear).retreatScale <
            pet::frame(pet::AwayFace).retreatScale &&
            pet::frame(pet::AwayFar).retreatScale <
            pet::frame(pet::AwayNear).retreatScale &&
            pet::frame(pet::AwayTiny).retreatScale <
            pet::frame(pet::AwayFar).retreatScale,
            "rare idle retreat should shrink as pet walks away");
    require(pet::frame(pet::GiftReturn).carriesGift,
            "historic idle gift pose must remain available");
    require(pet::AwayGone == 53 && pet::SadAngry == 61,
            "gift-specific pose IDs must append after existing set");
    require(pet::frame(pet::AwayGone).hidden,
            "pet must fully disappear between departure and side return");
    const auto returnStory = pet::fetchStory(pet::GiftKind::StarFly);
    int lastTo = -420, returnSteps = 0;
    for (const auto &frame : returnStory) {
        if (frame.cue != pet::GiftCue::Carry) continue;
        require(frame.moveFrom == lastTo && frame.moveTo > frame.moveFrom,
                "return motion must be continuous with no teleporting");
        lastTo = frame.moveTo;
        ++returnSteps;
    }
    require(returnSteps == 4 && lastTo == 0 &&
            pet::giftFlight(0.0, false).x() > 0,
            "entire pet must emerge from outside left edge with a gift");
    require(pet::frame(pet::ReturnPeek).line ==
            QStringLiteral("(づ｡◔‿‿◔｡)っ"),
            "return must face toward the right instead of walking backwards");
    for (int id = pet::ReturnPeek; id <= pet::ReturnCenter; ++id)
        require(!pet::frame(id).back,
                "returning pet must face the viewer, never return backside");
    require(pet::frame(pet::SadComic).eyeDy <= -4 &&
            pet::frame(pet::SadComic).mouthDy >= 4,
            "comic sad eyes must be higher and mouth lower");
    require(pet::frame(pet::SadCry).eyeDy < 0 &&
            pet::frame(pet::SadCry).mouthDy > 0,
            "crying expression must remain legible");
    require(!pet::hasJoyfulEyes(pet::frame(pet::SadComic)) &&
            !pet::hasJoyfulEyes(pet::frame(pet::SadCry)),
            "sad expressions must not trigger joyful tail");
    require(pet::frame(pet::F00).retreatScale == 1.0 &&
            !pet::frame(pet::F00).carriesGift,
            "normal idle silhouette must keep original grid and size");

    require(pet::hasJoyfulEyes(pet::frame(pet::F04)), "happy eyes must wag");
    require(pet::hasJoyfulEyes(pet::frame(pet::F16)), "laughing eyes must wag");
    require(pet::hasJoyfulEyes(pet::frame(pet::H04)), "happy hand pose must wag");
    require(pet::hasJoyfulEyes(pet::frame(pet::H05)), "happy opposite hand pose must wag");
    require(!pet::hasJoyfulEyes(pet::frame(pet::F00)), "normal eyes must not wag");
    require(!pet::hasJoyfulEyes(pet::frame(pet::B01)), "back-facing tail is independent");

    pet::JoyTail tail;
    require(!tail.advance(1000, true) && !tail.visible(), "no tail on first frame");
    require(!tail.advance(1040, true) && !tail.visible(), "wait a second frame");
    require(tail.advance(1080, true) && tail.visible(), "tail appears after two frames");
    const int firstSide = tail.side();
    require(!tail.advance(1120, true), "tail should not move too quickly");
    require(tail.advance(1240, true) && tail.side() == -firstSide,
            "tail must alternate continuously while happy");
    require(tail.advance(1400, true) && tail.side() == firstSide,
            "tail must wag multiple cycles while happy");
    require(tail.advance(1440, false) && !tail.visible(),
            "tail disappears the instant joyful eyes close");
    require(!tail.advance(1480, true) && !tail.visible(), "delay resets for next joy");
    require(!tail.advance(1520, true) && !tail.visible(), "second delay tick");
    require(tail.advance(1560, true) && tail.visible(), "tail restarts on third tick");

    // Celebration stays extended but uses ordinary happy poses, not a
    // special 700ms tail blip. The tail follows the eyes automatically.
    const auto celebration = pet::idleStory(pet::IdleAction::Celebrate);
    require(pet::duration(celebration) == 8000,
            "celebration should be sustained without slowing limb cadence");
    require(celebration.size() == 7,
            "celebration must include multiple joyful cycles");
    require(celebration[1].pose == pet::F04 &&
            celebration[1].durationMs == 1800 &&
            celebration[1].absoluteDuration,
            "celebration happiness must last at high speed settings");

    require(pet::kIdleActionCount == 15, "idle one-shot winks must be included");
    const auto patrol = pet::idleStory(pet::IdleAction::Patrol);
    const auto fetch = pet::idleStory(pet::IdleAction::Fetch);
    require(pet::duration(patrol) >= 4500 && pet::duration(fetch) >= 7000,
            "rare idle stories should be recognizable complete actions");
    bool leftStep = false, rightStep = false, gift = false, farAway = false;
    bool vanished = false, returnsFront = false;
    int patrolRight = 0, patrolLeft = 0;
    bool mirroredReturn = false;
    for (const auto &step : patrol) {
        leftStep |= step.pose == pet::IdleStepLeft;
        rightStep |= step.pose == pet::IdleStepRight;
        patrolRight = std::max(patrolRight, std::max(step.moveFrom, step.moveTo));
        patrolLeft = std::min(patrolLeft, std::min(step.moveFrom, step.moveTo));
        mirroredReturn |= step.mirror && step.moveTo < step.moveFrom;
    }
    require(patrolRight >= 150 && patrolLeft <= -120 && mirroredReturn,
            "Patrol must cross canvas and reverse with mirror");
    for (const auto &step : fetch) {
        gift |= step.cue == pet::GiftCue::Present;
        farAway |= step.pose == pet::AwayTiny;
        vanished |= step.pose == pet::AwayGone && step.durationMs >= 1000;
        returnsFront |= step.pose == pet::ReturnPeek;
    }
    require(leftStep && rightStep && gift && farAway && vanished && returnsFront,
            "idle story must depart, disappear, return facing front and show gift");

    pet::Animator walkingPreview;
    walkingPreview.setAutonomousSpeed(146, 0);
    walkingPreview.previewIdle(pet::IdleAction::Patrol, 0);
    double farthestLeft = 0, farthestRight = 0;
    bool turned = false;
    int movingTicks = 0;
    double lastOffset = 0;
    bool lastMirror = false;
    for (int t = 40; t <= 12000; t += 40) {
        const bool dirty = walkingPreview.advance(t, false, false);
        const auto motion = walkingPreview.motionVisual(t);
        farthestLeft = std::min(farthestLeft, motion.offsetX);
        farthestRight = std::max(farthestRight, motion.offsetX);
        turned |= motion.mirror;
        if (std::abs(motion.offsetX - lastOffset) > .5 ||
            lastMirror != motion.mirror) {
            ++movingTicks;
            require(dirty,
                    "position changes and mirrored turns need actual repaint");
            require(std::abs(motion.offsetX - lastOffset) < 25.0,
                    "patrol must not teleport between walking frames");
        }
        lastOffset = motion.offsetX;
        lastMirror = motion.mirror;
    }
    require(farthestLeft < -110 && farthestRight > 145 && turned &&
            movingTicks > 20,
            "Patrol must cross left/right edges smoothly and turn mirrored at 146%");
    require(pet::breathOffset(-1) == 0 && pet::breathOffset(0) == 0 &&
            pet::breathOffset(1) == 0 && pet::breathOffset(2) == -1 &&
            pet::breathOffset(3) == 0 && pet::breathOffset(4) == 0 &&
            pet::breathOffset(5) == 1,
            "breathing must preserve six-step cadence and visible offsets");

    // Every gift has its own complete ending. Explosion branches alternate
    // falling tears from different eyes, then pass through anger/recovery.
    for (int kind = 0; kind < pet::kGiftKindCount; ++kind) {
        const auto variant = static_cast<pet::GiftKind>(kind);
        const auto story = pet::fetchStory(variant);
        int vanishAt = -1, returnAt = -1, presentAt = -1;
        int tears = 0, bursts = 0, softSad = 0, calm = 0;
        int carried = 0, handoff = 0, flew = 0;
        int lastTearIndex = -1, lastTearAtMs = 0, elapsedMs = 0;
        pet::GiftCue lastTear{}; // Xlib defines None as a macro
        for (int i = 0; i < story.size(); ++i) {
            const auto &step = story[i];
            if (step.pose == pet::AwayGone) vanishAt = i;
            if (step.pose == pet::ReturnPeek) returnAt = i;
            if (step.cue == pet::GiftCue::Present) presentAt = i;
            if (step.cue == pet::GiftCue::Carry) ++carried;
            if (step.cue == pet::GiftCue::Transfer) ++handoff;
            if (step.cue == pet::GiftCue::FlyStraight ||
                step.cue == pet::GiftCue::FlyArc) {
                ++flew;
                require(step.durationMs >= 5200 && step.absoluteDuration,
                        "flight must be long and independent of limb tempo");
            }
            if (step.cue == pet::GiftCue::Burst) ++bursts;
            if (step.pose == pet::SadSoft) ++softSad;
            if (step.pose == pet::F13) ++calm;
            if (step.cue == pet::GiftCue::TearLeft ||
                step.cue == pet::GiftCue::TearRight) {
                require(lastTear != step.cue,
                        "tears must alternate between left and right");
                lastTear = step.cue;
                ++tears;
                lastTearIndex = i;
                lastTearAtMs = elapsedMs;
                require(step.durationMs >= 1000,
                        "tears must be slow enough to read visually");
            }
            elapsedMs += step.durationMs;
            if (returnAt >= 0 && i >= returnAt)
                require(!pet::frame(step.pose).back,
                        "all post-return poses must face forward");
        }
        require(vanishAt >= 0 && vanishAt < returnAt &&
                returnAt < presentAt && story.back().pose == pet::F00,
                "gift story timeline/neutral exit invalid");
        require(story[vanishAt].durationMs >= 3000,
                "pet must stay gone visibly before returning");
        require(carried == 4 && story[returnAt].cue == pet::GiftCue::Carry &&
                story[returnAt].moveFrom <= -410 &&
                story[returnAt].moveTo > story[returnAt].moveFrom,
                "return starts entirely offscreen, gift and pet enter smoothly");
        if (variant == pet::GiftKind::StarFly ||
            variant == pet::GiftKind::ArcFly)
            require(handoff == 1 && flew == 1,
                    "flying gift must transfer from right to left hand");
        require(story[returnAt].pose == pet::ReturnPeek &&
                !pet::frame(story[returnAt].pose).back,
                "return must show the original front-facing pet");
        if (pet::isBurst(variant)) {
            require(bursts == 1 && tears >= 3 && calm >= 1,
                    "burst must cause blast, slow tears and closed-eye recovery");
            require(lastTearIndex >= 0 &&
                    story[lastTearIndex+1].pose == story[lastTearIndex].pose,
                    "pet should keep crying after its final falling tear");
            require(story[lastTearIndex+1].durationMs >= 1500,
                    "hold sad face after final tear, do not pop eyes open");
            require(story[lastTearIndex+2].pose == pet::SadSoft &&
                    story[lastTearIndex+3].pose == pet::F13,
                    "soft sad eyes and sleepy eyes must bridge crying back to idle");
        }
        else if (variant != pet::GiftKind::Pulse)
            require(softSad > 0, "flying gift should get mild sad reaction");
        else require(bursts == 0 && tears == 0,
                     "pulse gift should stay happy");
    }

    // Gesture tempo and the duration of readable facial expressions
    // must be independent: at 146% the limbs stay quick but sleepy
    // eyes remain closed and side glances stay visible.
    const auto sleepy = pet::idleStory(pet::IdleAction::Sleepy);
    require(pet::duration(sleepy) >= 9000 &&
            sleepy[1].absoluteDuration && sleepy[1].durationMs >= 3200,
            "sleeping must be visibly long even at high animation speed");
    const auto peek = pet::idleStory(pet::IdleAction::Peek);
    require(peek.size() == 3 && peek[1].pose == pet::F02 &&
            peek[1].absoluteDuration && peek[1].durationMs >= 1800,
            "side glance must happen ONCE and return to idle");
    const auto shy = pet::idleStory(pet::IdleAction::Shy);
    require(shy.size() == 3 && shy[1].pose == pet::F08 &&
            shy[1].absoluteDuration && shy[1].durationMs >= 2800 &&
            shy[1].blush,
            "Shy must hold one bashful pose and blush, not cycle eyes");
    require(pet::frame(pet::SadCry).line == QStringLiteral("(づ｡•﹏﹏•｡)づ"),
            "crying uses dots rather than semicolons obscuring drawn tears");

    // 0 is slow, 100 is fast. Static scanlines do not use this duration.
    require(perspective::sweepDurationMs(0) == 5000, "slow sweep");
    require(perspective::sweepDurationMs(55) == 2525, "default sweep");
    require(perspective::sweepDurationMs(100) == 500, "fast sweep");
    require(perspective::sweepDurationMs(-1) == 5000, "sweep clamp low");
    require(perspective::sweepDurationMs(101) == 500, "sweep clamp high");

    // Every idle and music story is complete, in range, and returns to neutral.
    for (int n = 0; n < pet::kIdleActionCount; ++n) {
        const auto story = pet::idleStory(static_cast<pet::IdleAction>(n));
        require(pet::duration(story) >= 2400, "idle action too short");
        require(story.size() >= 3, "idle action is just a twitch");
        for (const auto &step : story)
            require(step.pose >= 0 && step.pose < pet::kPoseCount &&
                    step.durationMs > 0, "invalid idle frame");
        require(story.front().pose == pet::F00 || story.front().pose == pet::H00,
                "idle does not enter from neutral");
        require(story.back().pose == pet::F00 || story.back().pose == pet::H00,
                "idle does not return to neutral");
    }
    // Music stays at a single authored 280ms beat across the groove and
    // every featured move. Changes to user speed scale the full timeline.
    const auto groove = pet::musicGrooveStory();
    require(groove.size() == 8 && pet::duration(groove) == 2240,
            "continuous music groove needs eight even beats");
    for (const auto &step : groove) {
        require(step.durationMs == pet::kMusicBeatMs,
                "groove lost common cadence");
        require(step.pose != pet::F17 && step.pose != pet::F16,
                "rapid alternate mouth shapes should not appear in baseline groove");
    }
    require(pet::kMusicActionCount == 9,
            "LookToBeat and WinkGroove must be removed from music");
    const auto winks = pet::idleStory(pet::IdleAction::WinkLeftRight);
    int leftWinks = 0, rightWinks = 0;
    for (const auto &f : winks) {
        leftWinks += f.pose == pet::F09;
        rightWinks += f.pose == pet::F10;
    }
    require(leftWinks == 1 && rightWinks == 1,
            "idle wink should blink each eye only once");
    const auto indignation = pet::idleStory(pet::IdleAction::Indignant);
    int angryFrames = 0;
    for (const auto &f : indignation) angryFrames += f.pose == pet::F12;
    require(angryFrames == 1,
            "indignation should not cycle back to angry eyes repeatedly");
    for (int n = 0; n < pet::kMusicActionCount; ++n) {
        const auto story = pet::musicStory(static_cast<pet::MusicAction>(n));
        require(pet::duration(story) >= 3000, "music action too short");
        require(story.size() >= 4, "music action is too short to read");
        for (const auto &step : story)
            require(step.pose >= 0 && step.pose < pet::kPoseCount &&
                    step.durationMs == pet::kMusicBeatMs,
                    "music frames must keep a common beat");
        require(story.front().pose == pet::F00 || story.front().pose == pet::H00,
                "music does not start with a neutral silhouette");
        require(story.back().pose == pet::F00 || story.back().pose == pet::H00,
                "music does not return to a neutral silhouette");
        if (n != static_cast<int>(pet::MusicAction::TailWag)) {
            for (const auto &step : story)
                require(step.pose < pet::B00 || step.pose > pet::B02,
                        "backside wag must belong only to tail action");
        }
    }
    const auto wag = pet::musicStory(pet::MusicAction::TailWag);
    bool wagLeft = false, wagRight = false;
    for (const auto &step : wag) {
        if (step.pose == pet::B01) wagLeft = true;
        if (step.pose == pet::B02) wagRight = true;
    }
    require(wagLeft && wagRight, "the old tail movement must be restored");
    require(pet::musicStory(pet::MusicAction::HeadNod).size() >= 22,
            "nodding should last a few more seconds at fast tempo");
    const auto swaying = pet::musicStory(pet::MusicAction::Sway);
    bool streakRight = false, streakLeft = false;
    for (const auto &frame : swaying) {
        streakRight |= frame.streak == -1;
        streakLeft |= frame.streak == 1;
    }
    require(streakRight && streakLeft,
            "directional music marks must alternate with side movement");
    require(pet::duration(pet::typingStory()) < 600, "typing must feel responsive");
    for (const auto &step : pet::typingStory())
        require(step.durationMs == 115, "typing timeline must remain unscaled");

    // Real animator transition: continuous groove -> feature -> groove;
    // scaling is independent of typing and can be adjusted live.
    pet::Animator standard;
    standard.advance(0, false, true);
    require(standard.activity() == pet::Activity::Music &&
            !standard.featuredMusic(), "music should start in baseline groove");
    standard.advance(160, false, true);
    require(standard.pose() == pet::F00, "default music beat too fast");
    pet::Animator faster;
    faster.setAutonomousSpeed(180, 0);
    faster.advance(0, false, true);
    faster.advance(160, false, true);
    require(faster.pose() == pet::NodDown, "music speed slider has no effect");
    faster.advance(180, true, true);
    require(faster.pose() == pet::T00, "typing must interrupt fast music");
    faster.advance(290, true, true);
    require(faster.pose() == pet::T00, "typing speed must ignore the music slider");
    faster.advance(300, true, true);
    require(faster.pose() == pet::T01, "typing frame must retain its 115ms timing");
    standard.setAutonomousSpeed(180, 160);
    require(standard.autonomousSpeed() == 180, "live slider update missing");
    standard.setAutonomousSpeed(999, 160);
    require(standard.autonomousSpeed() == 180, "speed upper clamp missing");
    standard.setAutonomousSpeed(0, 160);
    require(standard.autonomousSpeed() == 50, "speed lower clamp missing");
    pet::Animator fullMusic;
    fullMusic.advance(0, false, true);
    bool enteredFeature = false, returnedToGroove = false;
    for (int ms = 40; ms <= 12500; ms += 40) {
        fullMusic.advance(ms, false, true);
        if (fullMusic.featuredMusic()) enteredFeature = true;
        if (enteredFeature && !fullMusic.featuredMusic()) {
            returnedToGroove = true;
            break;
        }
    }
    require(enteredFeature && returnedToGroove,
            "music must alternate groove and featured performance");

    // Idle breathing persists in the quiet gap and in long stationary
    // expressions, but is suppressed by moving/gift cues or typing/music.
    pet::Animator ambient;
    ambient.advance(0, false, false);
    require(ambient.idleResting() && ambient.idleBreathing(),
            "quiet idle should breathe between actions");
    ambient.advance(9000, false, false);
    require(!ambient.idleResting(), "idle story must start after quiet gap");
    require(ambient.idleBreathing(),
            "idle should still breathe during a long stationary story frame");
    ambient.advance(9040, true, false);
    require(!ambient.idleBreathing(), "typing must suppress breathing");
    ambient.advance(9080, false, true);
    require(!ambient.idleBreathing(), "music must suppress idle breathing");

    // ShuffleBag is not independent random drawing; every cycle covers all.
    for (const int count : {pet::kIdleActionCount, pet::kMusicActionCount,
                             pet::kGiftKindCount}) {
        pet::ShuffleBag bag(count);
        int lastAction = -1;
        for (int cycle = 0; cycle < 25; ++cycle) {
            std::vector<bool> seen(count, false);
            for (int i = 0; i < count; ++i) {
                const int id = bag.next();
                require(id >= 0 && id < count, "invalid shuffled action");
                require(!seen[id], "duplicate action within cycle");
                require(id != lastAction,
                        "adjacent duplicate, including at shuffle cycle boundary");
                lastAction = id;
                seen[id] = true;
            }
            for (const bool selected : seen)
                require(selected, "action skipped inside shuffled cycle");
        }
    }

    // The REAL animator must consume all 11 unique featured actions before
    // any repeats, not merely pass the ShuffleBag unit test in isolation.
    pet::Animator fullQueue;
    std::vector<bool> performed(pet::kMusicActionCount, false);
    int completedSelections = 0;
    bool previousFeature = false;
    for (int t = 0; t < 150000 && completedSelections < pet::kMusicActionCount; t += 40) {
        fullQueue.advance(t, false, true);
        const bool feature = fullQueue.featuredMusic();
        if (feature && !previousFeature) {
            const int id = fullQueue.currentFeaturedAction();
            require(id >= 0 && id < pet::kMusicActionCount,
                    "music selected invalid action");
            require(!performed[id], "real music scheduler repeated action before bag was exhausted");
            performed[id] = true;
            ++completedSelections;
        }
        previousFeature = feature;
    }
    require(completedSelections == pet::kMusicActionCount,
            "real music scheduler did not complete the shuffled set");
    for (bool selected : performed)
        require(selected, "music scheduler skipped a featured dance");

    // Typing interrupts and later resumes the SAME unfinished performance,
    // without silently removing another item from the unique-action bag.
    pet::Animator interrupted;
    int at = 0;
    for (; at < 4500; at += 40) {
        interrupted.advance(at, false, true);
        if (interrupted.featuredMusic()) break;
    }
    require(interrupted.featuredMusic(), "could not reach featured music");
    const int interruptedAction = interrupted.currentFeaturedAction();
    interrupted.advance(at + 40, true, true);
    require(interrupted.activity() == pet::Activity::Typing,
            "typing must interrupt the featured dance");
    interrupted.advance(at + 80, false, true);
    int resumedAction = -1;
    for (int t = at + 120; t < at + 5500; t += 40) {
        interrupted.advance(t, false, true);
        if (interrupted.featuredMusic()) {
            resumedAction = interrupted.currentFeaturedAction();
            break;
        }
    }
    require(resumedAction == interruptedAction,
            "interrupted featured action must be replayed before advancing bag");

    // The UI should be able to preview every registered animation WITHOUT
    // modifying the random shuffle queue or needing external MPRIS playback.
    for (int action = 0; action < pet::kIdleActionCount; ++action) {
        pet::Animator one;
        one.previewIdle(static_cast<pet::IdleAction>(action), 0);
        require(one.previewing(pet::Activity::Idle),
                "idle action preview must start immediately");
        const auto story = pet::idleStory(static_cast<pet::IdleAction>(action));
        const int finishAt = pet::duration(story);
        for (int t = 40; t <= finishAt + 1600; t += 40)
            one.advance(t, false, false);
        require(!one.previewing(pet::Activity::Idle) && one.pose() == pet::F00,
                "idle preview should end once and return to resting pose");
    }
    for (int action = 0; action < pet::kMusicActionCount; ++action) {
        pet::Animator one;
        one.previewMusic(static_cast<pet::MusicAction>(action), 0);
        require(one.previewing(pet::Activity::Music),
                "music preview must be available without playing audio");
        const int finishAt = pet::duration(
            pet::musicStory(static_cast<pet::MusicAction>(action)));
        for (int t = 40; t <= finishAt + 1100; t += 40)
            one.advance(t, false, false);
        require(!one.previewing(pet::Activity::Music) && one.pose() == pet::F00,
                "music preview must stop after one featured dance");
    }
    pet::Animator groovePreview;
    groovePreview.previewGroove(0);
    for (int t = 40; t <= 2500; t += 40)
        groovePreview.advance(t, false, false);
    require(!groovePreview.previewing(pet::Activity::Music),
            "groove preview should not loop forever");

    pet::Animator typingPreview;
    typingPreview.previewTyping(0);
    require(typingPreview.previewing(pet::Activity::Typing),
            "typing preview should run without real key presses");
    for (int t = 40; t <= 2500; t += 40)
        typingPreview.advance(t, false, false);
    require(!typingPreview.previewing(pet::Activity::Typing),
            "typing preview must stop, not repeat endlessly");

    pet::Animator breathingPreview;
    breathingPreview.previewIdleBreathing(0);
    require(breathingPreview.idleBreathing(),
            "quiet breathing should be explicitly previewable");
    breathingPreview.stopPreview(100);
    require(!breathingPreview.previewing(pet::Activity::Idle) &&
            breathingPreview.pose() == pet::F00,
            "Stop Preview must instantly restore resting pose");

    // Flying gifts keep a readable expression for most of their duration,
    // then change once without restarting the flight or fading its trail.
    const pet::GiftVisual earlier{pet::GiftCue::FlyArc,
                                  pet::GiftKind::ArcFly, .4};
    const pet::GiftVisual later{pet::GiftCue::FlyArc,
                                pet::GiftKind::ArcFly, .8};
    require(pet::visiblePose(pet::F02, earlier) == pet::F02 &&
            pet::visiblePose(pet::F02, later) == pet::F07,
            "flying gift should have two held emotional states");
    const QPointF pStart = pet::giftFlight(0.0, false);
    const QPointF pMid = pet::giftFlight(.5, false);
    const QPointF pEnd = pet::giftFlight(1.0, false);
    require(pStart.x() < pMid.x() && pMid.x() < pEnd.x() &&
            pMid.x() < 470 && pEnd.x() > 480,
            "gift must be visible on-screen for most of its slow flight");
    require(pStart.x() < 90 && pEnd.x() > 480,
            "stars must launch from the left paw and cross the entire canvas");
    const QPointF waveHigh = pet::giftFlight(.125, false);
    const QPointF waveLow = pet::giftFlight(.375, false);
    require(waveHigh.y() < waveLow.y(),
            "StarFly must undulate instead of drawing a straight diagonal");
    const QPointF arcMid = pet::giftFlight(.5, true);
    require(arcMid.y() < pMid.y() - 50,
            "ArcFly must make a clearly visible high arc");

    // Manual preview runs even when MPRIS is playing, so all five rare
    // gift endings can be inspected without waiting for the idle shuffle.
    // It must not turn into a permanent override after the story finishes.
    pet::Animator giftDemo;
    giftDemo.previewGift(pet::GiftKind::BurstSad, 0);
    bool demoGone = false, demoReturn = false, demoBurst = false;
    bool demoLeftTear = false, demoRightTear = false;
    bool musicRestored = false;
    for (int t = 0; t <= 34000; t += 40) {
        giftDemo.advance(t, false, true);
        if (giftDemo.pose() == pet::AwayGone) demoGone = true;
        if (giftDemo.pose() == pet::ReturnPeek) demoReturn = true;
        const auto visual = giftDemo.giftVisual(t);
        demoBurst |= visual.cue == pet::GiftCue::Burst &&
                     visual.progress > .2 && visual.progress < .8;
        demoLeftTear |= visual.cue == pet::GiftCue::TearLeft;
        demoRightTear |= visual.cue == pet::GiftCue::TearRight;
        if (giftDemo.activity() == pet::Activity::Music) musicRestored = true;
    }
    require(demoGone && demoReturn && demoBurst &&
            demoLeftTear && demoRightTear && musicRestored,
            "preview must show complete story, timed explosion/tears and restore music");

    pet::Animator typingDuringPreview;
    typingDuringPreview.previewGift(pet::GiftKind::StarFly, 0);
    typingDuringPreview.advance(40, true, true);
    require(typingDuringPreview.activity() == pet::Activity::Typing &&
            !typingDuringPreview.giftVisual(40).active(),
            "typing must cancel gift preview without leaving particles");

    // Music is a dense vertical stream within Matrix's former right-hand
    // strip. Notes are vector strokes and vanish toward the clock above.
    for (int phase : {0, 4, 8, 15, 27, 41}) {
        QImage image(480, 320, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        {
            QPainter painter(&image);
            pet::paintMusicStream(painter, QColor("#88FF88"), phase);
        }
        int pixels = 0;
        int columns[3] = {0, 0, 0};
        for (int y = 0; y < 215; ++y)
            for (int x = 376; x < 475; ++x)
                require(image.pixelColor(x, y).alpha() == 0,
                        "music must be clipped below the date, not run to the clock");
        for (int y = 215; y < 310; ++y)
            for (int x = 378; x < 465; ++x)
                if (image.pixelColor(x, y).alpha() > 0) {
                    ++pixels;
                    if (x < 402) ++columns[0];
                    else if (x < 426) ++columns[1];
                    else ++columns[2];
                }
        require(pixels > 200 && columns[0] > 15 &&
                columns[1] > 15 && columns[2] > 15,
                "dense musical stream must populate three staggered columns");
        for (int y = 55; y < 310; ++y)
            for (int x = 0; x < 374; ++x)
                require(image.pixelColor(x, y).alpha() == 0,
                        "music must not draw notes above pet's head");
    }
    QImage silent(480, 320, QImage::Format_ARGB32_Premultiplied);
    silent.fill(Qt::transparent);
    {
        QPainter painter(&silent);
        pet::paintMusicStream(painter, QColor("#88FF88"), -1);
    }
    for (int y = 55; y < 310; ++y)
        for (int x = 378; x < 465; ++x)
            require(silent.pixelColor(x, y).alpha() == 0,
                    "music stream must disappear while typing/idle");

    // Burst visuals must be much more than a single star, and the physical
    // spark/trail layer must disappear outside gift cues.
    for (pet::GiftKind kind : {pet::GiftKind::BurstSad,
                               pet::GiftKind::BurstCry}) {
        QImage blast(480, 320, QImage::Format_ARGB32_Premultiplied);
        blast.fill(Qt::transparent);
        {
            QPainter painter(&blast);
            pet::paintGiftEffect(painter, {pet::GiftCue::Burst, kind, .42},
                                 QColor("#88FF88"));
        }
        int count = 0;
        for (int y = 120; y < 319; ++y)
            for (int x = 285; x < 480; ++x)
                if (blast.pixelColor(x, y).alpha() > 0) ++count;
        require(count > 200, "burst must have a dense expanding ring and shards");
    }
    for (auto cue : {pet::GiftCue::Carry, pet::GiftCue::Transfer,
                     pet::GiftCue::FlyStraight, pet::GiftCue::FlyArc,
                     pet::GiftCue::TearLeft, pet::GiftCue::TearRight,
                     pet::GiftCue::Pulse, pet::GiftCue::Charge}) {
        QImage effect(480, 320, QImage::Format_ARGB32_Premultiplied);
        effect.fill(Qt::transparent);
        {
            QPainter painter(&effect);
            pet::paintGiftEffect(painter,
                {cue, pet::GiftKind::BurstCry, .38}, QColor("#88FF88"));
        }
        int count = 0;
        for (int y = 80; y < 320; ++y)
            for (int x = 130; x < 480; ++x)
                if (effect.pixelColor(x, y).alpha() > 0) ++count;
        require(count > 4, "gift effects must render at intermediate progress");
    }
    // Restore the original single-drop drawing (shape/color/size/curve)
    // while emitting one alternating-eye drop every 675ms.
    auto cryingFrame = [](double elapsedMs) {
        QImage image(480, 320, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        pet::paintGiftEffect(painter,
            {pet::GiftCue::TearLeft, pet::GiftKind::BurstCry, 0.0,
             0.0, elapsedMs}, QColor("#88FF88"));
        painter.end();
        return image;
    };
    // At the first instant exactly one original droplet is rendered.
    const double firstProgress = 200.0/1350.0;
    QImage originalDrop(480, 320, QImage::Format_ARGB32_Premultiplied);
    originalDrop.fill(Qt::transparent);
    {
        QPainter painter(&originalDrop);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const double alpha = firstProgress < .12 ? firstProgress/.12 :
                             firstProgress > .85 ? (1.0-firstProgress)/.15 : 1.0;
        pet::giftDrop(painter, 142.0, 251.0+33.0*firstProgress*firstProgress,
                      6.0, pet::giftTint(QColor("#b5faff"), alpha));
    }
    require(cryingFrame(0.0) == originalDrop,
            "first tear must preserve the original color, size and curve");

    // The original slow fall lasts 1350ms, but new drops appear
    // twice as often and keep alternating left/right.
    for (int ms = 0; ms <= 13500; ms += 75) {
        const QImage frame = cryingFrame(double(ms));
        int pixels = 0;
        for (const int x0 : {129, 224})
            for (int x = x0; x < x0+29; ++x)
                for (int y = 244; y <= 292; ++y)
                    pixels += frame.pixelColor(x,y).alpha() > 10;
        require(pixels > 4, "crying must not pause between falling drops");
    }
    for (int ms : {0, 675, 1350, 2025}) {
        const QImage frame = cryingFrame(double(ms));
        const int leadingX = (ms/675)%2 == 0 ? 142 : 237;
        require(frame.pixelColor(leadingX, 252).alpha() > 10,
                "tear starts from the alternating eye at 675ms intervals");
    }

    // Actual story: tears must begin when dot-eyes appear, continue across
    // the old TearLeft/TearRight boundaries and persist through the last
    // previously-empty sad hold. Repaint must remain active the entire time.
    for (pet::GiftKind kind : {pet::GiftKind::BurstSad,
                               pet::GiftKind::BurstCry}) {
        pet::Animator cryingPreview;
        cryingPreview.previewGift(kind, 0);
        bool enteredSad = false, exitedSad = false;
        qint64 lastElapsed = -1;
        int cryingTicks = 0, initialHoldTicks = 0, finalHoldTicks = 0;
        for (qint64 ms = 0; ms <= 43000; ms += 80) {
            const bool dirty = cryingPreview.advance(ms, false, false);
            const int pose = cryingPreview.pose();
            const bool sad = pose == pet::SadComic || pose == pet::SadCry;
            const auto visual = cryingPreview.giftVisual(ms);
            if (sad) {
                enteredSad = true;
                ++cryingTicks;
                require(dirty && visual.active() &&
                        visual.tearElapsedMs >= lastElapsed,
                        "continuous tears must repaint without resetting the timer");
                const QImage image = cryingFrame(visual.tearElapsedMs);
                int count = 0;
                for (int x = 129; x <= 252; ++x)
                    for (int y = 244; y <= 292; ++y)
                        count += image.pixelColor(x,y).alpha() > 10;
                require(count > 4, "tears must remain visible throughout dot-eyes");
                if (visual.tearElapsedMs < 1950.0) ++initialHoldTicks;
                if (visual.tearElapsedMs > 5500.0) ++finalHoldTicks;
                lastElapsed = static_cast<qint64>(visual.tearElapsedMs);
            } else if (enteredSad) {
                exitedSad = true;
                require(!visual.active() ||
                        (visual.cue != pet::GiftCue::TearLeft &&
                         visual.cue != pet::GiftCue::TearRight),
                        "tears must end when sad eyes disappear");
                break;
            }
        }
        require(enteredSad && exitedSad && cryingTicks >= 70 &&
                initialHoldTicks > 3 && finalHoldTicks > 3,
                "crying must cover complete long sadness, including its holds");
    }

    QImage noGift(480, 320, QImage::Format_ARGB32_Premultiplied);
    noGift.fill(Qt::transparent);
    {
        QPainter painter(&noGift);
        pet::paintGiftEffect(painter, {}, QColor("#88FF88"));
    }
    require(noGift.pixelColor(392, 247).alpha() == 0,
            "gift renderer must do nothing when cue is None");

    // Pose Unicode shaping (10 fixed cells and multiple thick strokes)
    // is cached per appearance and tail state. The active frame only
    // translates/mirrors a supersampled bitmap, without re-shaping text.
    pet::SpriteCache spriteCache;
    const pet::JoyTail noWag;
    spriteCache.configure(QStringLiteral("DejaVu Sans Mono"), original,
                          QColor("#B0FFBB"), 43, 25);
    const auto &firstSprite = spriteCache.sprite(pet::F00, noWag);
    require(firstSprite.width() == pet::SpriteCache::kCanvasWidth &&
            firstSprite.height() == pet::SpriteCache::kSpriteHeight,
            "cached sprite must stay at original pet resolution");
    const auto spriteBuilds = spriteCache.buildCount();
    require(spriteBuilds == 1 && spriteCache.cachedPoses() == 1,
            "first sprite should be rendered only once");
    const auto &reuse = spriteCache.sprite(pet::F00, noWag);
    require(&firstSprite == &reuse && spriteCache.buildCount() == spriteBuilds,
            "same pose and tail state must reuse the cached raster");
    const auto &otherSprite = spriteCache.sprite(pet::F02, noWag);
    require(!otherSprite.isNull(), "second sprite must rasterize");
    require(spriteCache.buildCount() == spriteBuilds + 1,
            "different eye pose should have its own reusable raster");
    spriteCache.configure(QStringLiteral("DejaVu Sans Mono"), original,
                          QColor("#B0FFBB"), 43, 25);
    require(spriteCache.cachedPoses() == 2,
            "unchanged user appearance should not invalidate sprites");
    spriteCache.configure(QStringLiteral("DejaVu Sans Mono"), original,
                          QColor("#B0FFBB"), 43, 65);
    require(spriteCache.cachedPoses() == 0,
            "changing font stroke must invalidate cached glyphs");

    // Matrix input has no key values and produces two independently rising
    // columns without changing the pet renderer or changing fonts.
    MatrixRain matrix;
    require(!matrix.active(), "rain should start empty");
    for (int i = 0; i < 8; ++i) matrix.addPress(0);
    require(matrix.glyphCount() == 8, "one glyph per key press");
    matrix.advance(40, true);
    require(matrix.image().width() == MatrixRain::CanvasWidth,
            "Matrix canvas dimensions changed");
    const auto redRain = matrix.image(QColor("#FF0000"), 90);
    const auto polishedRain = crtComposite(redRain, Qt::transparent, 0,
                                           44, 2, false, QColor("#808080"));
    require(polishedRain.width() == MatrixRain::CanvasWidth,
            "processed Matrix canvas must match original CRT canvas");
    bool hasRedPixels = false;
    for (int y = 0; y < redRain.height(); ++y) {
        for (int x = 0; x < redRain.width(); ++x) {
            const QColor pixel = redRain.pixelColor(x, y);
            if (pixel.alpha() > 0 && pixel.red() > pixel.green()) {
                hasRedPixels = true;
                break;
            }
        }
        if (hasRedPixels) break;
    }
    require(hasRedPixels, "Matrix must use selected user phosphor color");
    const auto compactRain = matrix.stripImage(QColor("#FF0000"), 90);
    require(compactRain.width() == 108 &&
            compactRain.height() == MatrixRain::CanvasHeight,
            "live Matrix source must only rasterize its 108px strip");
    for (int y = 0; y < MatrixRain::CanvasHeight; y += 4)
        for (int x = MatrixRain::StripX; x < MatrixRain::CanvasWidth; x += 4)
            require(compactRain.pixelColor(x - MatrixRain::StripX, y) ==
                    redRain.pixelColor(x, y),
                    "cropped Matrix glyphs must match original full-canvas rendering");
    for (int t = 80; t <= 7000; t += 40)
        matrix.advance(t, false);
    require(!matrix.active(), "Matrix glyphs should fade and vanish");

    // Priority: typing > music > idle, even when an idle story is active.
    pet::Animator animator;
    animator.advance(1000, true, true);
    require(animator.activity() == pet::Activity::Typing, "typing priority");
    require(animator.pose() == pet::T00, "typing starts immediately");
    animator.advance(1400, false, true);
    require(animator.activity() == pet::Activity::Music, "music resumes after typing");
    animator.advance(1900, true, true);
    require(animator.activity() == pet::Activity::Typing, "typing interrupts music");
    animator.advance(2000, false, false);
    require(animator.activity() == pet::Activity::Idle, "idle restored");
    require(animator.pose() == pet::F00, "neutral idle pose");

    // Flicker frequency is disabled at zero and reaches an interval of
    // four seconds at maximum. Old disabled checkbox migrates to frequency 0.
    require(crt::flickerIntervalMs(0) == 0, "zero must turn flicker off");
    require(crt::flickerIntervalMs(100) == 4000, "fastest flicker must be 4 seconds");
    require(crt::flickerIntervalMs(25) == 71500, "legacy-like default interval");
    require(crt::flickerIntervalMs(1) > crt::flickerIntervalMs(25),
            "flicker frequency direction incorrect");
    require(crt::flickerIntervalMs(50) > crt::flickerIntervalMs(100),
            "flicker intervals must decrease as frequency grows");
    require(crt::flickerIntervalMs(101) == 4000, "frequency upper bound");

    // Session autostart is opt-in, per-user and reversible. This test uses an
    // isolated XDG_CONFIG_HOME and must never touch a real user's autostart.
    QTemporaryDir temporaryConfig;
    require(temporaryConfig.isValid(), "failed to create isolated autostart config");
    const QByteArray previousXdg = qgetenv("XDG_CONFIG_HOME");
    qputenv("XDG_CONFIG_HOME", temporaryConfig.path().toUtf8());
    require(desktop::autostartPath().startsWith(temporaryConfig.path()),
            "autostart must follow XDG_CONFIG_HOME");
    require(!desktop::autostartEnabled(), "autostart must be opt-in");
    require(desktop::setAutostart(true), "failed to enable XDG autostart");
    require(desktop::autostartEnabled(), "autostart entry not created");
    QFile entry(desktop::autostartPath());
    require(entry.open(QIODevice::ReadOnly), "cannot inspect autostart entry");
    const QByteArray ini = entry.readAll();
    require(ini.contains("[Desktop Entry]") && ini.contains("Exec=crt-clock"),
            "autostart entry must launch the widget");
    entry.close();
    require(desktop::setAutostart(false), "failed to disable XDG autostart");
    require(!desktop::autostartEnabled(), "autostart entry remains after disable");
    if (previousXdg.isEmpty()) qunsetenv("XDG_CONFIG_HOME");
    else qputenv("XDG_CONFIG_HOME", previousXdg);

    // XFCE workspace visibility depends on _NET_WM_DESKTOP, not merely
    // _NET_WM_STATE_STICKY, which may remain set on a single workspace.
    require(desktop::isAllDesktopsValue(0xFFFFFFFFUL),
            "all-workspaces desktop value must be recognized");
    require(desktop::isAllDesktopsValue(~0UL),
            "Xlib sign-extended all-desktops value must be recognized");
    require(!desktop::isAllDesktopsValue(0UL),
            "first workspace must not pass all-workspaces check");
    require(!desktop::isAllDesktopsValue(1UL),
            "second workspace must not pass all-workspaces check");
    // Calibration must NEVER call QWidget::setWindowFlags after first map:
    // the single initial flag set contains no mode-specific hints.
    const auto flags = desktop::widgetWindowFlags();
    require((flags & Qt::WindowType_Mask) == Qt::Window &&
            flags.testFlag(Qt::FramelessWindowHint),
            "widget needs a fixed normal frameless window type");
    require(!flags.testFlag(Qt::WindowStaysOnTopHint) &&
            !flags.testFlag(Qt::WindowStaysOnBottomHint) &&
            !flags.testFlag(Qt::WindowDoesNotAcceptFocus),
            "initial flags must not require remapping for calibration");


    // Old four-corner configs still get exactly four derived edge midpoints.
    const std::array<QPointF,4> old{
        QPointF(0, 0), QPointF(300, 0),
        QPointF(300, 200), QPointF(0, 200)
    };
    auto handles = perspective::fromCorners(old);
    close(handles[4].x(), 150);
    close(handles[4].y(), 0);
    close(handles[5].x(), 300);
    close(handles[5].y(), 100);
    close(handles[6].x(), 150);
    close(handles[7].y(), 100);
    const QPointF center = perspective::center(handles);
    close(center.x(), 150);
    close(center.y(), 100);

    // A previously expensive four-cell perspective mapping is computed
    // once. Each moving frame is bilinearly projected using the cached LUT.
    // The second typing/Matrix source is composited over the pet, not warped
    // in a second four-quad QPainter call.
    perspective::RasterCache raster;
    const auto rectangle = perspective::fromCorners({
        QPointF(24, 18), QPointF(504, 18),
        QPointF(504, 338), QPointF(24, 338)
    });
    const QSize viewport(545, 365);
    require(raster.prepare(viewport, rectangle, QPointF(0,0)),
            "first projection must build geometry");
    require(!raster.prepare(viewport, rectangle, QPointF(0,0)) &&
            raster.buildCount() == 1,
            "unchanged projection must reuse its inverse geometry");
    require(raster.mappedPixels() >= 150000,
            "straight four-cell projective mapping must cover source canvas");

    QImage baseLayer(480, 320, QImage::Format_ARGB32_Premultiplied);
    baseLayer.fill(Qt::transparent);
    {
        QPainter p(&baseLayer);
        p.fillRect(QRect(40, 70, 300, 140), QColor(25, 115, 43, 255));
    }
    QImage matrixLayer(480, 320, QImage::Format_ARGB32_Premultiplied);
    matrixLayer.fill(Qt::transparent);
    {
        QPainter p(&matrixLayer);
        p.fillRect(QRect(410, 245, 22, 32), QColor(250, 90, 10, 220));
    }
    const QImage baseline = raster.render(baseLayer);
    const QImage combined = raster.render(baseLayer, matrixLayer);
    require(baseline.size() == viewport &&
            baseline.format() == QImage::Format_ARGB32_Premultiplied,
            "raster should return an in-memory screen-sized premultiplied image");
    require(baseline.pixelColor(120, 110) == baseLayer.pixelColor(96, 92),
            "unwarped baseline must preserve a solid source pixel");
    require(combined.pixelColor(120, 110) == baseline.pixelColor(120, 110),
            "an unrelated matrix glyph cannot alter the rest of the clock");
    require(combined.pixelColor(444, 274).red() > 130,
            "Matrix must overlay clock glyphs after perspective");

    QImage reference(viewport, QImage::Format_ARGB32_Premultiplied);
    reference.fill(Qt::transparent);
    {
        QPainter p(&reference);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        perspective::draw(p, baseLayer, rectangle, QPointF(0, 0));
        perspective::draw(p, matrixLayer, rectangle, QPointF(0, 0));
    }
    // Compare stable interior colors; raster may differ within ~1px of
    // heavily antialiased clip boundaries, which are calibrated by Qt.
    for (const QPoint pt : {QPoint(120,110), QPoint(200,140),
                             QPoint(240,180), QPoint(445,275)}) {
        const QColor actual = combined.pixelColor(pt);
        const QColor expected = reference.pixelColor(pt);
        require(std::abs(actual.red()-expected.red()) < 35 &&
                std::abs(actual.green()-expected.green()) < 35 &&
                std::abs(actual.blue()-expected.blue()) < 35 &&
                std::abs(actual.alpha()-expected.alpha()) < 35,
                "cached screen raster should match original Qt perspective colors");
    }

    auto skew = rectangle;
    skew[0] += QPointF(5,12);
    skew[4] += QPointF(11,4);
    skew[7] += QPointF(-9,10);
    require(raster.prepare(viewport, skew, QPointF(0,0)) &&
            raster.buildCount() == 2,
            "perspective adjustment must invalidate the cached pixel map");
    const QImage bent = raster.render(baseLayer, matrixLayer);
    require(!bent.isNull() && bent != combined,
            "different eight-handle warp must produce a different raster");
    require(!raster.prepare(viewport, skew, QPointF(0,0)),
            "constant geometry must remain cached across animation frames");

    // Incremental projection must be pixel-for-pixel identical to the full
    // renderer. In music/typing only a small corner changes from frame to
    // frame; changing the geometry or switching Matrix must still invalidate.
    perspective::RasterCache incremental;
    require(incremental.prepare(viewport, skew, QPointF(0,0)),
            "incremental first map");
    QImage projected;
    incremental.renderInto(projected, baseLayer, matrixLayer);
    require(projected == incremental.render(baseLayer, matrixLayer),
            "incremental cold frame must match full perspective");
    require(incremental.lastUpdatedPixels() == incremental.mappedPixels(),
            "first incremental frame must map every pixel");
    QImage changedBase = baseLayer.copy();
    {
        QPainter painter(&changedBase);
        painter.fillRect(QRect(426, 260, 16, 28), QColor(45, 205, 95, 210));
    }
    incremental.renderInto(projected, changedBase, matrixLayer);
    require(projected == incremental.render(changedBase, matrixLayer),
            "small musical change must project identically");
    require(incremental.lastUpdatedPixels() < incremental.mappedPixels()/8,
            "small musical region must avoid most projective samples");
    incremental.renderInto(projected, changedBase, matrixLayer);
    require(incremental.lastUpdatedPixels() == 0,
            "same frame must skip all projective samples");
    QImage changedMatrix = matrixLayer.copy();
    {
        QPainter painter(&changedMatrix);
        painter.fillRect(QRect(400, 232, 18, 26), QColor(65, 140, 255, 180));
    }
    incremental.renderInto(projected, changedBase, changedMatrix);
    require(projected == incremental.render(changedBase, changedMatrix),
            "Matrix movement must preserve source-over blend");
    incremental.renderInto(projected, changedBase);
    require(projected == incremental.render(changedBase),
            "stopping Matrix must clear its former projection");
    incremental.prepare(viewport, rectangle, QPointF(0,0));
    incremental.renderInto(projected, changedBase);
    require(projected == incremental.render(changedBase),
            "changed eight-point calibration must redraw whole output");
    require(incremental.lastUpdatedPixels() == incremental.mappedPixels(),
            "geometry change must invalidate incremental image");

    // Curved 8-handle calibration must preserve smooth colors across all
    // four joins. Compare the same full alpha gradient with Qt's original
    // projective path, excluding only partly covered outer-edge pixels.
    QImage gradient(480, 320, QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < gradient.height(); ++y) {
        auto *line = reinterpret_cast<QRgb *>(gradient.scanLine(y));
        for (int x = 0; x < gradient.width(); ++x)
            line[x] = qRgb((x*255)/479, (y*255)/319, (x+y)%256);
    }
    const QImage warpedGradient = raster.render(gradient);
    QImage qtGradient(viewport, QImage::Format_ARGB32_Premultiplied);
    qtGradient.fill(Qt::transparent);
    {
        QPainter painter(&qtGradient);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        perspective::draw(painter, gradient, skew, QPointF(0,0));
    }
    qint64 difference = 0;
    int samples = 0;
    for (int y = 25; y < viewport.height()-20; y += 4)
        for (int x = 25; x < viewport.width()-20; x += 4) {
            const QColor expected = qtGradient.pixelColor(x,y);
            const QColor actual = warpedGradient.pixelColor(x,y);
            if (expected.alpha() < 253 || actual.alpha() < 253)
                continue;
            difference += std::abs(expected.red() - actual.red());
            difference += std::abs(expected.green() - actual.green());
            ++samples;
        }
    require(samples > 7000 && double(difference) / samples < 35.0,
            "cached raster must preserve smooth colors across a bent 8-handle geometry");

    // Moving the central handle translates every corner/edge equally.
    const QPointF offset(13, -9);
    for (auto &handle : handles) handle += offset;
    for (int n=0; n<8; ++n) {
        close(handles[n].x(), perspective::fromCorners(old)[n].x()+13);
        close(handles[n].y(), perspective::fromCorners(old)[n].y()-9);
    }
    std::cout << "CRT core tests passed\n";
}
