#pragma once

#include <QString>

// Approved CRT Clock v0.5 pose catalog.
// A pose is a static drawing, not an action or an input event.
// Never measure Unicode glyphs, rescale, or recompute layout for each frame.
namespace pet {

// These dimensions are calibrated globally, not per character or animation.
// A compact 310px front silhouette replaces v0.4.1's oversized 400px grid.
constexpr int kFrontColumns = 10;
constexpr int kBackColumns = 11;
constexpr int kFrontCellPixels = 31;
constexpr int kBackCellPixels = 28;
constexpr int kFontPixels = 41;
constexpr int kPoseCount = 39;

enum PoseId : int {
    F00 = 0, F01, F02, F03, F04, F05, F06, F07, F08, F09,
    F10, F11, F12, F13, F14, F15, F16, F17,
    H00, H01, H02, H03, H04, H05, H06,
    B00, B01, B02,
    NodDown, NodUp, SwayLeft, SwayRight, BounceUp, BounceDown,
    T00, T01, T02,
    JoyTailLeft, JoyTailRight
};

struct Pose {
    QString line = QStringLiteral("(づ｡◕‿‿◕｡)づ");
    bool back = false;
    bool typing = false;
    bool frontTail = false;
    int tailDx = 0;
    int tailDy = 0;
    int headDx = 0;
    int headDy = 0;
    int bodyDx = 0;
    int bodyDy = 0;
};

// All front-facing frames occupy exactly ten fixed columns. Historical
// backside wagging is a deliberate eleven-column alternate silhouette.
inline Pose frame(int id) {
    Pose p;
    switch (id) {
        case F01: p.line = QStringLiteral("(づ｡-‿‿-｡)づ"); break;
        case F02: p.line = QStringLiteral("(づ｡◔‿‿◔｡)づ"); break;  // both eyes right
        case F03: p.line = QStringLiteral("(づ｡◡‿‿◡｡)づ"); break;
        case F04: p.line = QStringLiteral("(づ｡>‿‿<｡)づ"); break;
        case F05: p.line = QStringLiteral("(づ｡0‿‿0｡)づ"); break;  // digit zero
        case F06: p.line = QStringLiteral("(づ｡¬‿‿¬｡)づ"); break;
        case F07: p.line = QStringLiteral("(づ｡•‿‿•｡)づ"); break;
        case F08: p.line = QStringLiteral("(づ｡^‿‿^｡)づ"); break;
        case F09: p.line = QStringLiteral("(づ｡◕‿‿-｡)づ"); break;
        case F10: p.line = QStringLiteral("(づ｡-‿‿◕｡)づ"); break;
        case F11: p.line = QStringLiteral("(づ｡?‿‿?｡)づ"); break;
        case F12: p.line = QStringLiteral("(づ｡ಠ‿‿ಠ｡)づ"); break;
        case F13: p.line = QStringLiteral("(づ｡ᴗ‿‿ᴗ｡)づ"); break;
        case F14: p.line = QStringLiteral("(づ｡•ᴗᴗ•｡)づ"); break;
        case F15: p.line = QStringLiteral("(づ｡0ᴗᴗ0｡)づ"); break;
        case F16: p.line = QStringLiteral("(づ｡>ᴗᴗ<｡)づ"); break;
        case F17: p.line = QStringLiteral("(づ｡◕ᴗᴗ◕｡)づ"); break;
        case H01: p.line = QStringLiteral("(づ｡◕‿‿◕｡)っ"); break;
        case H02: p.line = QStringLiteral("(っ｡◕‿‿◕｡)づ"); break;
        case H03: p.line = QStringLiteral("(っ｡◕‿‿◕｡)っ"); break;
        case H04: p.line = QStringLiteral("(づ｡>‿‿<｡)っ"); break;
        case H05: p.line = QStringLiteral("(っ｡>‿‿<｡)づ"); break;
        case H06: p.line = QStringLiteral("(づ｡◔‿‿◔｡)っ"); break;
        case B00: p.line = QStringLiteral("ᕦ(   ●   )ᕤ"); p.back = true; break;
        case B01: p.line = QStringLiteral("ᕤ(  ●    )ᕦ"); p.back = true; break;
        case B02: p.line = QStringLiteral("ᕦ(    ●  )ᕤ"); p.back = true; break;
        case NodDown: p.headDy = 3; break;
        case NodUp: p.headDy = -3; break;
        case SwayLeft: p.bodyDx = -3; break;
        case SwayRight: p.bodyDx = 3; break;
        case BounceUp: p.bodyDy = -3; break;
        case BounceDown: p.bodyDy = 3; break;
        case T00: p.typing = true; break;
        case T01: p.line = QStringLiteral("(っ｡◕‿‿◕｡)っ"); p.typing = true; break;
        case T02: p.line = QStringLiteral("(づ｡◕‿‿◕｡)っ"); p.typing = true; break;
        // Front-facing joy wag uses a separate dot: never extend the text grid.
        case JoyTailLeft:
            p.line = QStringLiteral("(づ｡>‿‿<｡)づ");
            p.frontTail = true;
            p.tailDx = -5;
            p.tailDy = -2;
            break;
        case JoyTailRight:
            p.line = QStringLiteral("(づ｡>‿‿<｡)づ");
            p.frontTail = true;
            p.tailDx = 5;
            p.tailDy = 2;
            break;
        default: break;  // F00/H00 are the original reference pose
    }
    return p;
}
} // namespace pet
