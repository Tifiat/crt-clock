#include <QApplication>
#include <QColor>
#include <QDateTime>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDialog>
#include <QFormLayout>
#include <QSlider>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QColorDialog>
#include <QList>
#include "crt_style.hpp"
#include "pet_poses.hpp"
#include <QDir>
#include <QFileInfo>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QImage>
#include <QKeyEvent>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMouseEvent>
#include <QPainter>
#include <QPolygonF>
#include <QRandomGenerator>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QTransform>
#include <QWidget>
#include <algorithm>
#include <array>
#include <unistd.h>

namespace {
constexpr int W = 480;
constexpr int H = 320;

QString configFile() {
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + "/crt-clock/config.ini";
}

struct Settings {
    std::array<QPointF, 4> quad;
    QString font = QStringLiteral("IBM Plex Mono");
    QString pet = QStringLiteral("(づ｡◕‿‿◕｡)づ");
    QColor green = QColor("#B0FFBB");
    QColor dim = QColor("#70BB8A");
    QColor background = QColor("#092419");
    int backgroundAlpha = 35;
    bool frame = false;
    bool scanlines = true;
    int glow = 44;
    int scanlineStrength = 24;
    int brightness = 84;
    bool musicResponse = true;

    explicit Settings(QRect screen) {
        const qreal x = screen.left() + screen.width() * .65;
        const qreal y = screen.top() + screen.height() * .31;
        const qreal w = screen.width() * .18;
        const qreal h = screen.height() * .27;
        quad = {QPointF(x, y + 8), QPointF(x + w, y),
                QPointF(x + w - 10, y + h), QPointF(x + 8, y + h + 7)};
    }

    void load() {
        QSettings s(configFile(), QSettings::IniFormat);
        for (int i = 0; i < 4; ++i) {
            quad[i].setX(s.value(QString("quad/x%1").arg(i), quad[i].x()).toDouble());
            quad[i].setY(s.value(QString("quad/y%1").arg(i), quad[i].y()).toDouble());
        }
        font = s.value("appearance/font", font).toString();
        pet = s.value("appearance/pet", pet).toString();
        green = QColor(s.value("appearance/primary", green.name()).toString());
        dim = QColor(s.value("appearance/secondary", dim.name()).toString());
        background = QColor(s.value("appearance/background", background.name()).toString());
        backgroundAlpha = std::clamp(s.value("appearance/background_alpha", backgroundAlpha).toInt(), 0, 255);
        frame = s.value("appearance/border", frame).toBool();
        scanlines = s.value("appearance/scanlines", scanlines).toBool();
        glow = std::clamp(s.value("appearance/glow", glow).toInt(), 0, 100);
        scanlineStrength = std::clamp(s.value("appearance/scanline_strength", scanlineStrength).toInt(), 0, 100);
        brightness = std::clamp(s.value("appearance/brightness", brightness).toInt(), 25, 100);
        musicResponse = s.value("pet/music_response", musicResponse).toBool();
        if (!green.isValid()) green = QColor("#B0FFBB");
        if (!dim.isValid()) dim = QColor("#70BB8A");
        if (!background.isValid()) background = QColor("#092419");
        if (pet.isEmpty()) pet = QStringLiteral("(づ｡◕‿‿◕｡)づ");
    }

