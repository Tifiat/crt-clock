#pragma once

#include "pet_gift.hpp"
#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPolygonF>
#include <QPointF>
#include <algorithm>
#include <cmath>
#include <numbers>

namespace pet {

inline QColor giftTint(QColor color, double alpha) {
    color.setAlphaF(std::clamp(color.alphaF() * alpha, 0.0, 1.0));
    return color;
}

inline QPointF giftFlight(double t, bool arc) {
    const qreal x = 392.0 + 165.0 * t;
    const qreal y = 247.0 - 165.0 * t - (arc ? 34.0 * std::sin(std::numbers::pi * t) : 0.0);
    return {x, y};
}

inline void giftStar(QPainter &p, QPointF point, qreal radius, QColor color) {
    QPolygonF star;
    for (int i = 0; i < 10; ++i) {
        const qreal angle = -std::numbers::pi / 2 + i * std::numbers::pi / 5;
        const qreal r = i % 2 ? radius * 0.39 : radius;
        star << QPointF(point.x() + r * std::cos(angle),
                        point.y() + r * std::sin(angle));
    }
    p.setPen(QPen(giftTint(color, .9), 1.15));
    p.setBrush(giftTint(color, .66));
    p.drawPolygon(star);
}

inline void giftBox(QPainter &p, QPointF center, qreal size, QColor color) {
    const QRectF rect(center.x() - size/2, center.y() - size/2,
                      size, size);
    p.setPen(QPen(color, 2));
    p.setBrush(giftTint(color, .14));
    p.drawRoundedRect(rect, 2, 2);
    p.drawLine(QPointF(center.x(), rect.top()),
               QPointF(center.x(), rect.bottom()));
    p.drawLine(QPointF(rect.left(), center.y()-size*.16),
               QPointF(rect.right(), center.y()-size*.16));
    QPainterPath bow;
    bow.moveTo(center.x(), rect.top());
    bow.cubicTo(center.x()-size*.95, rect.top()-size*.8,
                center.x()-size*.7, rect.top()+size*.4,
                center.x(), rect.top());
    bow.cubicTo(center.x()+size*.7, rect.top()-size*.8,
                center.x()+size*.95, rect.top()+size*.4,
                center.x(), rect.top());
    p.setBrush(Qt::NoBrush);
    p.drawPath(bow);
}

inline void giftDrop(QPainter &p, qreal x, qreal y, qreal size, QColor color) {
    QPainterPath drop;
    drop.moveTo(x, y-size);
    drop.cubicTo(x-2.8*size/5, y-size*.3,
                  x-size*.7, y+size*.38, x, y+size*.75);
    drop.cubicTo(x+size*.7, y+size*.38,
                  x+2.8*size/5, y-size*.3, x, y-size);
    p.setPen(QPen(giftTint(color, .9), 1));
    p.setBrush(giftTint(color, .62));
    p.drawPath(drop);
}

inline void paintGiftEffect(QPainter &p, const GiftVisual &visual, QColor phosphor) {
    if (!visual.active()) return;
    const double t = std::clamp(visual.progress, 0.0, 1.0);
    const QPointF origin(392, 247);
    const QColor bright("#d8ffe1");
    QColor color = phosphor;
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setClipRect(QRectF(0, 0, 480, 320));

    switch (visual.cue) {
        case GiftCue::None: break;
        case GiftCue::Present: {
            const qreal pulse = 1.0 + .04 * std::sin(t * 2.0 * std::numbers::pi);
            if (isBurst(visual.kind)) giftBox(p, origin, 25 * pulse, color);
            else giftStar(p, origin, 10 * pulse, color);
            break;
        }
        case GiftCue::FlyStraight:
        case GiftCue::FlyArc: {
            const bool arc = visual.cue == GiftCue::FlyArc;
            const QPointF point = giftFlight(t, arc);
            // Three delicate disappearing trails follow the gift, never
            // appearing in the unrelated idle breathing animation.
            for (int i = 1; i <= 6; ++i) {
                const double prevT = std::max(0.0, t - i * .028);
                const QPointF old = giftFlight(prevT, arc);
                const double alpha = .22 * (1.0 - t) * (1.0 - i / 7.0);
                const QColor trail = giftTint(color, alpha);
                p.setPen(QPen(trail, i % 2 ? 1.7 : 1.0, Qt::SolidLine, Qt::RoundCap));
                const qreal sway = i % 2 ? -1.5 : 1.5;
                p.drawLine(old + QPointF(-6, sway), old + QPointF(0, sway));
            }
            giftStar(p, point, 10.0 + 2.0 * std::sin(std::numbers::pi * t), color);
            break;
        }
        case GiftCue::Pulse: {
            const qreal pulse = 10.0 + 6.0 * std::abs(std::sin(t * 4.0 * std::numbers::pi));
            const QColor faded = giftTint(color, 1.0 - .7 * t);
            giftStar(p, origin, pulse, faded);
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(giftTint(bright, .32 * (1.0-t)), 1.5));
            p.drawEllipse(origin, pulse + 7, pulse + 7);
            break;
        }
        case GiftCue::Charge: {
            const qreal pulse = 1 + .08 * std::sin(t * 10 * std::numbers::pi);
            giftBox(p, origin, (26 + 8*t)*pulse, color);
            for (int i = 0; i < 4; ++i) {
                const double angle = std::numbers::pi/2 * i + 2.8*t;
                const qreal r = 25 + t * 12;
                const QPointF dot(origin.x()+r*std::cos(angle),
                                  origin.y()+r*std::sin(angle));
                p.setPen(Qt::NoPen);
                p.setBrush(giftTint(bright, .28 + .4*t));
                p.drawEllipse(dot, 2.3 + t*1.5, 2.3 + t*1.5);
            }
            break;
        }
        case GiftCue::Burst: {
            const double power = visual.kind == GiftKind::BurstCry ? 1.15 : 1.0;
            const qreal r = power*(12 + 85 * t);
            const double fade = std::pow(1.0-t, 1.2);
            // Sharp central flash decays quickly; two separated rings
            // retain the explosion silhouette behind the moving shrapnel.
            if (t < .28) {
                const qreal flash = 23 + 25 * (t/.28);
                p.setBrush(giftTint(bright, .32 * (1-t/.28)));
                p.setPen(Qt::NoPen);
                p.drawEllipse(origin, flash, flash);
            }
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(giftTint(bright, .98*fade), 3.0 - 1.2*t));
            p.drawEllipse(origin, r, r);
            p.setPen(QPen(giftTint(color, .55*fade), 1.3));
            p.drawEllipse(origin, r*.68, r*.68);
            // Spokes are radial, staggered and tapered instead of
            // a flat "star" abruptly appearing/disappearing.
            for (int i = 0; i < 14; ++i) {
                const double a = 2*std::numbers::pi*i/14 + .13;
                const double x = std::cos(a), y = std::sin(a);
                const qreal inner = r*.74, outer = r*(1.12 + (i%3)*.05);
                p.setPen(QPen(giftTint(bright, .8*fade), i%3==0 ? 2.7 : 1.7,
                              Qt::SolidLine, Qt::RoundCap));
                p.drawLine(origin+QPointF(inner*x, inner*y),
                           origin+QPointF(outer*x, outer*y));
            }
            // Independently flying sparks, a faint echo of the old motion
            // dashes, with no full-screen flashes or CPU-intensive particles.
            for (int i = 0; i < 16; ++i) {
                const double a = 2*std::numbers::pi*(i+.35)/16;
                const double distance = (22 + (i%5)*6) + t*(55 + (i%4)*25)*power;
                const QPointF pos = origin + QPointF(std::cos(a)*distance,
                                                      std::sin(a)*distance);
                const qreal size = (i%3==0 ? 4.0 : 2.5)*(1.0-t*.6);
                p.setPen(QPen(giftTint(color, (.75-(i%3)*.09)*fade), 1.5));
                p.drawLine(pos-QPointF(size,size*.35),
                           pos+QPointF(size,-size*.35));
                if (i%4==0) giftStar(p, pos, size*1.4, giftTint(bright, fade));
            }
            break;
        }
        case GiftCue::TearLeft:
        case GiftCue::TearRight: {
            const qreal x = visual.cue == GiftCue::TearLeft ? 192.0 : 287.0;
            // Every cue is a separate 600ms tear, alternating eyes. The
            // single tear starts by the eye and falls below the face.
            const qreal y = 251.0 + 33.0 * t*t;
            const double alpha = t < .12 ? t/.12 :
                                 t > .85 ? (1.0-t)/.15 : 1.0;
            giftDrop(p, x, y, 6.0, giftTint(QColor("#b5faff"), alpha));
            break;
        }
    }
    p.restore();
}

} // namespace pet
