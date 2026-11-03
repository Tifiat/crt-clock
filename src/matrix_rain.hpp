#pragma once

#include "japanese_glyphs.hpp"
#include <QImage>
#include <QPainter>
#include <QVector>
#include <QtGlobal>
#include <algorithm>
#include <cmath>

// Two tightly adjacent independent columns. A press adds one authored glyph
// at the bottom of one column and shifts earlier glyphs up exactly one row.
// Positions ease into their new rows; after typing stops the stream drifts
// past the clock and fades. No real key values or text are stored.
class MatrixRain {
public:
    static constexpr int CanvasWidth = 480;
    static constexpr int CanvasHeight = 320;
    static constexpr int Cell = 23;
    static constexpr int FirstX = 398;
    static constexpr int BaseY = 261;
    static constexpr int VanishY = 54;

    void addPress(qint64 nowMs) {
        const int column = int(sequence_ % 2);
        const unsigned glyph = unsigned((sequence_ * 7 + (column ? 9 : 0)) % 24);
        ++sequence_;
        for (Drop &drop : drops_) {
            if (drop.column == column)
                drop.targetY -= Cell;
        }
        drops_.append({column, glyph, qreal(BaseY), qreal(BaseY)});
        if (lastTick_ < 0) lastTick_ = nowMs;
        changed_ = true;
    }

    // Call on every ~40ms GUI tick. Typing controls the stop/start of the
    // gradual upward drift, not the creation of individual symbols.
    bool advance(qint64 nowMs, bool typing) {
        const qint64 dtMs = lastTick_ < 0 ? 0 :
            std::clamp<qint64>(nowMs - lastTick_, 0, 100);
        lastTick_ = nowMs;
        bool dirty = changed_;
        changed_ = false;

        const qreal smooth = std::min(1.0, dtMs / 110.0);
        const qreal drift = typing ? 0.0 : dtMs * 0.092;
        for (Drop &drop : drops_) {
            drop.targetY -= drift;
            const qreal next = drop.y + (drop.targetY - drop.y) * smooth;
            if (std::abs(next - drop.y) > 0.02) dirty = true;
            drop.y = next;
            if (drift > 0) dirty = true;
        }
        const auto oldCount = drops_.size();
        drops_.erase(std::remove_if(drops_.begin(), drops_.end(),
            [](const Drop &d) { return d.y + Cell < VanishY; }), drops_.end());
        return dirty || drops_.size() != oldCount;
    }

    [[nodiscard]] bool active() const { return !drops_.isEmpty(); }
    [[nodiscard]] int glyphCount() const { return drops_.size(); }

    QImage image(QColor phosphor = QColor("#B8FFBF"), int brightness = 100) const {
        QImage layer(CanvasWidth, CanvasHeight,
                     QImage::Format_ARGB32_Premultiplied);
        layer.fill(Qt::transparent);
        QPainter p(&layer);
        p.setRenderHint(QPainter::Antialiasing, true);
        for (const Drop &drop : drops_) {
            const qreal fade = std::clamp((drop.y - VanishY) / 80.0, 0.0, 1.0);
            if (fade <= 0.0) continue;
            QColor color(phosphor);
            color.setAlphaF(fade * 0.98 *
                            std::clamp(brightness, 0, 100) / 100.0);
            japanese::drawGlyph(p,
                QRectF(FirstX + drop.column * Cell, drop.y, Cell, Cell),
                drop.glyph, color);
        }
        return layer;
    }

private:
    struct Drop {
        int column = 0;
        unsigned glyph = 0;
        qreal y = BaseY;
        qreal targetY = BaseY;
    };
    QVector<Drop> drops_;
    quint64 sequence_ = 0;
    qint64 lastTick_ = -1;
    bool changed_ = false;
};
