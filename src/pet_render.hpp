#pragma once

#include "pet_poses.hpp"
#include "pet_tail.hpp"
#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRectF>
#include <algorithm>
#include <cmath>

// Renderer only: fixed columns, a user-selected font/size and stroke density
// shared by every pose. Never auto-fit individual Unicode characters.
namespace pet {

inline void paint(QPainter &p, const QRectF &area, const QString &family,
                  const QString &original, int poseId, QColor color,
                  int fontPixels, int strokeStrength, const JoyTail &tail,
                  const MotionVisual &motion = {}) {
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
    if (pose.hidden) return;
    p.save();
    if (motion.mirror) {
        // True visual turn: reflect only the actor during the return leg
        // of Patrol, never the whole CRT canvas, clock or particles.
        const qreal pivot = area.center().x() + motion.offsetX;
        p.translate(pivot, 0);
        p.scale(-1.0, 1.0);
        p.translate(-pivot, 0);
    }
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
                      - (pose.typing ? 20 : 0) + pose.bodyDx + motion.offsetX;
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
        const int expressionY = (column == 3 || column == 6) ? pose.eyeDy :
                                (column == 4 || column == 5) ? pose.mouthDy : 0;
        const QRectF slot(left + column * cell + (face ? pose.headDx : 0),
                          top + (face ? pose.headDy : 0) + expressionY,
                          cell, area.height());
        const QString letter(pose.line.at(column));
        if (letter == QStringLiteral(" ")) continue;
        for (int i = 0; i < strokes; ++i)
            p.drawText(slot.translated(offsets[i][0], offsets[i][1]),
                       Qt::AlignCenter | Qt::TextSingleLine, letter);
    }

    if (pose.back && poseId >= AwayFace && poseId <= AwayReturn) {
        // Real curved wagging tail for the walk-away silhouette.
        // Previous centered white circle looked like a dot/anus.
        const int sway = (poseId % 3) - 1;
        const QPointF root(left + width/2, area.center().y() + 13);
        const QPointF tip(root.x() + sway*15 + 3, root.y() - 23);
        QPainterPath tailPath;
        tailPath.moveTo(root);
        tailPath.cubicTo(root.x() - 11, root.y() - 15,
                         tip.x() + 9, tip.y() + 16, tip.x(), tip.y());
        p.save();
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(color, 3.1, Qt::SolidLine,
                      Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(tailPath);
        p.setBrush(color);
        p.drawEllipse(tip, 4.5, 4.5);
        p.restore();
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

// Sparse gestural marks only while the music character sways.
// The mark is on the OPPOSITE side of the movement, not permanent idle noise.
inline void paintGestureStreak(QPainter &p, const QRectF &area,
                               QColor color, int direction) {
    if (direction == 0) return;
    p.save();
    color.setAlphaF(color.alphaF() * .33);
    p.setPen(QPen(color, 1.7, Qt::SolidLine, Qt::RoundCap));
    const qreal edge = area.center().x() + (direction < 0 ? 168 : -168);
    const qreal y = area.center().y();
    const qreal side = direction < 0 ? 1.0 : -1.0;
    p.drawLine(QPointF(edge, y - 11), QPointF(edge + side*9, y - 14));
    p.drawLine(QPointF(edge, y + 2), QPointF(edge + side*6, y + 2));
    p.restore();
}

// A three-column, chessboard-staggered musical rain replaces the six notes
// over the pet's head. It uses the SAME screen strip as typing Matrix,
// but never runs at the same time because Typing has higher priority.
// No font glyph is sampled; every note is built from simple vector strokes.
inline void paintMusicStream(QPainter &p, QColor color, int phase) {
    if (phase < 0) return;
    const qreal offset = std::fmod(phase * 2.4, 18.0);
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);

    for (int column = 0; column < 3; ++column) {
        for (int row = 0; row < 19; ++row) {
            const qreal x = 390 + 23 * column;
            const qreal y = 308 - 18 * row - offset - (column % 2 ? 9 : 0);
            if (y < 54 || y > 314) continue;
            const qreal appear = std::clamp((314.0 - y) / 22.0, 0.0, 1.0);
            const qreal vanish = std::clamp((y - 54.0) / 75.0, 0.0, 1.0);
            const qreal variation = 0.74 + .26 * ((row * 7 + column * 3) % 5) / 4.0;
            QColor ink = color;
            ink.setAlphaF(color.alphaF() * .73 * appear * vanish * variation);
            if (ink.alphaF() < .025) continue;

            p.setPen(QPen(ink, 1.75, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.setBrush(ink);
            const bool doubleNote = (row + column * 2) % 4 == 0;
            if (doubleNote) {
                // A compact paired eighth note, 15px tall.
                p.drawEllipse(QRectF(x-7, y+2, 5, 4));
                p.drawEllipse(QRectF(x+3, y+2, 5, 4));
                p.drawLine(QPointF(x-2, y+4), QPointF(x-2, y-10));
                p.drawLine(QPointF(x+8, y+4), QPointF(x+8, y-10));
                p.drawLine(QPointF(x-2, y-10), QPointF(x+8, y-10));
            } else {
                p.drawEllipse(QRectF(x-4, y+2, 7, 5));
                p.drawLine(QPointF(x+3, y+4), QPointF(x+3, y-11));
                QPainterPath flag;
                flag.moveTo(x+3, y-11);
                flag.cubicTo(x+11, y-9, x+11, y-3, x+6, y-2);
                p.setBrush(Qt::NoBrush);
                p.drawPath(flag);
            }
        }
    }
    p.restore();
}

} // namespace pet
