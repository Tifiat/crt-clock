#pragma once

#include <QString>

namespace pet {

// Every built-in frame is an authored 10-character line.
// The grid and font size NEVER depend on the selected pose or glyph widths.
constexpr int kColumns = 10;
constexpr int kCellPixels = 40;
constexpr int kFontPixels = 38;

struct Pose {
    QString line = QStringLiteral("(づ｡◕‿‿◕｡)づ");
    int headDx = 0;
    int headDy = 0;
    int bodyDy = 0;
    int tailOffset = 0;
    bool typing = false;
};

inline Pose frame(int id) {
    Pose p;
    switch (id) {
        case 1:  p.line = QStringLiteral("(づ｡－‿‿－｡)づ"); break;  // Blink
        case 2:  p.line = QStringLiteral("(づ｡◕‿‿◔｡)づ"); break;  // Look right
        case 3:  p.line = QStringLiteral("(づ｡◔‿‿◕｡)づ"); break;  // Look left
        case 4:  p.line = QStringLiteral("(づ｡◕‿‿◕｡)っ"); break;  // Right wave
        case 5:  p.line = QStringLiteral("(っ｡◕‿‿◕｡)づ"); break;  // Left wave
        case 6:  p.line = QStringLiteral("(ヽ｡◕‿‿◕｡)ﾉ"); break;  // Stretch
        case 7:  p.line = QStringLiteral("(づ｡－‿‿－｡)づ"); break;  // Nap
        case 8:  p.tailOffset = -1; break;                         // Tail ready
        case 9:  p.tailOffset = -5; break;                         // Wag left
        case 10: p.tailOffset = 5; break;                          // Wag right
        case 11: p.line = QStringLiteral("(ヽ｡◕‿‿◕｡)ﾉ"); break;  // Both arms up
        case 12: p.line = QStringLiteral("(ヾ｡◕‿‿◕｡)ﾉ"); break;  // Arms waving
        case 13: p.typing = true; break;                           // Typing left
        case 14: p.line = QStringLiteral("(っ｡◕‿‿◕｡)っ"); p.typing = true; break;
        case 15: p.line = QStringLiteral("(づ｡◕‿‿◕｡)っ"); p.typing = true; break;
        case 16: p.line = QStringLiteral("(づ｡>‿‿<｡)づ"); break;  // Happy
        case 17: p.line = QStringLiteral("(づ｡O‿‿O｡)づ"); break;  // Surprise
        case 18: p.line = QStringLiteral("(づ｡◕ᴗᴗ◕｡)づ"); p.headDy = 3; break;
        case 19: p.headDy = -3; break;                              // Head up
        case 20: p.line = QStringLiteral("(づ｡◕‿‿◔｡)づ"); p.headDx = -3; break;
        case 21: p.line = QStringLiteral("(づ｡◔‿‿◕｡)づ"); p.headDx = 3; break;
        case 22: p.bodyDy = -3; break;                              // Bounce up
        case 23: p.bodyDy = 2; break;                               // Bounce down
        case 24: p.line = QStringLiteral("(ヽ｡◕‿‿◕｡)づ"); break;  // Left arm up
        case 25: p.line = QStringLiteral("(づ｡◕‿‿◕｡)ﾉ"); break;  // Right arm up
        default: break;                                            // Neutral
    }
    return p;
}
} // namespace pet