    void save() const {
        QDir().mkpath(QFileInfo(configFile()).absolutePath());
        QSettings s(configFile(), QSettings::IniFormat);
        for (int i = 0; i < 4; ++i) {
            s.setValue(QString("quad/x%1").arg(i), quad[i].x());
            s.setValue(QString("quad/y%1").arg(i), quad[i].y());
        }
        s.setValue("appearance/font", font);
        s.setValue("appearance/pet", pet);
        s.setValue("appearance/primary", green.name());
        s.setValue("appearance/secondary", dim.name());
        s.setValue("appearance/background", background.name());
        s.setValue("appearance/background_alpha", backgroundAlpha);
        s.setValue("appearance/border", frame);
        s.setValue("appearance/scanlines", scanlines);
        s.setValue("appearance/glow", glow);
        s.setValue("appearance/scanline_strength", scanlineStrength);
        s.setValue("appearance/brightness", brightness);
        s.setValue("pet/music_response", musicResponse);
        s.sync();
    }
};

QRect bounds(const Settings &cfg) {
    QPolygonF p;
    for (const auto &c : cfg.quad) p << c;
    return p.boundingRect().adjusted(-12, -12, 12, 12).toAlignedRect();
}

void centered(QPainter &p, QRectF r, const QString &text, QFont font, QColor color) {
    while (font.pixelSize() > 12 && QFontMetricsF(font).horizontalAdvance(text) > r.width() - 8)
        font.setPixelSize(font.pixelSize() - 1);
    p.setFont(font);
    p.setPen(color);
    p.drawText(r, Qt::AlignCenter | Qt::TextSingleLine, text);
}

QImage drawCanvas(const Settings &cfg, int pose) {
    QImage glyphs(W, H, QImage::Format_ARGB32_Premultiplied);
    glyphs.fill(Qt::transparent);
    QPainter text(&glyphs);
    text.setRenderHint(QPainter::Antialiasing, true);
    text.setRenderHint(QPainter::TextAntialiasing, true);

    QFont time(cfg.font);
    time.setPixelSize(97);
    time.setWeight(QFont::Bold);
    time.setHintingPreference(QFont::PreferFullHinting);
    QFont date(cfg.font);
    date.setPixelSize(32);
    date.setWeight(QFont::Medium);
    QFont pet(cfg.font);
    pet.setPixelSize(45);

    const auto now = QDateTime::currentDateTime();
    QColor primary = cfg.green;
    primary.setAlphaF(cfg.brightness / 100.0);
    QColor secondary = cfg.dim;
    secondary.setAlphaF(cfg.brightness / 100.0);

    centered(text, QRectF(12, 27, 456, 125), now.toString("HH:mm"), time, primary);
    centered(text, QRectF(14, 154, 452, 52),
             QLocale(QLocale::English).toString(now.date(), "ddd, dd MMM yyyy").toUpper(),
             date, secondary);
    centered(text, QRectF(10, 217, 460, 82), petPose(cfg.pet, pose), pet, primary);
    text.end();

    return crtComposite(glyphs, cfg.background, cfg.backgroundAlpha, cfg.glow,
                        cfg.scanlines ? cfg.scanlineStrength : 0, cfg.frame, cfg.dim);
}

// Read only the playback state over MPRIS; never capture audio or sound.
bool musicIsPlaying() {
    auto *iface = QDBusConnection::sessionBus().interface();
    if (!iface) return false;
    const QDBusReply<QStringList> services = iface->registeredServiceNames();
    if (!services.isValid()) return false;
    for (const QString &name : services.value()) {
        if (!name.startsWith("org.mpris.MediaPlayer2.")) continue;
        QDBusMessage request = QDBusMessage::createMethodCall(
            name, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get");
        request << QStringLiteral("org.mpris.MediaPlayer2.Player")
                << QStringLiteral("PlaybackStatus");
        QDBusMessage reply = QDBusConnection::sessionBus().call(request, QDBus::Block, 200);
        if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) continue;
        const QVariant status = reply.arguments().first();
        if (status.canConvert<QDBusVariant>()
            && status.value<QDBusVariant>().variant().toString() == "Playing")
            return true;
    }
    return false;
}

class ClockWindow final : public QWidget {
public:
    ClockWindow() : screen_(QGuiApplication::primaryScreen()->geometry()), cfg_(screen_) {
        cfg_.load();
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setMouseTracking(true);
        connect(&tick_, &QTimer::timeout, this, [this] {
            ++seconds_;
            if (cfg_.musicResponse && seconds_ % 5 == 0)
                musicPlaying_ = musicIsPlaying();

            if (cfg_.musicResponse && musicPlaying_) {
                steps_.clear();
                pose_ = 8 + (seconds_ % 3);
            } else if (!steps_.isEmpty()) {
                pose_ = steps_.takeFirst();
            } else if (QRandomGenerator::global()->bounded(14) == 0) {
                const int choice = QRandomGenerator::global()->bounded(4);
                if (choice == 0) steps_ = {1, 0};                      // Blink
                if (choice == 1) steps_ = {3, 0, 3, 0};                // Wave
                if (choice == 2) steps_ = {2, 4, 0};                   // Sway
                if (choice == 3) steps_ = {4, 5, 6, 5, 7, 5, 4, 0};  // Turn, tail wag, return
                pose_ = steps_.takeFirst();
            } else {
                pose_ = 0;
            }
            redraw();
        });
        tick_.start(1000);
        redraw();
    }

    void start() { editing_ = false; applyMode(); }

