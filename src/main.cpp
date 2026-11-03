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
#include <QElapsedTimer>
#include <QFontComboBox>
#include <QLocale>
#include <QLineF>
#include <QPainterPath>
#include <QTimer>
#include <memory>
#include "pet_animation.hpp"
#include "perspective.hpp"
#include "keyboard_activity.hpp"
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
    perspective::Handles quad;
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
    bool typingResponse = true;
    bool clockFlicker = true;
    int pixelScale = 2;

    explicit Settings(QRect screen) {
        const qreal x = screen.left() + screen.width() * .65;
        const qreal y = screen.top() + screen.height() * .31;
        const qreal w = screen.width() * .18;
        const qreal h = screen.height() * .27;
        quad = perspective::fromCorners({QPointF(x, y + 8), QPointF(x+w, y),
            QPointF(x+w-10, y+h), QPointF(x+8, y+h+7)});
    }

    void load() {
        QSettings s(configFile(), QSettings::IniFormat);
        for (int i = 0; i < 4; ++i) {
            quad[i].setX(s.value(QString("quad/x%1").arg(i), quad[i].x()).toDouble());
            quad[i].setY(s.value(QString("quad/y%1").arg(i), quad[i].y()).toDouble());
        }
        // v0.2 saved four points only: derive new midpoints AFTER loading those.
        const std::array<QPointF,4> oldCorners{quad[0], quad[1], quad[2], quad[3]};
        const auto defaults = perspective::fromCorners(oldCorners);
        for (int i = 4; i < 8; ++i) {
            quad[i].setX(s.value(QString("quad/x%1").arg(i), defaults[i].x()).toDouble());
            quad[i].setY(s.value(QString("quad/y%1").arg(i), defaults[i].y()).toDouble());
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
        typingResponse = s.value("pet/typing_response", typingResponse).toBool();
        clockFlicker = s.value("appearance/clock_flicker", clockFlicker).toBool();
        pixelScale = std::clamp(s.value("appearance/pixel_scale", pixelScale).toInt(), 1, 4);
        if (!green.isValid()) green = QColor("#B0FFBB");
        if (!dim.isValid()) dim = QColor("#70BB8A");
        if (!background.isValid()) background = QColor("#092419");
        if (pet.isEmpty()) pet = QStringLiteral("(づ｡◕‿‿◕｡)づ");
    }

    void save() const {
        QDir().mkpath(QFileInfo(configFile()).absolutePath());
        QSettings s(configFile(), QSettings::IniFormat);
        for (int i = 0; i < 8; ++i) {
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
        s.setValue("pet/typing_response", typingResponse);
        s.setValue("appearance/clock_flicker", clockFlicker);
        s.setValue("appearance/pixel_scale", pixelScale);
        s.sync();
    }
};

QRect bounds(const Settings &cfg) {
    return perspective::pixelBounds(cfg.quad);
}

void centered(QPainter &p, QRectF r, const QString &text, const QFont &font, QColor color) {
    p.setFont(font);
    p.setPen(color);
    p.drawText(r, Qt::AlignCenter | Qt::TextSingleLine, text);
}

QImage drawCanvas(const Settings &cfg, int pose, double clockIntensity) {
    QImage glyphs(W, H, QImage::Format_ARGB32_Premultiplied);
    glyphs.fill(Qt::transparent);
    QPainter text(&glyphs);
    text.setRenderHint(QPainter::Antialiasing, true);
    text.setRenderHint(QPainter::TextAntialiasing, true);

    QFont time(cfg.font);
    time.setPixelSize(97);
    time.setWeight(QFont::Bold);
    QFont date(cfg.font);
    date.setPixelSize(32);
    date.setWeight(QFont::Medium);
    QFont petFont(cfg.font);
    petFont.setPixelSize(45);

    // One shared font size for every animation pose. Never shrink the pet
    // when a wider frame appears; that previously made it jump and deform.
    while (petFont.pixelSize() > 19) {
        QFontMetricsF metrics(petFont);
        qreal widest = metrics.horizontalAdvance(cfg.pet);
        for (int i = 1; i <= 17; ++i)
            widest = std::max(widest, metrics.horizontalAdvance(pet::glyph(i, cfg.pet)));
        if (widest <= 446) break;
        petFont.setPixelSize(petFont.pixelSize() - 1);
    }

    const auto now = QDateTime::currentDateTime();
    QColor timeColor = cfg.green;
    timeColor.setAlphaF(std::clamp(cfg.brightness / 100.0 * clockIntensity, 0.0, 1.0));
    QColor dateColor = cfg.dim;
    dateColor.setAlphaF(std::clamp(cfg.brightness / 100.0 * clockIntensity, 0.0, 1.0));
    QColor petColor = cfg.green;
    petColor.setAlphaF(cfg.brightness / 100.0);

    centered(text, QRectF(12, 27, 456, 125), now.toString("HH:mm"), time, timeColor);
    centered(text, QRectF(14, 154, 452, 52),
             QLocale(QLocale::English).toString(now.date(), "ddd, dd MMM yyyy").toUpper(),
             date, dateColor);
    centered(text, QRectF(10, 217, 460, 82), pet::glyph(pose, cfg.pet), petFont, petColor);
    text.end();

    // Final scanlines are drawn AFTER perspective mapping, at screen scale.
    return crtComposite(glyphs, cfg.background, cfg.backgroundAlpha, cfg.glow,
                        cfg.pixelScale, cfg.frame, cfg.dim);
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
        elapsed_.start();
        keyboard_ = std::make_unique<KeyboardActivity>([this] { edit(); });
        flickerDue_ = QRandomGenerator::global()->bounded(55000, 85000);
        connect(&tick_, &QTimer::timeout, this, [this] { updateState(); });
        tick_.start(40);
        redraw();
    }

    void updateState() {
        const qint64 now = elapsed_.elapsed();
        keyboard_->poll(now);
        if (cfg_.musicResponse && now >= nextMusicCheck_) {
            musicPlaying_ = musicIsPlaying();
            nextMusicCheck_ = now + 3000;
        } else if (!cfg_.musicResponse) {
            musicPlaying_ = false;
        }

        const bool typing = cfg_.typingResponse && keyboard_->recentlyTyping(now);
        bool dirty = animator_.advance(now, typing, cfg_.musicResponse && musicPlaying_);

        // Rare two-pulse phosphor flicker affects CLOCK only, not the pet.
        double intensity = 1.0;
        if (cfg_.clockFlicker) {
            if (flickerStart_ < 0 && now >= flickerDue_) {
                flickerStart_ = now;
                flickerDue_ = now + QRandomGenerator::global()->bounded(55000, 85000);
            }
            if (flickerStart_ >= 0) {
                static constexpr double pulses[] = {0.95, 0.38, 0.98, 0.24, 1.0};
                const int phase = int((now - flickerStart_) / 95);
                if (phase < 5) intensity = pulses[phase];
                else flickerStart_ = -1;
            }
        }
        if (intensity != clockIntensity_) {
            clockIntensity_ = intensity;
            dirty = true;
        }

        const auto stamp = QDateTime::currentDateTime().toString("yyyyMMddHHmm");
        if (stamp != lastMinute_) {
            lastMinute_ = stamp;
            dirty = true;
        }
        if (dirty) redraw();
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
        addSlider("Pixel / raster size", cfg_.pixelScale, 1, 4,
                  [](Settings &s, int x) { s.pixelScale = x; });
        addSlider("Background opacity", cfg_.backgroundAlpha, 0, 180,
                  [](Settings &s, int x) { s.backgroundAlpha = x; });

        auto *flicker = new QCheckBox("Rare two-pulse clock flicker", dialog);
        flicker->setChecked(cfg_.clockFlicker);
        form->addRow(flicker);
        connect(flicker, &QCheckBox::toggled, dialog, [this](bool on) {
            cfg_.clockFlicker = on;
            if (!on) clockIntensity_ = 1;
            redraw();
        });

        auto *typing = new QCheckBox("React to typing (priority over music)", dialog);
        typing->setChecked(cfg_.typingResponse);
        form->addRow(typing);
        connect(typing, &QCheckBox::toggled, dialog, [this](bool on) {
            cfg_.typingResponse = on;
        });

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

        auto *fontMenu = new QFontComboBox(dialog);
        fontMenu->setFontFilters(QFontComboBox::MonospacedFonts);
        fontMenu->setCurrentFont(QFont(cfg_.font));
        form->addRow("Font", fontMenu);
        connect(fontMenu, &QFontComboBox::currentFontChanged, dialog,
                [this](const QFont &f) { cfg_.font = f.family(); redraw(); });

        auto *calibrate = new QPushButton("Edit 8 perspective points (Super+Shift+C)", dialog);
        form->addRow(calibrate);
        connect(calibrate, &QPushButton::clicked, dialog, [this] { edit(); });

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
        perspective::draw(p, frame_, cfg_.quad, origin);
        perspective::scanlines(p, cfg_.quad, origin,
                              cfg_.scanlines ? cfg_.scanlineStrength : 0);
        if (!editing_) return;
        QPolygonF to = perspective::outline(cfg_.quad);
        for (QPointF &point : to) point -= origin;
        p.setPen(QPen(QColor("#7BFFAA"), 2, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawPolygon(to);
        for (int i = 0; i < 8; ++i) {
            const QPointF point = cfg_.quad[i] - origin;
            p.setPen(QPen(QColor("#102A1C"), 1));
            p.setBrush(i == selected_ ? QColor("#FFFFA1") : QColor("#7BFFAA"));
            p.drawEllipse(point, i < 4 ? 10 : 8, i < 4 ? 10 : 8);
            p.setPen(QColor("#102A1C"));
            QFont f(cfg_.font); f.setPixelSize(12); f.setBold(true);
            p.setFont(f);
            p.drawText(QRectF(point.x()-10, point.y()-10, 20, 20),
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
                           "8 точек: углы и середины сторон • Enter: сохранить • Esc: отмена • B: фон • F: рамка"));
    }

    void mousePressEvent(QMouseEvent *e) override {
        if (!editing_ || e->button() != Qt::LeftButton) return;
        QPointF pt = e->position() + QPointF(geometry().topLeft());
        for (int i = 0; i < 8; ++i) {
            if (QLineF(pt, cfg_.quad[i]).length() < 20) {
                selected_ = i;
                last_ = pt;
                update();
                return;
            }
        }
        QPolygonF polygon = perspective::outline(cfg_.quad);
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
    void redraw() {
        frame_ = drawCanvas(cfg_, animator_.pose(), clockIntensity_);
        update();
    }
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
    perspective::Handles before_{};
    QImage frame_;
    QTimer tick_;
    QElapsedTimer elapsed_;
    std::unique_ptr<KeyboardActivity> keyboard_;
    pet::Animator animator_;
    QPointF last_;
    int selected_ = -1;
    qint64 nextMusicCheck_ = 0;
    qint64 flickerDue_ = 60000;
    qint64 flickerStart_ = -1;
    double clockIntensity_ = 1.0;
    QString lastMinute_;
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
