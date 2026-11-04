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
        lastPicked_ = order_[offset_++];
        return lastPicked_;
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
        // An independently shuffled new cycle may otherwise start with
        // the previous cycle's final action. Avoid a back-to-back repeat
        // while preserving one occurrence of every action in each cycle.
        if (order_.size() > 1 && order_.front() == lastPicked_) {
            const int alternate = QRandomGenerator::global()->bounded(1, int(order_.size()));
            std::swap(order_[0], order_[alternate]);
        }
        offset_ = 0;
    }

    int count_;
    QVector<int> order_;
    qsizetype offset_ = 0;
    int lastPicked_ = -1;
};
} // namespace pet
