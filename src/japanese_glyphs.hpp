#pragma once

#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QRectF>
#include <initializer_list>

// Font-independent Japanese glyphs for Matrix streams. Their stroke width is
// authored here; they never depend on the selected pet font or its metrics.
namespace japanese {
inline void stroke(QPainter &p, std::initializer_list<QPointF> points) {
    QPainterPath path;
    bool first = true;
    for (const QPointF &point : points) {
        if (first) { path.moveTo(point); first = false; }
        else path.lineTo(point);
    }
    p.drawPath(path);
}

inline void drawGlyph(QPainter &p, QRectF rect, unsigned int index, QColor color) {
    p.save();
    p.translate(rect.topLeft());
    p.scale(rect.width() / 20.0, rect.height() / 20.0);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(color, 2.35, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
    switch (index % 24) {
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
        case 12: // ロ
            stroke(p, {{3,3},{17,3},{17,17},{3,17},{3,3}});
            break;
        case 13: // ツ
            stroke(p, {{4,5},{5,10}});
            stroke(p, {{9,4},{10,9}});
            stroke(p, {{17,4},{15,12},{7,17}});
            break;
        case 14: // マ
            stroke(p, {{3,5},{17,5},{12,12},{8,15}});
            stroke(p, {{6,11},{13,17}});
            break;
        case 15: // ネ
            stroke(p, {{5,4},{9,4},{7,2}});
            stroke(p, {{3,8},{17,8},{7,14},{7,19}});
            stroke(p, {{9,12},{16,17}});
            break;
        case 16: // ホ
            stroke(p, {{4,6},{16,6}});
            stroke(p, {{10,2},{10,18}});
            stroke(p, {{5,11},{3,17}});
            stroke(p, {{15,11},{18,17}});
            break;
        case 17: // ア
            stroke(p, {{3,4},{17,4},{14,10}});
            stroke(p, {{10,7},{9,15},{6,18}});
            break;
        case 18: // ミ
            stroke(p, {{6,4},{14,6}});
            stroke(p, {{5,10},{15,12}});
            stroke(p, {{4,16},{16,19}});
            break;
        case 19: // ク
            stroke(p, {{9,2},{5,9}});
            stroke(p, {{9,4},{17,4},{14,13},{8,18}});
            break;
        case 20: // シ
            stroke(p, {{4,4},{8,6}});
            stroke(p, {{4,10},{8,12}});
            stroke(p, {{17,5},{15,13},{7,18}});
            break;
        case 21: // ラ
            stroke(p, {{4,5},{16,5}});
            stroke(p, {{3,10},{17,10},{12,18}});
            break;
        case 22: // モ
            stroke(p, {{5,5},{16,5}});
            stroke(p, {{4,10},{17,10}});
            stroke(p, {{11,2},{8,16},{11,18},{16,18}});
            break;
        case 23: // ナ
            stroke(p, {{3,8},{17,8}});
            stroke(p, {{11,2},{11,12},{7,18}});
            break;
    }
    p.restore();
}


} // namespace japanese
