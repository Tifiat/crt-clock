#pragma once
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QRadialGradient>
#include <algorithm>

// CRT texture on a small offscreen image, before perspective mapping.
inline QImage crtComposite(const QImage &glyphs, QColor background, int backgroundAlpha,
                           int glow, int scanlines, bool border, QColor borderColor) {
    const int w = glyphs.width(), h = glyphs.height();
    QImage result(w, h, QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);
    QPainter p(&result);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    if (backgroundAlpha > 0) {
        background.setAlpha(std::clamp(backgroundAlpha, 0, 255));
        p.fillRect(result.rect(), background);
    }
    if (glow > 0) {
        // Low-resolution convolution: two inexpensive soft phosphor halos.
        QImage wide = glyphs.scaled(std::max(1, w / 12), std::max(1, h / 12),
                Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                .scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        QImage tight = glyphs.scaled(std::max(1, w / 5), std::max(1, h / 5),
                Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                .scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        p.setOpacity(0.65 * glow / 100.0);
        p.drawImage(0, 0, wide);
        p.setOpacity(0.40 * glow / 100.0);
        p.drawImage(0, 0, tight);
    }
    p.setOpacity(1);
    p.drawImage(0, 0, glyphs);
    if (scanlines > 0) {
        QColor dark(0, 0, 0, std::clamp(scanlines * 75 / 100, 0, 75));
        for (int y = 1; y < h; y += 4)
            p.fillRect(QRect(0, y, w, 1), dark);
    }
    if (backgroundAlpha > 0) {
        QRadialGradient v(QPointF(w * .5, h * .48), w * .74);
        v.setColorAt(0, QColor(0, 0, 0, 0));
        v.setColorAt(1, QColor(0, 0, 0, std::min(35, backgroundAlpha / 3)));
        p.fillRect(result.rect(), v);
    }
    if (border) {
        borderColor.setAlpha(135);
        p.setPen(QPen(borderColor, 1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(QRectF(1, 1, w - 2, h - 2));
    }
    return result;
}
