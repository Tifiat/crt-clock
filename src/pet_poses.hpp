#pragma once

#include <QString>

// A pose is a single-line Unicode drawing. Keep the original reference
// unchanged; all alternative poses are opt-in animation frames.
namespace pet {
inline QString glyph(int pose, const QString &original) {
    switch (pose) {
        case 1:  return QStringLiteral("(づ｡－‿‿－｡)づ"); // Blink
        case 2:  return QStringLiteral("(づ｡◕‿‿◔｡)づ"); // Look left
        case 3:  return QStringLiteral("(づ｡◔‿‿◕｡)づ"); // Look right
        case 4:  return QStringLiteral("(づ｡◕‿‿◕｡)っ"); // Wave left
        case 5:  return QStringLiteral("(っ｡◕‿‿◕｡)づ"); // Wave right
        case 6:  return QStringLiteral("ヽ(｡◕‿‿◕｡)ﾉ"); // Stretch
        case 7:  return QStringLiteral("(づ｡－‿‿－｡)づ zZ"); // Nap
        case 8:  return QStringLiteral("ᕦ(   ●   )ᕤ"); // Turned away
        case 9:  return QStringLiteral("ᕤ(  ●    )ᕦ"); // Tail left
        case 10: return QStringLiteral("ᕦ(    ●  )ᕤ"); // Tail right
        case 11: return QStringLiteral("ヽ(｡◕‿‿◕｡)ﾉ"); // Dance left
        case 12: return QStringLiteral("ヾ(｡◕‿‿◕｡)ﾉ"); // Dance right
        case 13: return QStringLiteral("(づ｡◕‿‿◕｡)づ_"); // Typing left
        case 14: return QStringLiteral("(っ｡◕‿‿◕｡)っ_"); // Typing right
        case 15: return QStringLiteral("(づ｡◕‿‿◕｡)っ_"); // Typing alternating
        case 16: return QStringLiteral("(づ｡>‿‿<｡)づ"); // Happy
        case 17: return QStringLiteral("(づ｡O‿‿O｡)づ"); // Surprised
        default: return original;                     // EXACT reference pose
    }
}
}  // namespace pet
