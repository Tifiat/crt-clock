#pragma once

#include "pet_poses.hpp"
#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QRectF>
#include <initializer_list>

namespace pet {

// Tiny vector kanji to avoid missing-glyph squares without a heavyweight CJK
// font dependency. Twelve deliberately simple characters use fixed strokes.
inline void stroke(QPainter &p, std::initializer_list<QPointF> points) {
    QPainterPath path;
    bool first = true;
    for (const QPointF &point : points) {
        if (first) { path.moveTo(point); first = false; }
        else path.lineTo(point);
    }
    p.drawPath(path);
}

inline void drawKanji(QPainter &p, QRectF rect, unsigned int index, QColor color) {
    p.save();
    p.translate(rect.topLeft());
    p.scale(rect.width() / 20.0, rect.height() / 20.0);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(color, 1.35, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
    switch (index % 12) {
        case 0: // 日
            stroke(p, {{4,2},{4,18},{16,18},{16,2},{4,2}});
            stroke(p, {{4,10},{16,10}});
            break;
        case 1: // 月
            stroke(p, {{6,2},{6,13},{4,18}});
            stroke(p, {{6,2},{16,2},{16,18},{12,18}});
            stroke(p, {{6,8},{16,8}});
            stroke(p, {{6,13},{16,13}});
            break;
        case 2: // 山
            stroke(p, {{3,8},{3,17},{17,17},{17,8}});
            stroke(p, {{10,2},{10,17}});
            break;
        case 3: // 川
            stroke(p, {{5,4},{5,13},{3,17}});
            stroke(p, {{10,3},{10,16}});
            stroke(p, {{16,2},{16,18}});
            break;
        case 4: // 木
            stroke(p, {{3,7},{17,7}});
            stroke(p, {{10,2},{10,18}});
            stroke(p, {{10,8},{3,16}});
            stroke(p, {{10,8},{17,16}});
            break;
        case 5: // 水
            stroke(p, {{10,2},{10,17},{7,19}});
            stroke(p, {{3,8},{7,8},{4,15}});
            stroke(p, {{11,10},{17,5}});
            stroke(p, {{11,10},{18,17}});
            break;
        case 6: // 火
            stroke(p, {{5,4},{7,9}});
            stroke(p, {{15,3},{13,9}});
            stroke(p, {{10,7},{9,13},{4,18}});
            stroke(p, {{10,11},{16,18}});
            break;
        case 7: // 田
            stroke(p, {{3,3},{17,3},{17,18},{3,18},{3,3}});
            stroke(p, {{10,3},{10,18}});
            stroke(p, {{3,10},{17,10}});
            break;
        case 8: // 中
            stroke(p, {{3,6},{17,6},{17,15},{3,15},{3,6}});
            stroke(p, {{10,2},{10,19}});
            break;
        case 9: // 土
            stroke(p, {{6,8},{14,8}});
            stroke(p, {{10,3},{10,17}});
            stroke(p, {{3,17},{17,17}});
            break;
        case 10: // 人
            stroke(p, {{11,2},{9,10},{3,18}});
            stroke(p, {{10,8},{17,18}});
            break;
        case 11: // 十
            stroke(p, {{3,8},{17,8}});
            stroke(p, {{10,2},{10,18}});
            break;
    }
    p.restore();
}

// Native canvas is 480x320. Ten static cells: no font measuring, no
// shrinking, no variable-width layout, no recentering across animations.
inline void paint(QPainter &p, QRectF area, const QString &family,
                  const QString &original, int pose, QColor color,
                  unsigned int keyIndex) {
    QFont font(family);
    font.setPixelSize(kFontPixels);
    font.setKerning(false);
    p.setFont(font);
    p.setPen(color);

    const QString canonical = QStringLiteral("(づ｡◕‿‿◕｡)づ");
    if (original != canonical) {
        // Legacy custom string: draw at fixed size, without auto-fitting.
        p.drawText(area, Qt::AlignCenter | Qt::TextSingleLine, original);
        return;
    }

    const Pose action = frame(pose);
    const qreal gridWidth = kColumns * kCellPixels;
    const qreal left = area.center().x() - gridWidth / 2 - (action.typing ? 20 : 0);
    const qreal top = area.top();

    for (int column = 0; column < kColumns; ++column) {
        // Positions are constant. Only explicitly animated face/body offsets vary.
        const bool face = column >= 2 && column <= 7;
        const qreal x = left + column * kCellPixels + (face ? action.headDx : 0);
        const qreal y = top + action.bodyDy + (face ? action.headDy : 0);
        p.drawText(QRectF(x, y, kCellPixels, area.height()),
                   Qt::AlignCenter | Qt::TextSingleLine,
                   QString(action.line.at(column)));
    }

    if (action.tailOffset) {
        p.save();
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        const qreal tailX = left + gridWidth - 42 + action.tailOffset;
        const qreal tailY = area.center().y() + 18;
        p.drawEllipse(QPointF(tailX, tailY), 4.5, 4.5);
        p.restore();
    }

    if (action.typing) {
        // Independent permanent indicator slot; no font fallback and no reflow.
        drawKanji(p, QRectF(area.right() - 31, area.center().y() - 15, 23, 29),
                  keyIndex, color);
    }
}
} // namespace pet
