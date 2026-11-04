#include <QApplication>
#include <QEventLoop>
#include <QRect>
#include <QTimer>
#include <QWidget>
#include <iostream>
#include <X11/Xatom.h>
// Must follow all Qt headers because Xlib exports None as a macro.
#include "desktop_workspace.hpp"

static void require(bool ok, const char *message) {
    if (!ok) {
        std::cerr << "XFWM4 integration FAILED: " << message << '\n';
        std::exit(1);
    }
}

static void waitForWm(int milliseconds) {
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}

static void switchWorkspace(int index) {
    Display *dpy = XOpenDisplay(nullptr);
    require(dpy != nullptr, "cannot connect to X11");
    XEvent event{};
    event.xclient.type = ClientMessage;
    event.xclient.window = DefaultRootWindow(dpy);
    event.xclient.message_type = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
    event.xclient.format = 32;
    event.xclient.data.l[0] = index;
    event.xclient.data.l[1] = CurrentTime;
    XSendEvent(dpy, DefaultRootWindow(dpy), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &event);
    XFlush(dpy);
    XCloseDisplay(dpy);
    waitForWm(280);
}

static bool isViewable(WId id) {
    Display *dpy = XOpenDisplay(nullptr);
    require(dpy != nullptr, "cannot connect to X11");
    XWindowAttributes attributes{};
    const bool visible = XGetWindowAttributes(
        dpy, static_cast<Window>(id), &attributes) &&
        attributes.map_state == IsViewable;
    XCloseDisplay(dpy);
    return visible;
}

static void dumpProperty(Display *dpy, Window w, const char *name) {
    const Atom atom = XInternAtom(dpy, name, False);
    Atom type = None;
    int format = 0;
    unsigned long count = 0, leftover = 0;
    unsigned char *data = nullptr;
    const int rc = XGetWindowProperty(dpy, w, atom, 0, 64, False, AnyPropertyType,
                                      &type, &format, &count, &leftover, &data);
    std::cerr << name << ": rc=" << rc << " format=" << format
              << " count=" << count;
    if (format == 32 && data) {
        const auto *values = reinterpret_cast<const unsigned long *>(data);
        for (unsigned long i = 0; i < count; ++i) {
            char *atomName = nullptr;
            if (type == XA_ATOM)
                atomName = XGetAtomName(dpy, static_cast<Atom>(values[i]));
            std::cerr << " [" << std::hex << values[i] << std::dec
                      << (atomName ? atomName : "") << "]";
            if (atomName) XFree(atomName);
        }
    }
    std::cerr << '\n';
    if (data) XFree(data);
}

static void dumpWindow(WId id) {
    Display *dpy = XOpenDisplay(nullptr);
    if (!dpy) return;
    std::cerr << "window=" << std::hex << static_cast<unsigned long>(id)
              << std::dec << '\n';
    dumpProperty(dpy, static_cast<Window>(id), "_NET_WM_DESKTOP");
    dumpProperty(dpy, static_cast<Window>(id), "_NET_WM_STATE");
    dumpProperty(dpy, DefaultRootWindow(dpy), "_NET_NUMBER_OF_DESKTOPS");
    dumpProperty(dpy, DefaultRootWindow(dpy), "_NET_CLIENT_LIST");
    XCloseDisplay(dpy);
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QWidget widget(nullptr, desktop::widgetWindowFlags());
    widget.setAttribute(Qt::WA_TranslucentBackground);
    widget.setAttribute(Qt::WA_ShowWithoutActivating);

    // Same production mode-switch function as ClockWindow::applyMode().
    desktop::applyWindowMode(widget, false, QRect(50, 50, 280, 170));
    const WId initialXid = widget.winId();
    for (int attempt = 0; attempt < 5; ++attempt) {
        waitForWm(220);
        desktop::keepOnEveryWorkspace(initialXid);
    }
    if (!desktop::isOnEveryWorkspace(initialXid)) dumpWindow(initialXid);
    require(desktop::isOnEveryWorkspace(initialXid),
            "first map is not visible on all XFWM4 workspaces");

    desktop::applyWindowMode(widget, true, QRect(0, 0, 900, 650));
    require(widget.winId() == initialXid,
            "entering calibration recreated the X11 window");
    for (int attempt = 0; attempt < 4; ++attempt) {
        waitForWm(220);
        desktop::keepOnEveryWorkspace(widget.winId());
    }

    // Simulates pressing Escape: geometry and focus/stacking are restored
    // without calling hide() or setWindowFlags().
    desktop::applyWindowMode(widget, false, QRect(50, 50, 280, 170));
    require(widget.winId() == initialXid,
            "Escape from calibration recreated the X11 window");
    for (int attempt = 0; attempt < 5; ++attempt) {
        waitForWm(240);
        desktop::keepOnEveryWorkspace(widget.winId());
    }
    require(desktop::isOnEveryWorkspace(initialXid),
            "Escape discarded the all-workspaces assignment");

    // A window may have a plausible property and still be hidden by a WM
    // on another desktop. Actually switch workspaces and check map state.
    for (int index = 0; index < 4; ++index) {
        switchWorkspace(index);
        require(desktop::isOnEveryWorkspace(initialXid),
                "workspace switch changed the desktop property");
        require(isViewable(initialXid),
                "widget disappeared when switching to another workspace");
    }

    std::cout << "XFWM4 workspace integration passed; XID remained "
              << static_cast<unsigned long>(initialXid) << '\n';
}
