#pragma once

#include <QWidget>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

// EWMH: 0xFFFFFFFF is the special "all virtual desktops" value.
// XFWM4 can read the property at initial map; sending ClientMessages after
// mapping additionally requests sticky state for an already-managed window.
namespace desktop {
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
    XFlush(dpy);
    XCloseDisplay(dpy);
}
} // namespace desktop