    void settings() {
        if (settingsWindow_) {
            settingsWindow_->raise();
            settingsWindow_->activateWindow();
            return;
        }
        const Settings before = cfg_;
        auto *dialog = new QDialog(nullptr);
        settingsWindow_ = dialog;
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowTitle("CRT Clock / Settings");
        dialog->resize(370, 380);
        auto *layout = new QVBoxLayout(dialog);
        auto *form = new QFormLayout();
        layout->addLayout(form);

        auto addSlider = [this, dialog, form](const QString &label, int initial,
                                              int lower, int upper, auto setter) {
            auto *slider = new QSlider(Qt::Horizontal, dialog);
            slider->setRange(lower, upper);
            slider->setValue(initial);
            auto *value = new QLabel(QString::number(initial), dialog);
            auto *row = new QWidget(dialog);
            auto *h = new QHBoxLayout(row);
            h->setContentsMargins(0, 0, 0, 0);
            h->addWidget(slider);
            h->addWidget(value);
            form->addRow(label, row);
            connect(slider, &QSlider::valueChanged, dialog, [this, setter, value](int n) {
                setter(cfg_, n);
                value->setText(QString::number(n));
                redraw();
            });
        };

        addSlider("Phosphor glow", cfg_.glow, 0, 100,
                  [](Settings &s, int x) { s.glow = x; });
        addSlider("Scanlines", cfg_.scanlineStrength, 0, 100,
                  [](Settings &s, int x) { s.scanlineStrength = x; s.scanlines = x > 0; });
        addSlider("Text brightness", cfg_.brightness, 25, 100,
                  [](Settings &s, int x) { s.brightness = x; });
        addSlider("Background opacity", cfg_.backgroundAlpha, 0, 180,
                  [](Settings &s, int x) { s.backgroundAlpha = x; });

        auto *music = new QCheckBox("Dance while a MPRIS player is playing", dialog);
        music->setChecked(cfg_.musicResponse);
        form->addRow(music);
        connect(music, &QCheckBox::toggled, dialog, [this](bool on) {
            cfg_.musicResponse = on;
            if (!on) musicPlaying_ = false;
            redraw();
        });

        auto *border = new QCheckBox("Show border", dialog);
        border->setChecked(cfg_.frame);
        form->addRow(border);
        connect(border, &QCheckBox::toggled, dialog, [this](bool on) {
            cfg_.frame = on;
            redraw();
        });

        auto *colorButton = new QPushButton("Text color…", dialog);
        form->addRow(colorButton);
        connect(colorButton, &QPushButton::clicked, dialog, [this, dialog] {
            QColor c = QColorDialog::getColor(cfg_.green, dialog, "CRT phosphor color");
            if (c.isValid()) {
                cfg_.green = c;
                cfg_.dim = c.darker(145);
                redraw();
            }
        });

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, dialog);
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, dialog, [this, dialog] {
            cfg_.save();
            dialog->accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, dialog, [this, dialog, before] {
            cfg_ = before;
            redraw();
            dialog->reject();
        });
        connect(dialog, &QObject::destroyed, this, [this] { settingsWindow_ = nullptr; });
        dialog->show();
    }
    void edit() {
        if (editing_) return;
        before_ = cfg_.quad;
        editing_ = true;
        selected_ = -1;
        applyMode();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        const QPointF origin(geometry().topLeft());
        QPolygonF from, to;
        from << QPointF(0, 0) << QPointF(W, 0) << QPointF(W, H) << QPointF(0, H);
        for (const auto &point : cfg_.quad) to << point - origin;
        QTransform t;
        if (QTransform::quadToQuad(from, to, t)) {
            p.setTransform(t);
            p.drawImage(QPointF(), frame_);
            p.resetTransform();
        }
        if (!editing_) return;
        p.setPen(QPen(QColor("#7BFFAA"), 2, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawPolygon(to);
        for (int i = 0; i < 4; ++i) {
            p.setPen(QColor("#102A1C"));
            p.setBrush(i == selected_ ? QColor("#FFFFA1") : QColor("#7BFFAA"));
            p.drawEllipse(to[i], 10, 10);
            p.setPen(QColor("#102A1C"));
            QFont f(cfg_.font); f.setPixelSize(12); f.setBold(true);
            p.setFont(f);
            p.drawText(QRectF(to[i].x()-10, to[i].y()-10, 20, 20),
                       Qt::AlignCenter, QString::number(i+1));
        }
        QRect top(14, 14, std::min(width()-28, 890), 80);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(5, 24, 15, 230));
        p.drawRoundedRect(top, 5, 5);
        p.setPen(QColor("#B0FFBB"));
        QFont f(cfg_.font); f.setPixelSize(15); p.setFont(f);
        p.drawText(top.adjusted(12, 6, -6, -6), Qt::AlignLeft | Qt::TextWordWrap,
            QStringLiteral("CRT-CLOCK / КАЛИБРОВКА\n"
                           "Тяни углы или весь экран • Enter: сохранить • Esc: отмена • B: фон • F: рамка"));
    }

    void mousePressEvent(QMouseEvent *e) override {
        if (!editing_ || e->button() != Qt::LeftButton) return;
        QPointF pt = e->position() + QPointF(geometry().topLeft());
        for (int i = 0; i < 4; ++i) {
            if (QLineF(pt, cfg_.quad[i]).length() < 25) {
                selected_ = i;
                last_ = pt;
                update();
                return;
            }
        }
        QPolygonF polygon;
        for (const auto &point : cfg_.quad) polygon << point;
        if (polygon.containsPoint(pt, Qt::OddEvenFill)) {
            selected_ = 4; // Drag entire quad.
            last_ = pt;
        }
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if (!editing_ || selected_ < 0) return;
        QPointF pt = e->position() + QPointF(geometry().topLeft());
        QPointF clamped(std::clamp(pt.x(), qreal(screen_.left()+2), qreal(screen_.right()-2)),
                        std::clamp(pt.y(), qreal(screen_.top()+2), qreal(screen_.bottom()-2)));
        if (selected_ < 4) cfg_.quad[selected_] = clamped;
        else {
            QPointF delta = clamped - last_;
            bool fits = true;
            for (const auto &corner : cfg_.quad)
                fits &= screen_.adjusted(2, 2, -2, -2).contains((corner+delta).toPoint());
            if (fits) for (auto &corner : cfg_.quad) corner += delta;
        }
        last_ = clamped;
        update();
    }
    void mouseReleaseEvent(QMouseEvent *e) override {
        if (e->button() == Qt::LeftButton) selected_ = -1;
    }
    void keyPressEvent(QKeyEvent *e) override {
        if (!editing_) return;
        switch (e->key()) {
            case Qt::Key_Escape:
                cfg_.quad = before_; editing_ = false; applyMode(); break;
            case Qt::Key_Enter:
            case Qt::Key_Return:
                cfg_.save(); editing_ = false; redraw(); applyMode(); break;
            case Qt::Key_B:
                cfg_.backgroundAlpha = cfg_.backgroundAlpha ? 0 : 35; redraw(); break;
            case Qt::Key_F:
                cfg_.frame = !cfg_.frame; redraw(); break;
            default: QWidget::keyPressEvent(e);
        }
    }

private:
    void redraw() { frame_ = drawCanvas(cfg_, pose_); update(); }
    void applyMode() {
        hide();
        Qt::WindowFlags flags = Qt::Tool | Qt::FramelessWindowHint;
        if (editing_) flags |= Qt::WindowStaysOnTopHint;
        else flags |= Qt::WindowDoesNotAcceptFocus |
                      Qt::WindowTransparentForInput | Qt::WindowStaysOnBottomHint;
        setWindowFlags(flags);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating, !editing_);
        setFocusPolicy(editing_ ? Qt::StrongFocus : Qt::NoFocus);
        setGeometry(editing_ ? screen_ : bounds(cfg_));
        show();
        if (editing_) { raise(); activateWindow(); setFocus(); }
    }

