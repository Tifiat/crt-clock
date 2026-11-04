#pragma once

#include "perspective.hpp"
#include <QImage>
#include <QPointF>
#include <QPolygonF>
#include <QSize>
#include <QTransform>
#include <QtGlobal>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

// Fast software renderer for a *fixed* eight-handle, four-cell projective
// surface. Qt's quadToQuad is used ONCE per geometry change. For animation
// frames we reuse the inverse map per destination pixel and perform bilinear
// sampling of the existing premultiplied ARGB images. No change to how the
// eight calibration handles are stored or where they are drawn.
namespace perspective {

class RasterCache {
public:
    // True only when the output mapping actually needs rebuilding.
    bool prepare(QSize viewport, const Handles &absolute, QPointF origin) {
        if (prepared_ && viewport == viewport_ && absolute == handles_ &&
            origin == origin_)
            return false;
        prepared_ = true;
        viewport_ = viewport;
        handles_ = absolute;
        origin_ = origin;
        samples_.clear();
        previousBase_ = {};
        previousMatrix_ = {};
        lastUpdatedPixels_ = 0;
        ++buildCount_;

        if (viewport.isEmpty()) return true;

        Handles h = absolute;
        for (auto &point : h) point -= origin;
        const QPointF mid = center(h);
        constexpr qreal w = 480, height = 320;
        const QPointF tl(0,0), tr(w,0), br(w,height), bl(0,height);
        const QPointF tm(w/2,0), rm(w,height/2), bm(w/2,height), lm(0,height/2);
        const QPointF c(w/2,height/2);
        const std::array<QPolygonF,4> input{{
            QPolygonF{tl,tm,c,lm}, QPolygonF{tm,tr,rm,c},
            QPolygonF{c,rm,br,bm}, QPolygonF{lm,c,bm,bl}
        }};
        const std::array<QPolygonF,4> output{{
            QPolygonF{h[0],h[4],mid,h[7]},
            QPolygonF{h[4],h[1],h[5],mid},
            QPolygonF{mid,h[5],h[2],h[6]},
            QPolygonF{h[7],mid,h[6],h[3]}
        }};

        // Most mapped widgets occupy only a small portion of a laptop
        // display. Store only destination pixels inside the four quads.
        const int width = viewport.width(), rows = viewport.height();
        // Cells can share an edge pixel. The original painter's last cell
        // wins, so retain a single (last) inverse sample per destination.
        std::vector<int> sampleIndex(static_cast<size_t>(width)*rows, -1);
        for (int cell = 0; cell < 4; ++cell) {
            QTransform forward;
            if (!QTransform::quadToQuad(input[cell], output[cell], forward))
                continue;
            bool invertible = false;
            const QTransform inverse = forward.inverted(&invertible);
            if (!invertible) continue;
            const QRect rect = output[cell].boundingRect().toAlignedRect();
            const int x0 = std::max(0, rect.left());
            const int x1 = std::min(width - 1, rect.right());
            const int y0 = std::max(0, rect.top());
            const int y1 = std::min(rows - 1, rect.bottom());
            for (int y = y0; y <= y1; ++y) {
                for (int x = x0; x <= x1; ++x) {
                    const QPointF middle(x + .5, y + .5);
                    if (!output[cell].containsPoint(middle, Qt::WindingFill))
                        continue;
                    const QPointF uv = inverse.map(middle);
                    const double sx = uv.x() - .5;
                    const double sy = uv.y() - .5;
                    if (!std::isfinite(sx) || !std::isfinite(sy))
                        continue;
                    if (sx < -1.0 || sx > w || sy < -1.0 || sy > height)
                        continue;
                    const int ix = static_cast<int>(std::floor(sx));
                    const int iy = static_cast<int>(std::floor(sy));
                    const int fx = std::clamp(static_cast<int>(std::lround(
                        (sx - ix) * 256.0)), 0, 256);
                    const int fy = std::clamp(static_cast<int>(std::lround(
                        (sy - iy) * 256.0)), 0, 256);
                    const int dst = y * width + x;
                    Sample sample{dst, ix, iy,
                                  static_cast<uint16_t>(fx),
                                  static_cast<uint16_t>(fy)};
                    if (sampleIndex[dst] == -1) {
                        sampleIndex[dst] = static_cast<int>(samples_.size());
                        samples_.push_back(sample);
                    } else {
                        samples_[sampleIndex[dst]] = sample;
                    }
                }
            }
        }
        return true;
    }

