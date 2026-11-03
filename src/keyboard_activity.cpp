#include "keyboard_activity.hpp"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <array>
#include <vector>
#include <algorithm>
#include <utility>
#include <cstring>

struct KeyboardActivity::Impl {
    Display *display = nullptr;
    Window root = 0;
    KeyCode shortcut = 0;
    std::array<char, 32> previous{};
    std::vector<KeyCode> modifiers;
    std::function<void()> calibration;
    qint64 lastKeyTime = -1000000;

    explicit Impl(std::function<void()> onCalibrate) : calibration(std::move(onCalibrate)) {
        display = XOpenDisplay(nullptr);
        if (!display) return;
        root = DefaultRootWindow(display);
        shortcut = XKeysymToKeycode(display, XK_c);

        for (KeySym sym : {XK_Shift_L, XK_Shift_R, XK_Control_L, XK_Control_R,
                           XK_Alt_L, XK_Alt_R, XK_Super_L, XK_Super_R,
                           XK_Meta_L, XK_Meta_R, XK_Caps_Lock}) {
            const KeyCode key = XKeysymToKeycode(display, sym);
            if (key) modifiers.push_back(key);
        }

        if (shortcut) {
            for (unsigned int mods : {0u, unsigned(LockMask),
                                      unsigned(Mod2Mask), unsigned(LockMask | Mod2Mask)}) {
                XGrabKey(display, shortcut, Mod4Mask | ShiftMask | mods,
                         root, True, GrabModeAsync, GrabModeAsync);
            }
            XFlush(display);
        }
        XQueryKeymap(display, previous.data());
    }

    ~Impl() {
        if (!display) return;
        if (shortcut) {
            for (unsigned int mods : {0u, unsigned(LockMask),
                                      unsigned(Mod2Mask), unsigned(LockMask | Mod2Mask)}) {
                XUngrabKey(display, shortcut, Mod4Mask | ShiftMask | mods, root);
            }
        }
        XCloseDisplay(display);
    }

    void poll(qint64 now) {
        if (!display) return;
        while (XPending(display) > 0) {
            XEvent e{};
            XNextEvent(display, &e);
            if (e.type == KeyPress && e.xkey.keycode == shortcut &&
                (e.xkey.state & (Mod4Mask | ShiftMask)) == (Mod4Mask | ShiftMask)) {
                calibration();
            }
        }

        std::array<char, 32> current{};
        XQueryKeymap(display, current.data());
        for (unsigned int key = 8; key < 256; ++key) {
            if (std::find(modifiers.begin(), modifiers.end(), key) != modifiers.end()) continue;
            const unsigned int byte = key / 8, mask = 1u << (key % 8);
            const unsigned int pressed = static_cast<unsigned char>(current[byte]) & mask;
            const unsigned int previous = static_cast<unsigned char>(this->previous[byte]) & mask;
            if (pressed && !previous) {
                lastKeyTime = now;
                break;
            }
        }
        previous = current;
    }
};

KeyboardActivity::KeyboardActivity(std::function<void()> callback)
    : impl_(std::make_unique<Impl>(std::move(callback))) {}

KeyboardActivity::~KeyboardActivity() = default;

void KeyboardActivity::poll(qint64 now) { impl_->poll(now); }

bool KeyboardActivity::recentlyTyping(qint64 now) const {
    return impl_->display && (now - impl_->lastKeyTime < 950);
}
