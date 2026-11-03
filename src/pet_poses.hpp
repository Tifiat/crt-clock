#pragma once
#include <QString>

// Primary expression remains the exact original reference.
// 4..7 are a short turn-and-bunny-tail sequence; 8..10 are music dance.
inline QString petPose(const QString &reference, int pose) {
    switch (pose) {
        case 1: return QStringLiteral("(づ｡－‿‿－｡)づ");
        case 2: return QStringLiteral("(っ｡◕‿‿◕｡)っ");
        case 3: return QStringLiteral("(づ｡◕‿‿◕｡)っ");
        case 4: return QStringLiteral("(っ｡◕‿‿◕｡)づ");
        case 5: return QStringLiteral("ᕦ(   ●   )ᕤ");
        case 6: return QStringLiteral("ᕤ(  ●    )ᕦ");
        case 7: return QStringLiteral("ᕦ(    ●  )ᕤ");
        case 8: return QStringLiteral("ヽ(｡◕‿‿◕｡)ﾉ");
        case 9: return QStringLiteral("ヾ(｡◕‿‿◕｡)ﾉ");
        case 10: return QStringLiteral("ヽ(｡◕‿‿◕｡)ノ");
        default: return reference;
    }
}
