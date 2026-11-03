#include <QPointF>
#include <QString>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include "pet_animation.hpp"
#include "pet_poses.hpp"
#include "perspective.hpp"

static void close(double a, double b) {
    assert(std::abs(a-b) < .001);
}

int main() {
    const QString original = QStringLiteral("(づ｡◕‿‿◕｡)づ");
    assert(pet::glyph(0, original) == original);
    assert(pet::glyph(9, original).contains(QStringLiteral("●")));
    assert(!pet::glyph(13, original).isEmpty());

    // Explicit priority contract: typing > music > idle.
    pet::Animator animator;
    animator.advance(1000, true, true);
    assert(animator.activity() == pet::Activity::Typing);
    assert(animator.pose() == 13);
    animator.advance(1400, false, true);
    assert(animator.activity() == pet::Activity::Music);
    assert(animator.pose() == 11);
    animator.advance(1900, true, true);
    assert(animator.activity() == pet::Activity::Typing);
    animator.advance(2000, false, false);
    assert(animator.activity() == pet::Activity::Idle);
    assert(animator.pose() == 0);

    // v0.2 four-corner settings map to the new eight-handle rectangle.
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

    std::cout << "core geometry and pet priority checks passed\n";
}
