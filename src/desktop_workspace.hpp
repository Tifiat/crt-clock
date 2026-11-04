#pragma once

#include <QWidget>
#include <utility>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

// EWMH: 0xFFFFFFFF is the special "all virtual desktops" value.
// XFWM4 can read the property at initial map; sending ClientMessages after
// mapping additionally requests sticky state for an already-managed window.
namespace desktop {
// Do not change Qt window flags after show(): Qt hides/reparents the widget
// and can recreate its X11 window ID, losing XFWM4 workspace state.
inline Qt::WindowFlags widgetWindowFlags() {
    return Qt::Window | Qt::FramelessWindowHint;
}

// Update stacking through the WM without changing Qt flags or X11 window ID.
// Calibration needs focus and ABOVE; the desktop widget needs BELOW.
inline void requestStacking(WId nativeWindow, bool editing) {
    Display *dpy = XOpenDisplay(nullptr);
    if (!dpy) return;
    const Window window = static_cast<Window>(nativeWindow);
    const Window root = DefaultRootWindow(dpy);
    const Atom state = XInternAtom(dpy, "_NET_WM_STATE", False);
    const Atom above = XInternAtom(dpy, "_NET_WM_STATE_ABOVE", False);
    const Atom below = XInternAtom(dpy, "_NET_WM_STATE_BELOW", False);

    for (const auto [atom, add] : {
             std::pair<Atom, bool>{above, editing},
             std::pair<Atom, bool>{below, !editing}}) {
        XEvent request{};
        request.xclient.type = ClientMessage;
        request.xclient.window = window;
        request.xclient.message_type = state;
        request.xclient.format = 32;
        request.xclient.data.l[0] = add ? 1 : 0; // ADD / REMOVE
        request.xclient.data.l[1] = static_cast<long>(atom);
        request.xclient.data.l[3] = 1; // application
        XSendEvent(dpy, root, False,
                   SubstructureRedirectMask | SubstructureNotifyMask, &request);
    }
    XFlush(dpy);
    XCloseDisplay(dpy);
}

inline void keepOnEveryWorkspace(WId nativeWindow) {
    Display *dpy = XOpenDisplay(nullptr);
    if (!dpy) return;
    const Window window = static_cast<Window>(nativeWindow);
    const Window root = DefaultRootWindow(dpy);
    const Atom desktop = XInternAtom(dpy, "_NET_WM_DESKTOP", False);
    const Atom cardinal = XInternAtom(dpy, "CARDINAL", False);
    const Atom state = XInternAtom(dpy, "_NET_WM_STATE", False);
    const Atom sticky = XInternAtom(dpy, "_NET_WM_STATE_STICKY", False);

    unsigned long allDesktops = 0xFFFFFFFFUL;
    // Property assignment is for the pre-map (withdrawn) window only.
    // Once mapped, send ClientMessage and let XFWM4 OWN this property.
    // Writing it directly after mapping previously made readback report
    // 0xFFFFFFFF even when the WM had not honored the request.
    XWindowAttributes attributes{};
    const bool isMapped = XGetWindowAttributes(dpy, window, &attributes) &&
                          attributes.map_state != IsUnmapped;
    if (!isMapped)
        XChangeProperty(dpy, window, desktop, cardinal, 32, PropModeReplace,
                        reinterpret_cast<const unsigned char *>(&allDesktops), 1);

    XEvent request{};
    request.xclient.type = ClientMessage;
    request.xclient.window = window;
    request.xclient.message_type = desktop;
    request.xclient.format = 32;
    request.xclient.data.l[0] = static_cast<long>(allDesktops);
    request.xclient.data.l[1] = 1; // source: application
    XSendEvent(dpy, root, False,
               SubstructureRedirectMask | SubstructureNotifyMask, &request);

    // Some XFWM configurations require the sticky state in addition to the
    // all-desktops property, especially for Qt::Tool windows.
    XEvent stickyRequest{};
    stickyRequest.xclient.type = ClientMessage;
    stickyRequest.xclient.window = window;
    stickyRequest.xclient.message_type = state;
    stickyRequest.xclient.format = 32;
    stickyRequest.xclient.data.l[0] = 1; // _NET_WM_STATE_ADD
    stickyRequest.xclient.data.l[1] = static_cast<long>(sticky);
    stickyRequest.xclient.data.l[2] = 0;
    stickyRequest.xclient.data.l[3] = 1; // application
    XSendEvent(dpy, root, False,
               SubstructureRedirectMask | SubstructureNotifyMask, &stickyRequest);
    // A normal Qt::Window stays out of the taskbar and workspace pager
    // through EWMH hints rather than relying on Qt::Tool window semantics.
    XEvent skipRequest{};
    skipRequest.xclient.type = ClientMessage;
    skipRequest.xclient.window = window;
    skipRequest.xclient.message_type = state;
    skipRequest.xclient.format = 32;
    skipRequest.xclient.data.l[0] = 1; // _NET_WM_STATE_ADD
    skipRequest.xclient.data.l[1] = static_cast<long>(
        XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR", False));
    skipRequest.xclient.data.l[2] = static_cast<long>(
        XInternAtom(dpy, "_NET_WM_STATE_SKIP_PAGER", False));
    skipRequest.xclient.data.l[3] = 1;
    XSendEvent(dpy, root, False,
               SubstructureRedirectMask | SubstructureNotifyMask, &skipRequest);
    XFlush(dpy);
    XCloseDisplay(dpy);
}

// Read the actual desktop assignment back from XFWM4.
// _NET_WM_STATE_STICKY alone is NOT proof of "all workspaces": it controls
// viewport scrolling, while _NET_WM_DESKTOP=0xFFFFFFFF controls visibility
// across distinct XFCE workspaces. Checking (desktop || sticky) masked a real
// regression when XFWM4 kept STICKY but moved the widget to one desktop.
constexpr bool isAllDesktopsValue(unsigned long desktop) {
    return desktop == 0xFFFFFFFFUL;
}
inline bool isOnEveryWorkspace(WId nativeWindow) {
    Display *dpy = XOpenDisplay(nullptr);
    if (!dpy) return false;
    const Window window = static_cast<Window>(nativeWindow);
    bool assignedEverywhere = false;

    const Atom desktop = XInternAtom(dpy, "_NET_WM_DESKTOP", False);
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long items = 0, bytesAfter = 0;
    unsigned char *data = nullptr;
    if (XGetWindowProperty(dpy, window, desktop, 0, 1, False, XA_CARDINAL,
                           &actualType, &actualFormat, &items, &bytesAfter, &data)
        == Success) {
        assignedEverywhere = actualType == XA_CARDINAL && actualFormat == 32 &&
                             items >= 1 && data != nullptr &&
                             isAllDesktopsValue(*reinterpret_cast<unsigned long *>(data));
    }
    if (data) XFree(data);

    XCloseDisplay(dpy);
    return assignedEverywhere;
}
} // namespace desktop
