#pragma once

#include "pet_poses.hpp"
#include <QFont>
#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>
#include <QRectF>
#include <array>
#include <algorithm>
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

inline void paint(QPainter &p, QRectF area, const QString &family,
                  const QString &original, int pose, QColor color,
                  unsigned int keyIndex) {
    const Parts part = parts(pose);
    const QString canonical = QStringLiteral("(づ｡◕‿‿◕｡)づ");
    QFont font(family);
    font.setPixelSize(46);

    // Arbitrary configured pets still draw normally. The fixed-slot layout
    // applies to the built-in ASCII companion.
    if (original != canonical) {
        while (font.pixelSize() > 19 &&
               QFontMetricsF(font).horizontalAdvance(original) > area.width() - 30)
            font.setPixelSize(font.pixelSize()-1);
        p.setPen(color);
        p.setFont(font);
        p.drawText(area, Qt::AlignCenter | Qt::TextSingleLine, original);
    } else {
        std::array<qreal, 9> widths{};
        qreal fullWidth = 0;
        while (true) {
            const QFontMetricsF m(font);
            const qreal arm = std::max({m.horizontalAdvance(QStringLiteral("づ")),
                m.horizontalAdvance(QStringLiteral("っ")),
                m.horizontalAdvance(QStringLiteral("ヽ")),
                m.horizontalAdvance(QStringLiteral("ヾ")),
                m.horizontalAdvance(QStringLiteral("ﾉ"))});
            const qreal eye = std::max({m.horizontalAdvance(QStringLiteral("◕")),
                m.horizontalAdvance(QStringLiteral("◔")),
                m.horizontalAdvance(QStringLiteral("－")),
                m.horizontalAdvance(QStringLiteral("O")),
                m.horizontalAdvance(QStringLiteral(">"))});
            const qreal mouth = std::max(m.horizontalAdvance(QStringLiteral("‿‿")),
                                         m.horizontalAdvance(QStringLiteral("ᴗᴗ")));
            widths = {m.horizontalAdvance(QStringLiteral("(")), arm,
                      m.horizontalAdvance(QStringLiteral("｡")), eye, mouth, eye,
                      m.horizontalAdvance(QStringLiteral("｡")),
                      m.horizontalAdvance(QStringLiteral(")")), arm};
            fullWidth = 0;
            for (const auto width : widths) fullWidth += width;
            if (fullWidth <= area.width()-40 || font.pixelSize() <= 20) break;
            font.setPixelSize(font.pixelSize()-1);
        }

        const QFontMetricsF metrics(font);
        const qreal height = metrics.height()+6;
        const qreal top = area.center().y() - height/2;
        qreal x = area.center().x() - fullWidth/2 - (part.typing ? 20 : 0);
        const std::array<QString,9> segments = {
            QStringLiteral("("), part.leftArm, QStringLiteral("｡"),
            part.leftEye, part.mouth, part.rightEye, QStringLiteral("｡"),
            QStringLiteral(")"), part.rightArm
        };
        p.setFont(font);
        p.setPen(color);
        for (int i=0; i<9; ++i) {
            const int face = (i>=2 && i<=6);
            const qreal dx = face ? part.headDx : 0;
            const qreal dy = part.bodyDy + (face ? part.headDy : 0);
            p.drawText(QRectF(x+dx, top+dy, widths[i], height),
                       Qt::AlignCenter | Qt::TextSingleLine, segments[i]);
            x += widths[i];
        }
        if (part.tailSide) {
            p.save();
            p.setPen(Qt::NoPen);
            p.setBrush(color);
            const QPointF tail(area.center().x() + fullWidth*.29 -
                                   (part.typing ? 20 : 0) + part.tailSide*2,
                               top + height*.77 + (part.tailSide < 0 ? -1 : 1));
            p.drawEllipse(tail, 4.5, 4.5);
            p.restore();
        }
    }

    if (part.typing) {
        // Japanese key-activity indicator stays in its own fixed slot.
        drawKanji(p, QRectF(area.right()-31, area.center().y()-15, 23, 29),
                  keyIndex, color);
    }
}
} // namespace pet
