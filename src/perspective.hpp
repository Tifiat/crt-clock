#pragma once

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QTransform>
#include <QPointF>
#include <QLineF>
#include <array>
#include <algorithm>

namespace perspective {
// order: TL, TR, BR, BL, TOP, RIGHT, BOTTOM, LEFT
using Handles = std::array<QPointF, 8>;

inline QPointF midpoint(const QPointF &a, const QPointF &b) {
    return (a + b) / 2.0;
}
inline Handles fromCorners(const std::array<QPointF, 4> &c) {
    return {c[0], c[1], c[2], c[3],
            midpoint(c[0], c[1]), midpoint(c[1], c[2]),
            midpoint(c[2], c[3]), midpoint(c[3], c[0])};
}
inline QPointF center(const Handles &h) {
    return (h[4] + h[5] + h[6] + h[7]) / 4.0;
}
inline QPolygonF outline(const Handles &h) {
    return QPolygonF{h[0], h[4], h[1], h[5], h[2], h[6], h[3], h[7]};
}
inline QRect pixelBounds(const Handles &h) {
    return outline(h).boundingRect().adjusted(-12, -12, 12, 12).toAlignedRect();
}

// Eight editable handles bend all four sides. Four projective cells meet at
// one derived centre; the previous four-corner geometry is a strict subset.
inline void draw(QPainter &p, const QImage &source, const Handles &absolute,
                 QPointF windowOrigin) {
    Handles h = absolute;
    for (auto &point : h) point -= windowOrigin;
    const QPointF mid = center(h);
    const qreal w = source.width(), ht = source.height();
    const QPointF tl(0, 0), tr(w, 0), br(w, ht), bl(0, ht);
    const QPointF tm(w/2, 0), rm(w, ht/2), bm(w/2, ht), lm(0, ht/2);
    const QPointF c(w/2, ht/2);

    const std::array<QPolygonF, 4> input = {{
        QPolygonF{tl, tm, c, lm},
        QPolygonF{tm, tr, rm, c},
        QPolygonF{c, rm, br, bm},
        QPolygonF{lm, c, bm, bl}
    }};
    const std::array<QPolygonF, 4> output = {{
        QPolygonF{h[0], h[4], mid, h[7]},
        QPolygonF{h[4], h[1], h[5], mid},
        QPolygonF{mid, h[5], h[2], h[6]},
        QPolygonF{h[7], mid, h[6], h[3]}
    }};

    for (int i = 0; i < 4; ++i) {
        QTransform warp;
        if (!QTransform::quadToQuad(input[i], output[i], warp)) continue;
        p.save();
        QPainterPath clip;
        clip.addPolygon(output[i]);
        p.setClipPath(clip);
        p.setTransform(warp);
        p.drawImage(QPointF(0, 0), source);
        p.restore();
    }
}

inline QPointF sidePoint(QPointF top, QPointF middle, QPointF bottom, qreal t) {
    return t < .5 ? top + (middle-top) * (t*2) :
                    middle + (bottom-middle) * ((t-.5)*2);
}

// Important: draw scanlines in *physical screen pixels*, AFTER warping.
// Source-space 480px scanlines previously disappeared on a ~200px monitor.
inline void scanlines(QPainter &p, const Handles &absolute,
                      QPointF origin, int strength) {
    if (strength <= 0) return;
    Handles h = absolute;
    for (auto &point : h) point -= origin;
    const int height = std::max(1, static_cast<int>(std::max(
            QLineF(h[0], h[3]).length(), QLineF(h[1], h[2]).length())));
    const int count = std::max(1, height / 3);
    p.save();
    QPainterPath clip;
    clip.addPolygon(outline(h));
    p.setClipPath(clip);
    QColor shade(0, 10, 3, std::clamp(strength * 165 / 100, 0, 165));
    p.setPen(QPen(shade, 0.8));
    for (int i = 0; i <= count; ++i) {
        const qreal t = (i + .4) / count;
        if (t > 1) break;
        p.drawLine(sidePoint(h[0], h[7], h[3], t),
                   sidePoint(h[1], h[5], h[2], t));
    }
    p.restore();
}
} // namespace perspective
