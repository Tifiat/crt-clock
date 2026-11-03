#pragma once

#include <QString>

// Stable drawing slots: changing expression must never change the pet's bounds.
namespace pet {
struct Parts {
    QString leftArm = QStringLiteral("づ");
    QString rightArm = QStringLiteral("づ");
    QString leftEye = QStringLiteral("◕");
    QString rightEye = QStringLiteral("◕");
    QString mouth = QStringLiteral("‿‿");
    int headDx = 0;
    int headDy = 0;
    int bodyDy = 0;
    int tailSide = 0;
    bool typing = false;
};

inline Parts parts(int pose) {
    Parts p;
    switch (pose) {
        case 1:  p.leftEye = p.rightEye = QStringLiteral("－"); break;
        case 2:  p.rightEye = QStringLiteral("◔"); break;
        case 3:  p.leftEye = QStringLiteral("◔"); break;
        case 4:  p.rightArm = QStringLiteral("っ"); break;
        case 5:  p.leftArm = QStringLiteral("っ"); break;
        case 6:  p.leftArm = QStringLiteral("ヽ"); p.rightArm = QStringLiteral("ﾉ"); break;
        case 7:  p.leftEye = p.rightEye = QStringLiteral("－"); break;
        // The previous full-body backside/tail glyph caused a one-frame jump.
        // A round tail now animates in its own fixed slot.
        case 8:  p.tailSide = -1; break;
        case 9:  p.tailSide = -2; break;
        case 10: p.tailSide = 2; break;
        case 11: p.leftArm = QStringLiteral("ヽ"); p.rightArm = QStringLiteral("ﾉ"); break;
        case 12: p.leftArm = QStringLiteral("ヾ"); p.rightArm = QStringLiteral("ﾉ"); break;
        case 13: p.typing = true; break;
        case 14: p.typing = true; p.leftArm = p.rightArm = QStringLiteral("っ"); break;
        case 15: p.typing = true; p.rightArm = QStringLiteral("っ"); break;
        case 16: p.leftEye = p.rightEye = QStringLiteral(">"); break;
        case 17: p.leftEye = p.rightEye = QStringLiteral("O"); break;
        case 18: p.headDy = 3; p.mouth = QStringLiteral("ᴗᴗ"); break; // head nod down
        case 19: p.headDy = -3; break; // head nod up
        case 20: p.headDx = -3; p.rightEye = QStringLiteral("◔"); break;
        case 21: p.headDx = 3; p.leftEye = QStringLiteral("◔"); break;
        case 22: p.bodyDy = -3; break;
        case 23: p.bodyDy = 2; break;
        case 24: p.leftArm = QStringLiteral("ヽ"); break;
        case 25: p.rightArm = QStringLiteral("ﾉ"); break;
        default: break;
    }
    return p;
}

// Text form remains useful in tests and for users with custom pet glyphs.
// Actual animation is drawn slot-by-slot by pet_render.hpp.
inline QString glyph(int pose, const QString &original) {
    if (pose == 0) return original;
    const Parts p = parts(pose);
    QString result = QStringLiteral("(") + p.leftArm + QStringLiteral("｡") +
        p.leftEye + p.mouth + p.rightEye + QStringLiteral("｡)") + p.rightArm;
    if (p.tailSide) result += QStringLiteral("●");
    return result;
}
} // namespace pet
