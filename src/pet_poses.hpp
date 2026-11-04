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
constexpr int kDefaultFontPixels = 43;
constexpr int kDefaultStrokeStrength = 25;
constexpr int kPoseCount = 45;

enum PoseId : int {
    F00 = 0, F01, F02, F03, F04, F05, F06, F07, F08, F09,
    F10, F11, F12, F13, F14, F15, F16, F17,
    H00, H01, H02, H03, H04, H05, H06,
    B00, B01, B02,
    NodDown, NodUp, SwayLeft, SwayRight, BounceUp, BounceDown,
    T00, T01, T02,
    JoyTailLeft, JoyTailRight,
    // New IDs append only: the 39 historic pose values never change.
    ListenNote0, ListenNote1, ListenNote2, ListenNote3,
    StepLeft, StepRight
};

struct Pose {
    QString line = QStringLiteral("(づ｡◕‿‿◕｡)づ");
    bool back = false;
    bool typing = false;
    int headDx = 0;
    int headDy = 0;
    int bodyDx = 0;
    int bodyDy = 0;
    int notePhase = -1; // -1 = no notes; 0..3 = authored upward flight
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
        case NodDown: p.headDy = 5; break;
        case NodUp: p.headDy = -5; break;
        case SwayLeft: p.bodyDx = -8; break;
        case SwayRight: p.bodyDx = 8; break;
        case BounceUp: p.bodyDy = -6; break;
        case BounceDown: p.bodyDy = 6; break;
        case T00: p.typing = true; break;
        case T01: p.line = QStringLiteral("(っ｡◕‿‿◕｡)っ"); p.typing = true; break;
        case T02: p.line = QStringLiteral("(づ｡◕‿‿◕｡)っ"); p.typing = true; break;
        // Kept as stable v0.5.1 IDs, but the tail is now tied to joyful eyes
        // by JoyTail; these legacy frames no longer impose their own wag.
        case JoyTailLeft:
        case JoyTailRight: p.line = QStringLiteral("(づ｡>‿‿<｡)づ"); break;
        case ListenNote0:
        case ListenNote1:
        case ListenNote2:
        case ListenNote3:
            p.line = QStringLiteral("(づ｡-‿‿-｡)づ"); // closed eyes, fixed front grid
            p.notePhase = id - ListenNote0;
            // A tiny sway, distinct from the note flight itself.
            p.bodyDx = (p.notePhase % 2 == 0) ? -3 : 3;
            break;
        case StepLeft:
            p.line = QStringLiteral("(づ｡>‿‿<｡)っ");
            p.bodyDx = -9;
            break;
        case StepRight:
            p.line = QStringLiteral("(っ｡>‿‿<｡)づ");
            p.bodyDx = 9;
            break;
        default: break;  // F00/H00 are the original reference pose
    }
    return p;
}
} // namespace pet
