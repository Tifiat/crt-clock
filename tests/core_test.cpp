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
    // Deterministic layout contract: every single frame is exactly ten cells.
    const QString original = QStringLiteral("(づ｡◕‿‿◕｡)づ");
    require(pet::frame(0).line == original, "canonical pose changed");
    require(pet::kColumns == 10 && pet::kCellPixels == 40 && pet::kFontPixels == 38,
            "fixed layout constants changed");
    for (int id = 0; id <= 25; ++id) {
        const pet::Pose frame = pet::frame(id);
        require(frame.line.size() == pet::kColumns, "variable length pose");
        require(frame.line.at(0) == QChar('('), "left anchor changed");
        require(frame.line.at(1) != QChar(' '), "left arm missing");
        require(frame.line.at(2) == QChar(0xFF61), "left face anchor changed");
        require(frame.line.at(7) == QChar(0xFF61), "right face anchor changed");
        require(frame.line.at(8) == QChar(')'), "right anchor changed");
        require(frame.headDx >= -3 && frame.headDx <= 3, "horizontal head movement too large");
        require(frame.headDy >= -3 && frame.headDy <= 3, "vertical head movement too large");
        require(frame.bodyDy >= -3 && frame.bodyDy <= 3, "vertical body movement too large");
    }
    require(pet::frame(9).tailOffset != pet::frame(10).tailOffset, "tail does not wag");
    require(pet::frame(13).typing && pet::frame(14).typing &&
            pet::frame(15).typing, "typing indicator missing");

    // One shuffled cycle must contain each action once, in random order.
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
                require(selected, "some action never selected in cycle");
        }
    }

    // Every non-typing story is a complete action lasting several seconds.
    for (int n=0; n<7; ++n)
        require(pet::duration(pet::idleStory(static_cast<pet::IdleAction>(n))) >= 2400,
                "idle action too short");
    for (int n=0; n<5; ++n)
        require(pet::duration(pet::musicStory(static_cast<pet::MusicAction>(n))) >= 3000,
                "music action too short");
    require(pet::duration(pet::typingStory()) < 600, "typing response too slow");

    // Priority: typing > music > idle, even when an idle story is active.
    pet::Animator animator;
    animator.advance(1000, true, true);
    require(animator.activity() == pet::Activity::Typing, "typing priority");
    require(animator.pose() == 13, "typing starts immediately");
    animator.advance(1400, false, true);
    require(animator.activity() == pet::Activity::Music, "music resumes after typing");
    animator.advance(1900, true, true);
    require(animator.activity() == pet::Activity::Typing, "typing interrupts music");
    animator.advance(2000, false, false);
    require(animator.activity() == pet::Activity::Idle, "idle restored");
    require(animator.pose() == 0, "neutral idle pose");

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