    [[nodiscard]] QImage render(const QImage &base, const QImage &matrix = {}) const {
        QImage output(viewport_, QImage::Format_ARGB32_Premultiplied);
        if (output.isNull()) return output;
        output.fill(Qt::transparent);
        if (!prepared_ || base.isNull()) return output;

        const QImage convertedBase = base.format() == QImage::Format_ARGB32_Premultiplied
            ? base : base.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        const bool hasMatrix = !matrix.isNull();
        const QImage convertedMatrix = !hasMatrix ? QImage() :
            matrix.format() == QImage::Format_ARGB32_Premultiplied
                ? matrix : matrix.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        auto *pixels = reinterpret_cast<uint32_t *>(output.bits());
        // viewport_width pixels per row, output generated tightly packed by Qt
        const int destinationStride = output.bytesPerLine() / 4;
        const int width = viewport_.width();

        for (const Sample &sample : samples_) {
            const uint32_t baseColor = bilinear(convertedBase, sample);
            const uint32_t color = hasMatrix ?
                over(bilinear(convertedMatrix, sample), baseColor) : baseColor;
            const int y = sample.dst / width;
            const int x = sample.dst % width;
            pixels[y * destinationStride + x] = color;
        }
        return output;
    }

    // The output is owned by the window: don't allocate or clear it on
    // ordinary animation frames. Only source pixels that changed since
    // the last projection need their corresponding destination samples
    // recalculated. This produces exactly the same image as render().
    void renderInto(QImage &output, const QImage &base,
                    const QImage &matrix = {}) {
        const bool fresh = output.size() != viewport_ ||
                           output.format() != QImage::Format_ARGB32_Premultiplied ||
                           previousBase_.isNull();
        if (fresh) {
            output = QImage(viewport_, QImage::Format_ARGB32_Premultiplied);
            if (output.isNull()) {
                lastUpdatedPixels_ = 0;
                return;
            }
            output.fill(Qt::transparent);
        }
        if (!prepared_ || base.isNull()) {
            output.fill(Qt::transparent);
            previousBase_ = {};
            previousMatrix_ = {};
            lastUpdatedPixels_ = 0;
            return;
        }

        const QImage currentBase =
            base.format() == QImage::Format_ARGB32_Premultiplied
                ? base : base.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        const QImage currentMatrix = matrix.isNull() ? QImage() :
            matrix.format() == QImage::Format_ARGB32_Premultiplied
                ? matrix : matrix.convertToFormat(QImage::Format_ARGB32_Premultiplied);

        QRect changed;
        if (!fresh) {
            changed = changedBounds(previousBase_, currentBase);
            const QRect matrixChange = changedBounds(previousMatrix_, currentMatrix);
            if (changed.isEmpty()) changed = matrixChange;
            else if (!matrixChange.isEmpty()) changed = changed.united(matrixChange);
        }
        lastUpdatedPixels_ = 0;
        if (!fresh && changed.isEmpty()) {
            previousBase_ = currentBase;
            previousMatrix_ = currentMatrix;
            return;
        }

        const int left = changed.left(), right = changed.right();
        const int top = changed.top(), bottom = changed.bottom();
        const int width = viewport_.width();
        const int dstStride = output.bytesPerLine()/4;
        auto *pixels = reinterpret_cast<uint32_t *>(output.bits());
        for (const Sample &sample : samples_) {
            // A bilinear sample reads the 2x2 neighborhood (x..x+1,
            // y..y+1), including adjacent samples at change boundaries.
            if (!fresh && (sample.x > right || sample.x+1 < left ||
                           sample.y > bottom || sample.y+1 < top))
                continue;
            const uint32_t baseColor = bilinear(currentBase, sample);
            const uint32_t color = currentMatrix.isNull() ? baseColor :
                over(bilinear(currentMatrix, sample), baseColor);
            pixels[(sample.dst / width)*dstStride + sample.dst % width] = color;
            ++lastUpdatedPixels_;
        }
        previousBase_ = currentBase;
        previousMatrix_ = currentMatrix;
    }

