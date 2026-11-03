#pragma once

#include "pet_poses.hpp"
#include <QtGlobal>

// Tail wags whenever the face uses the joyful > < eyes, independent of which
// idle/music script happens to show that face. Wait two GUI animation ticks,
// then animate continuously until the eyes return to another expression.
namespace pet {

inline bool hasJoyfulEyes(const Pose &pose) {
    return !pose.back && pose.line.size() == kFrontColumns &&
           pose.line.at(3) == QChar('>') && pose.line.at(6) == QChar('<');
}

class JoyTail {
public:
    // Must be called each ~40ms animation tick, even without pose changes.
    bool advance(qint64 nowMs, bool joyful) {
        const bool beforeVisible = visible_;
        const int beforeSide = side_;
        if (!joyful) {
            framesSeen_ = 0;
            visible_ = false;
            side_ = -1;
            nextWagMs_ = 0;
        } else {
            if (framesSeen_ < 3) ++framesSeen_;
            if (framesSeen_ == 3 && !visible_) {
                visible_ = true;
                nextWagMs_ = nowMs + 160;
            }
            if (visible_ && nowMs >= nextWagMs_) {
                side_ = -side_;
                nextWagMs_ = nowMs + 160;
            }
        }
        return beforeVisible != visible_ || (visible_ && beforeSide != side_);
    }

    [[nodiscard]] bool visible() const { return visible_; }
    [[nodiscard]] int side() const { return side_; }
private:
    int framesSeen_ = 0;
    bool visible_ = false;
    int side_ = -1;
    qint64 nextWagMs_ = 0;
};
} // namespace pet
