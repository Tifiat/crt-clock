#pragma once

#include "pet_poses.hpp"
#include "pet_tail.hpp"
#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRectF>
#include <algorithm>

// Renderer only: fixed columns, a user-selected font/size and stroke density
// shared by every pose. Never auto-fit individual Unicode characters.
namespace pet {

inline void paint(QPainter &p, const QRectF &area, const QString &family,
                  const QString &original, int poseId, QColor color,
                  int fontPixels, int strokeStrength, const JoyTail &tail) {
    const int thickness = std::clamp(strokeStrength, 0, 100);
    QFont font(family);
    font.setPixelSize(std::clamp(fontPixels, 30, 54));
    font.setWeight(static_cast<QFont::Weight>(
        std::clamp(400 + thickness * 3, 400, 700)));
    font.setKerning(false);
    p.setFont(font);
    p.setPen(color);

    const QString canonical = QStringLiteral("(づ｡◕‿‿◕｡)づ");
    if (original != canonical) {
        // User-supplied glyph: fixed font parameters, no animated built-in pose.
        p.drawText(area, Qt::AlignCenter | Qt::TextSingleLine, original);
        return;
    }

    const Pose pose = frame(poseId);
    const int columns = pose.back ? kBackColumns : kFrontColumns;
    const int cell = pose.back ? kBackCellPixels : kFrontCellPixels;
    const qreal width = columns * cell;
    const qreal left = area.center().x() - width / 2.0
                      - (pose.typing ? 20 : 0) + pose.bodyDx;
    const qreal top = area.top() + pose.bodyDy;

    // The default is deliberately lighter than the previous five impressions.
    // Changing thickness never alters the font's size or the cell positions.
    static constexpr qreal offsets[][2] = {
        {0, 0}, {0.42, 0}, {-0.42, 0},
        {0, 0.55}, {0, -0.55}, {0.8, 0}, {-0.8, 0}
    };
    const int strokes = thickness < 20 ? 1 :
                        thickness < 35 ? 2 :
                        thickness < 55 ? 3 :
                        thickness < 80 ? 5 : 7;
    for (int column = 0; column < columns; ++column) {
        const bool face = !pose.back && column >= 2 && column <= 7;
        const QRectF slot(left + column * cell + (face ? pose.headDx : 0),
                          top + (face ? pose.headDy : 0),
                          cell, area.height());
        const QString letter(pose.line.at(column));
        if (letter == QStringLiteral(" ")) continue;
        for (int i = 0; i < strokes; ++i)
            p.drawText(slot.translated(offsets[i][0], offsets[i][1]),
                       Qt::AlignCenter | Qt::TextSingleLine, letter);
    }

    // The joyful dot is attached to the body, between the closing bracket
    // and right paw, NOT beyond the paw as it was in v0.5.1.
    if (!pose.back && tail.visible()) {
        p.save();
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        const QPointF center(left + width - cell * 1.2 + tail.side() * 4.0,
                             area.center().y() + pose.bodyDy + 16 +
                             tail.side() * 2.0);
        p.drawEllipse(center, 4.3, 4.3);
        p.restore();
    }
}
    
// Notes are a music-only decorative overlay, independent of featured pose.
// Draw simple vector quavers: no reliance on font glyph fallback or text
// metrics, no new character cells, no overlap with Matrix's right columns.
// phase < 0 suppresses music notes completely (idle and typing).
inline void paintMusicNotes(QPainter &p, const QRectF &area,
                            QColor color, int phase) {
    if (phase < 0) return;
    phase %= 8;
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    color.setAlphaF(color.alphaF() * 0.85);
    p.setPen(QPen(color, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(color);

    auto quaver = [&p](qreal x, qreal y) {
        p.drawEllipse(QRectF(x - 4, y + 12, 8, 6));
        p.drawLine(QPointF(x + 4, y + 15), QPointF(x + 4, y - 1));
        QPainterPath flag;
        flag.moveTo(x + 4, y - 1);
        flag.cubicTo(x + 15, y + 1, x + 14, y + 9, x + 7, y + 10);
        p.setBrush(Qt::NoBrush);
        p.drawPath(flag);
        p.setBrush(p.pen().color());
    };

    // Two staggered upward flights loop continuously while MPRIS is Playing.
    // They use the short clear strip just above the pet, not the date cells.
    quaver(area.center().x() + 32, area.top() + 5 - (phase % 4) * 5);
    quaver(area.center().x() + 92, area.top() + 11 - ((phase + 2) % 4) * 5);
    p.restore();
}

} // namespace pet