    [[nodiscard]] quint64 buildCount() const { return buildCount_; }
    [[nodiscard]] qsizetype mappedPixels() const { return samples_.size(); }
    [[nodiscard]] qsizetype lastUpdatedPixels() const { return lastUpdatedPixels_; }

private:
    // The source image is 480x320, so a row compare is cheap compared with
    // bilinear projection of the whole destination. Bounds include the
    // entire changed region; an unchanged row costs one memcmp.
    static QRect changedBounds(const QImage &previous, const QImage &current) {
        if (previous.cacheKey() == current.cacheKey()) return {};
        if (previous.size() != current.size() ||
            previous.format() != current.format())
            return QRect(0, 0, std::max(previous.width(), current.width()),
                         std::max(previous.height(), current.height()));
        int left = current.width(), right = -1, top = current.height(), bottom = -1;
        const int width = current.width();
        for (int y = 0; y < current.height(); ++y) {
            const auto *oldRow = reinterpret_cast<const uint32_t *>(
                previous.constScanLine(y));
            const auto *newRow = reinterpret_cast<const uint32_t *>(
                current.constScanLine(y));
            if (std::memcmp(oldRow, newRow, size_t(width)*sizeof(uint32_t)) == 0)
                continue;
            int first = 0, last = width-1;
            while (first < width && oldRow[first] == newRow[first]) ++first;
            while (last >= first && oldRow[last] == newRow[last]) --last;
            left = std::min(left, first);
            right = std::max(right, last);
            top = std::min(top, y);
            bottom = y;
        }
        if (right < 0) return {};
        return QRect(QPoint(left, top), QPoint(right, bottom));
    }

    struct Sample {
        int dst;
        int x;
        int y;
        uint16_t fx;
        uint16_t fy;
    };

    static uint32_t at(const QImage &img, int x, int y) {
        if (x < 0 || x >= img.width() || y < 0 || y >= img.height())
            return 0;
        const auto *row = reinterpret_cast<const uint32_t *>(img.constScanLine(y));
        return row[x];
    }

    static uint32_t bilinear(const QImage &img, const Sample &s) {
        if (img.isNull()) return 0;
        const uint32_t c00 = at(img, s.x, s.y);
        const uint32_t c10 = at(img, s.x+1, s.y);
        const uint32_t c01 = at(img, s.x, s.y+1);
        const uint32_t c11 = at(img, s.x+1, s.y+1);
        const uint32_t wx = s.fx, wy = s.fy;
        const uint32_t w00 = (256-wx)*(256-wy);
        const uint32_t w10 = wx*(256-wy);
        const uint32_t w01 = (256-wx)*wy;
        const uint32_t w11 = wx*wy;
        auto channel = [&](int shift) -> uint32_t {
            const uint32_t value =
                ((c00 >> shift)&255u)*w00 +
                ((c10 >> shift)&255u)*w10 +
                ((c01 >> shift)&255u)*w01 +
                ((c11 >> shift)&255u)*w11;
            return std::min(255u, (value + 32768u) >> 16);
        };
        return (channel(24)<<24) | (channel(16)<<16) |
               (channel(8)<<8) | channel(0);
    }

    // Premultiplied ARGB source-over, matching QPainter's default
    // CompositionMode_SourceOver (before scanlines are applied).
    static uint32_t over(uint32_t top, uint32_t bottom) {
        const uint32_t alpha = top >> 24;
        if (alpha == 0) return bottom;
        if (alpha == 255) return top;
        const uint32_t k = 255-alpha;
        auto blend = [&](int shift) -> uint32_t {
            return std::min(255u, ((top >> shift)&255u) +
                              (((bottom >> shift)&255u)*k+127u)/255u);
        };
        return (blend(24)<<24) | (blend(16)<<16) |
               (blend(8)<<8) | blend(0);
    }

    bool prepared_ = false;
    QSize viewport_;
    Handles handles_{};
    QPointF origin_;
    std::vector<Sample> samples_;
    QImage previousBase_;
    QImage previousMatrix_;
    qsizetype lastUpdatedPixels_ = 0;
    quint64 buildCount_ = 0;
};

} // namespace perspective
