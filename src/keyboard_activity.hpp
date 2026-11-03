#pragma once
#include <QtGlobal>
#include <functional>
#include <memory>

// X11 only. Detects "a key was newly pressed" without recording keys or text.
// A single global shortcut opens calibration; no root privileges required.
class KeyboardActivity {
public:
    explicit KeyboardActivity(std::function<void()> calibrationCallback);
    ~KeyboardActivity();
    KeyboardActivity(const KeyboardActivity &) = delete;
    KeyboardActivity &operator=(const KeyboardActivity &) = delete;

    void poll(qint64 monotonicMs);
    bool recentlyTyping(qint64 monotonicMs) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
