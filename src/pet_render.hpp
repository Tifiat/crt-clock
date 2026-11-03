#pragma once

#include "pet_poses.hpp"
#include <QFont>
#include <QPainter>
#include <QRectF>

// Presentation only. Pose data and scenario scheduling live elsewhere.
// The native canvas, glyph cells, font size, anchors and stroke thickness do
// not vary by pose: only explicitly authored head/body movements are allowed.
namespace pet {

inline void paint(QPainter &p, const QRectF &area, const QString &family,
                  const QString &original, int poseId, QColor color) {
    QFont font(family);
    font.setPixelSize(kFontPixels);
    font.setWeight(QFont::Black);
    font.setKerning(false);
    p.setFont(font);
    p.setPen(color);

    const QString canonical = QStringLiteral("(づ｡◕‿‿◕｡)づ");
    if (original != canonical) {
        // Preserve user-provided custom strings without adaptive rescaling.
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

    // In source resolution, these four subpixel offsets thicken the outline,
    // but never scale the pet or alter the advance of any character.
    static constexpr qreal offsets[5][2] = {
        {0.0, 0.0}, {0.8, 0.0}, {-0.8, 0.0}, {0.0, 0.8}, {0.0, -0.8}
    };
    for (int column = 0; column < columns; ++column) {
        const bool face = !pose.back && column >= 2 && column <= 7;
        const QRectF slot(left + column * cell + (face ? pose.headDx : 0),
                          top + (face ? pose.headDy : 0),
                          cell, area.height());
        const QString letter(pose.line.at(column));
        if (letter == QStringLiteral(" ")) continue;
        for (const auto &offset : offsets)
            p.drawText(slot.translated(offset[0], offset[1]),
                       Qt::AlignCenter | Qt::TextSingleLine, letter);
    }

    // Front-tail decoration is independent of the ten text columns.
    // Only the two explicit joy frames enable it; the backside wag is unchanged.
    if (pose.frontTail) {
        p.save();
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        const QPointF tailCenter(left + width + 9 + pose.tailDx,
                                 area.center().y() + 13 + pose.tailDy);
        p.drawEllipse(tailCenter, 4.8, 4.8);
        p.restore();
    }
}
} // namespace pet
