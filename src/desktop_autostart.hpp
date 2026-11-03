#pragma once

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QString>

// Desktop Session Autostart Specification (XDG), per-user and opt-in.
// Avoid systemd user services for a GUI application: XFCE loads the .desktop
// entry only after the graphical session and display are available.
namespace desktop {
inline QString autostartPath() {
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + QStringLiteral("/autostart/crt-clock.desktop");
}
inline bool autostartEnabled() {
    return QFileInfo::exists(autostartPath());
}
inline bool setAutostart(bool enabled) {
    const QString path = autostartPath();
    if (!enabled) return !QFileInfo::exists(path) || QFile::remove(path);
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) return false;

    static constexpr const char entry[] =
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=CRT Clock\n"
        "Comment=Perspective CRT desktop clock\n"
        "Exec=crt-clock\n"
        "TryExec=crt-clock\n"
        "Terminal=false\n"
        "X-GNOME-Autostart-enabled=true\n";
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    if (file.write(entry, sizeof(entry)-1) != sizeof(entry)-1) return false;
    return file.commit();
}
} // namespace desktop
