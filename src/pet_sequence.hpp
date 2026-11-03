#pragma once

#include <QRandomGenerator>
#include <QVector>
#include <algorithm>
#include <utility>

namespace pet {

// Shuffle-bag: every action is selected exactly once per cycle.
// The following cycle is independently shuffled. This logic knows nothing
// about poses, durations, input sources, or Qt painting.
class ShuffleBag {
public:
    explicit ShuffleBag(int count) : count_(count) {}

    int next() {
        if (offset_ >= order_.size()) refill();
        return order_[offset_++];
    }

private:
    void refill() {
        order_.clear();
        for (int i=0; i<count_; ++i) order_.append(i);
        // Fisher-Yates with Qt's built-in random generator.
        for (int i=order_.size()-1; i>0; --i) {
            const int j = QRandomGenerator::global()->bounded(i+1);
            std::swap(order_[i], order_[j]);
        }
        offset_ = 0;
    }

    int count_;
    QVector<int> order_;
    qsizetype offset_ = 0;
};
} // namespace pet
