#pragma once

#include "pet_render.hpp"
#include <QHash>
#include <QImage>
#include <QPainter>
#include <QColor>
#include <QString>

// Rasterize each fixed 10/11-cell pose only once for a given appearance.
// Movement, breathing and mirror are applied when compositing the cached
// bitmap, so they remain fluid at the same 40ms GUI tick without repeated
// Unicode shaping and multi-stroke font drawing on each frame.
namespace pet {

class SpriteCache {
public:
    static constexpr int kCanvasWidth = 480;
    static constexpr int kCropTop = 192;
    static constexpr int kSpriteHeight = 320 - kCropTop;

    void configure(const QString &font, const QString &original,
                   QColor color, int pixelSize, int stroke) {
        if (font_ == font && original_ == original && color_ == color &&
            pixelSize_ == pixelSize && stroke_ == stroke)
            return;
        font_ = font;
        original_ = original;
        color_ = color;
        pixelSize_ = pixelSize;
        stroke_ = stroke;
        images_.clear();
    }

    [[nodiscard]] const QImage &sprite(int pose, const JoyTail &tail) {
        const int tailCode = !tail.visible() ? 0 : (tail.side() < 0 ? 1 : 2);
        const int key = pose * 3 + tailCode;
        auto it = images_.find(key);
        if (it != images_.end()) return it.value();

        // 2x supersampling is paid once per sprite, not every 40ms. It
        // prevents jagged mid-transition glyph edges under the perspective.
        constexpr int ss = 2;
        QImage large(kCanvasWidth * ss, kSpriteHeight * ss,
                     QImage::Format_ARGB32_Premultiplied);
        large.fill(Qt::transparent);
        QPainter p(&large);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::TextAntialiasing, true);
        p.scale(ss, ss);
        p.translate(0, -kCropTop);
        pet::paint(p, QRectF(-40, 217, 460, 82), font_,
                   original_, pose, color_, pixelSize_, stroke_, tail);
        p.end();
        QImage sprite = large.scaled(kCanvasWidth, kSpriteHeight,
                                    Qt::IgnoreAspectRatio,
                                    Qt::SmoothTransformation);
        it = images_.insert(key, std::move(sprite));
        ++buildCount_;
        return it.value();
    }

    [[nodiscard]] int cachedPoses() const { return images_.size(); }
    [[nodiscard]] quint64 buildCount() const { return buildCount_; }

private:
    QString font_;
    QString original_;
    QColor color_;
    int pixelSize_ = -1;
    int stroke_ = -1;
    QHash<int, QImage> images_;
    quint64 buildCount_ = 0;
};

} // namespace pet