    QRect screen_;
    Settings cfg_;
    std::array<QPointF, 4> before_{};
    QImage frame_;
    QTimer tick_;
    QPointF last_;
    int selected_ = -1, pose_ = 0, seconds_ = 0;
    QList<int> steps_;
    bool musicPlaying_ = false;
    QDialog *settingsWindow_ = nullptr;
    bool editing_ = false;
};
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setApplicationName("crt-clock");

    const QStringList args = app.arguments();
    const QString command = args.contains("--calibrate") ? "calibrate"
                          : args.contains("--settings") ? "settings"
                          : args.contains("--quit") ? "quit" : "start";
    const QString socket = QString("crt-clock-%1").arg(getuid());

    QLocalSocket existing;
    existing.connectToServer(socket, QIODevice::WriteOnly);
    if (existing.waitForConnected(300)) {
        existing.write((command + "\n").toUtf8());
        existing.waitForBytesWritten(300);
        existing.disconnectFromServer();
        return 0;
    }
    if (command == "quit") return 0;

    QLocalServer::removeServer(socket);
    QLocalServer server;
    if (!server.listen(socket)) return 1;
    ClockWindow clock;
    QObject::connect(&server, &QLocalServer::newConnection, &app, [&] {
        while (auto *client = server.nextPendingConnection()) {
            QObject::connect(client, &QLocalSocket::readyRead, &app, [client, &clock] {
                QString cmd = QString::fromUtf8(client->readAll()).trimmed();
                if (cmd == "calibrate") clock.edit();
                else if (cmd == "settings") clock.settings();
                else if (cmd == "quit") QApplication::quit();
                else clock.start();
                client->disconnectFromServer();
            });
            QObject::connect(client, &QLocalSocket::disconnected,
                             client, &QObject::deleteLater);
        }
    });
    if (command == "calibrate") clock.edit();
    else {
        clock.start();
        if (command == "settings") clock.settings();
    }
    return app.exec();
}
