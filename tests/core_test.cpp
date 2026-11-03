#include <QPointF>
#include <QString>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include "pet_animation.hpp"
#include "pet_poses.hpp"
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
    require(pet::glyph(0, original) == original, "canonical pose changed");
    require(pet::glyph(9, original).contains(QStringLiteral("●")), "tail missing");
    require(pet::parts(13).typing && pet::parts(15).typing, "typing indicator missing");

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
