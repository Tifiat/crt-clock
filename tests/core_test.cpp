#include <QPointF>
#include <QString>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "pet_animation.hpp"
#include "pet_poses.hpp"
#include "pet_render.hpp"
#include "matrix_rain.hpp"
#include "perspective.hpp"

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
    require(pet::kFontPixels == 41, "fixed pet font changed");

    // Compare every approved pose, including historical backside tail frames.
    for (int id = 0; id < pet::kPoseCount; ++id) {
        const pet::Pose p = pet::frame(id);
        require(p.line.size() == (p.back ? pet::kBackColumns : pet::kFrontColumns),
                "unexpected pose column count");
        require(p.headDx >= -3 && p.headDx <= 3, "head moves too far");
        require(p.headDy >= -3 && p.headDy <= 3, "head moves too far");
        require(p.bodyDx >= -3 && p.bodyDx <= 3, "body moves too far");
        require(p.bodyDy >= -3 && p.bodyDy <= 3, "body moves too far");
        if (p.back) {
            require(id >= pet::B00 && id <= pet::B02, "unexpected backside pose");
            require(p.line.at(1) == QChar('(') &&
                    p.line.at(9) == QChar(')'), "backside body geometry changed");
            require(p.line.contains(QStringLiteral("●")), "backside tail missing");
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
    for (int n = 0; n < pet::kMusicActionCount; ++n) {
        const auto story = pet::musicStory(static_cast<pet::MusicAction>(n));
        require(pet::duration(story) >= 3000, "music action too short");
        require(story.size() >= 4, "music action is too short to read");
        for (const auto &step : story)
            require(step.pose >= 0 && step.pose < pet::kPoseCount &&
                    step.durationMs > 0, "invalid music frame");
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

    // ShuffleBag is not independent random drawing; every cycle covers all.
    for (const int count : {pet::kIdleActionCount, pet::kMusicActionCount}) {
        pet::ShuffleBag bag(count);
        for (int cycle = 0; cycle < 25; ++cycle) {
            std::vector<bool> seen(count, false);
            for (int i = 0; i < count; ++i) {
                const int id = bag.next();
                require(id >= 0 && id < count, "invalid shuffled action");
                require(!seen[id], "duplicate action within cycle");
                seen[id] = true;
            }
            for (const bool selected : seen)
                require(selected, "action skipped inside shuffled cycle");
        }
    }

    // Matrix input has no key values and produces two independently rising
    // columns without changing the pet renderer or changing fonts.
    MatrixRain matrix;
    require(!matrix.active(), "rain should start empty");
    for (int i = 0; i < 8; ++i) matrix.addPress(0);
    require(matrix.glyphCount() == 8, "one glyph per key press");
    matrix.advance(40, true);
    require(matrix.image().width() == MatrixRain::CanvasWidth,
            "Matrix canvas dimensions changed");
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
