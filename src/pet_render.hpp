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
    p.save();
    if (pose.retreatScale < 1.0) {
        // Rare fetch story only. The character's authored grid and font stay
        // unchanged; the completed rear-facing figure shrinks as it walks away.
        const QPointF center = area.center();
        p.translate(center.x() + pose.retreatDx, center.y());
        p.scale(pose.retreatScale, pose.retreatScale);
        p.translate(-center.x(), -center.y());
    }
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

    if (pose.carriesGift) {
        // A tiny vector gift appears just outside the raised right paw.
        // It never changes the fixed ten-column character grid.
        p.save();
        QPolygonF sparkle;
        const QPointF c(left + width + 5.0, area.center().y() + 4.0);
        sparkle << QPointF(c.x(), c.y() - 8) << QPointF(c.x() + 5, c.y())
                << QPointF(c.x(), c.y() + 8) << QPointF(c.x() - 5, c.y());
        p.setPen(QPen(color, 1));
        p.setBrush(color);
        p.drawPolygon(sparkle);
        p.restore();
    }
    p.restore();
}

// Idle ambient movement is independent of the slow idle shuffle-bag actions.
// Short low-contrast strokes near paws plus a one-pixel breathing motion
// prevent long idle pauses from looking like a frozen screenshot.
inline void paintIdleMarks(QPainter &p, const QRectF &area, QColor color, int phase) {
    if (phase < 0) return;
    phase %= 6;
    color.setAlphaF(color.alphaF() * 0.46);
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap));
    const qreal y = area.center().y();
    if (phase == 1 || phase == 2 || phase == 5) {
        p.drawLine(QPointF(area.center().x()-168, y-14),
                   QPointF(area.center().x()-174, y-19));
        p.drawLine(QPointF(area.center().x()-166, y-1),
                   QPointF(area.center().x()-173, y-2));
    }
    if (phase == 3 || phase == 4 || phase == 5) {
        p.drawLine(QPointF(area.center().x()+166, y-14),
                   QPointF(area.center().x()+174, y-19));
        p.drawLine(QPointF(area.center().x()+168, y-1),
                   QPointF(area.center().x()+174, y-2));
    }
    p.restore();
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

    // Six staggered quavers in the short strip above the pet, each with
    // independently phased vertical travel. No audio capture or analysis.
    // They avoid the Matrix columns at x=398/421 and stay within 480x320.
    static constexpr qreal xOffset[6] = {-129, -88, -44, 6, 55, 101};
    static constexpr int stagger[6] = {0, 3, 6, 2, 5, 1};
    for (int i = 0; i < 6; ++i) {
        const int flight = (phase + stagger[i]) % 8;
        const qreal x = area.center().x() + xOffset[i];
        const qreal y = area.top() + 12 + (i % 3) * 3 - flight * 2.6;
        quaver(x, y);
    }
    p.restore();
}

} // namespace pet
