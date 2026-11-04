#include <QPointF>
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
#include "pet_gift_render.hpp"
#include "pet_tail.hpp"
#include "crt_style.hpp"
#include "matrix_rain.hpp"
#include "perspective.hpp"
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

int main() {
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
        if (id >= pet::ReturnPeek && id <= pet::ReturnApproach)
            require(p.bodyDx >= 70 && p.bodyDx <= 240,
                    "front-facing side entrance must approach from right");
        else
            require(p.bodyDx >= -9 && p.bodyDx <= 9, "body moves too far");
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
                require(p.line.contains(QStringLiteral("◦")), "idle departure has no rear silhouette");
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
    require(pet::duration(celebration) == 4850, "celebration duration changed");
    require(celebration.size() == 5, "celebration should have five clear steps");
    require(celebration[1].pose == pet::F04 &&
            celebration[1].durationMs == 1650, "celebration happy hold missing");

    require(pet::kIdleActionCount == 14, "new idle patrol/fetch actions missing");
    const auto patrol = pet::idleStory(pet::IdleAction::Patrol);
    const auto fetch = pet::idleStory(pet::IdleAction::Fetch);
    require(pet::duration(patrol) >= 4500 && pet::duration(fetch) >= 7000,
            "rare idle stories should be recognizable complete actions");
    bool leftStep = false, rightStep = false, gift = false, farAway = false;
    bool vanished = false, returnsFront = false;
    for (const auto &step : patrol) {
        leftStep |= step.pose == pet::IdleStepLeft;
        rightStep |= step.pose == pet::IdleStepRight;
    }
    for (const auto &step : fetch) {
        gift |= step.cue == pet::GiftCue::Present;
        farAway |= step.pose == pet::AwayTiny;
        vanished |= step.pose == pet::AwayGone && step.durationMs >= 1000;
        returnsFront |= step.pose == pet::ReturnPeek;
    }
    require(leftStep && rightStep && gift && farAway && vanished && returnsFront,
            "idle story must depart, disappear, return facing front and show gift");
    // Every gift has its own complete ending. Explosion branches alternate
    // falling tears from different eyes, then pass through anger/recovery.
    for (int kind = 0; kind < pet::kGiftKindCount; ++kind) {
        const auto variant = static_cast<pet::GiftKind>(kind);
        const auto story = pet::fetchStory(variant);
        int vanishAt = -1, returnAt = -1, presentAt = -1;
        int tears = 0, bursts = 0, softSad = 0, angry = 0;
        pet::GiftCue lastTear = pet::GiftCue::None;
        for (int i = 0; i < story.size(); ++i) {
            const auto &step = story[i];
            if (step.pose == pet::AwayGone) vanishAt = i;
            if (step.pose == pet::ReturnPeek) returnAt = i;
            if (step.cue == pet::GiftCue::Present) presentAt = i;
            if (step.cue == pet::GiftCue::Burst) ++bursts;
            if (step.pose == pet::SadSoft) ++softSad;
            if (step.pose == pet::SadAngry) ++angry;
            if (step.cue == pet::GiftCue::TearLeft ||
                step.cue == pet::GiftCue::TearRight) {
                require(lastTear != step.cue,
                        "tears must alternate between left and right");
                lastTear = step.cue;
                ++tears;
            }
            if (returnAt >= 0 && i >= returnAt)
                require(!pet::frame(step.pose).back,
                        "all post-return poses must face forward");
        }
        require(vanishAt >= 0 && vanishAt < returnAt &&
                returnAt < presentAt && story.back().pose == pet::F00,
                "gift story timeline/neutral exit invalid");
        if (pet::isBurst(variant))
            require(bursts == 1 && tears >= 3 && angry >= 1,
                    "burst must cause visible blast, alternating tears and recovery");
        else if (variant != pet::GiftKind::Pulse)
            require(softSad > 0, "flying gift should get mild sad reaction");
        else require(bursts == 0 && tears == 0,
                     "pulse gift should stay happy");
    }

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
    require(pet::kMusicActionCount == 11, "featured music set incomplete");
    for (const auto action : {pet::MusicAction::LookToBeat,
                              pet::MusicAction::WinkGroove}) {
        const auto story = pet::musicStory(action);
        for (int i = 1; i < story.size(); ++i) {
            if (story[i].pose == pet::F09 || story[i].pose == pet::F10)
                require(story[i-1].pose == story[i].pose ||
                        (i+1 < story.size() &&
                         story[i+1].pose == story[i].pose),
                        "musical winks should be held at least two beats");
        }
    }
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
    for (int ms = 40; ms <= 6500; ms += 40) {
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
    for (int t = 0; t < 85000 && completedSelections < pet::kMusicActionCount; t += 40) {
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
        for (int y = 54; y < 310; ++y)
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
    for (auto cue : {pet::GiftCue::FlyStraight, pet::GiftCue::FlyArc,
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
    QImage noGift(480, 320, QImage::Format_ARGB32_Premultiplied);
    noGift.fill(Qt::transparent);
    {
        QPainter painter(&noGift);
        pet::paintGiftEffect(painter, {}, QColor("#88FF88"));
    }
    require(noGift.pixelColor(392, 247).alpha() == 0,
            "gift renderer must do nothing when cue is None");

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

    // Moving the central handle translates every corner/edge equally.
    const QPointF offset(13, -9);
    for (auto &handle : handles) handle += offset;
    for (int n=0; n<8; ++n) {
        close(handles[n].x(), perspective::fromCorners(old)[n].x()+13);
        close(handles[n].y(), perspective::fromCorners(old)[n].y()-9);
    }
    std::cout << "CRT core tests passed\n";
}
