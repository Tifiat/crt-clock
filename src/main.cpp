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
#include <QComboBox>
#include <QFontDatabase>
#include <QScrollArea>
#include <QFrame>
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
#include "pet_render.hpp"
#include "pet_tail.hpp"
#include "matrix_rain.hpp"
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QKeyEvent>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
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
#include <X11/Xlib.h>
#include <X11/Xatom.h>

namespace {
constexpr int W = 480;
constexpr int H = 320;

// Request XFCE/X11 to show the desktop widget on all workspaces.
void keepOnEveryDesktop(WId window) {
    Display *display = XOpenDisplay(nullptr);
    if (!display) return;
    const Atom desktop = XInternAtom(display, "_NET_WM_DESKTOP", False);
    XEvent event{};
    event.xclient.type = ClientMessage;
    event.xclient.window = static_cast<Window>(window);
    event.xclient.message_type = desktop;
    event.xclient.format = 32;
    event.xclient.data.l[0] = 0xFFFFFFFFL;
    event.xclient.data.l[1] = 1;
    XSendEvent(display, DefaultRootWindow(display), False,
               SubstructureNotifyMask | SubstructureRedirectMask, &event);
    XFlush(display);
    XCloseDisplay(display);
}

QString configFile() {
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + "/crt-clock/config.ini";
}

struct Settings {
    perspective::Handles quad;
    QString clockFont = QStringLiteral("IBM Plex Mono");
    QString petFont = QStringLiteral("IBM Plex Mono");
    int petFontSize = pet::kDefaultFontPixels;
    int petStroke = pet::kDefaultStrokeStrength;
    QString pet = QStringLiteral("(づ｡◕‿‿◕｡)づ");
    QColor green = QColor("#B0FFBB");
    QColor dim = QColor("#70BB8A");
    QColor background = QColor("#092419");
    int backgroundAlpha = 35;
    bool frame = false;
    bool scanlines = true;
    int glow = 44;
    int scanlineStrength = 32;
    int scanlineSpeed = 55;
    int grilleStrength = 35;
    int rippleStrength = 24;
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
        // Backward compatibility: a single v0.5 font is migrated to both fields.
        const QString previousFont = s.value("appearance/font", clockFont).toString();
        clockFont = s.value("appearance/clock_font", previousFont).toString();
        petFont = s.value("appearance/pet_font", previousFont).toString();
        petFontSize = std::clamp(s.value("appearance/pet_font_size", petFontSize).toInt(), 30, 54);
        petStroke = std::clamp(s.value("appearance/pet_stroke", petStroke).toInt(), 0, 100);
        if (clockFont.trimmed().isEmpty()) clockFont = QStringLiteral("IBM Plex Mono");
        if (petFont.trimmed().isEmpty()) petFont = clockFont;
        pet = s.value("appearance/pet", pet).toString();
        green = QColor(s.value("appearance/primary", green.name()).toString());
        dim = QColor(s.value("appearance/secondary", dim.name()).toString());
        background = QColor(s.value("appearance/background", background.name()).toString());
        backgroundAlpha = std::clamp(s.value("appearance/background_alpha", backgroundAlpha).toInt(), 0, 255);
        frame = s.value("appearance/border", frame).toBool();
        scanlines = s.value("appearance/scanlines", scanlines).toBool();
        glow = std::clamp(s.value("appearance/glow", glow).toInt(), 0, 100);
        scanlineStrength = std::clamp(s.value("appearance/scanline_strength", scanlineStrength).toInt(), 0, 100);
        scanlineSpeed = std::clamp(s.value("appearance/scanline_speed", scanlineSpeed).toInt(), 0, 100);
        grilleStrength = std::clamp(s.value("appearance/grille_strength", grilleStrength).toInt(), 0, 100);
        rippleStrength = std::clamp(s.value("appearance/ripple_strength", rippleStrength).toInt(), 0, 100);
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
        s.setValue("appearance/clock_font", clockFont);
        s.setValue("appearance/pet_font", petFont);
        s.setValue("appearance/pet_font_size", petFontSize);
        s.setValue("appearance/pet_stroke", petStroke);
        s.setValue("appearance/pet", pet);
        s.setValue("appearance/primary", green.name());
        s.setValue("appearance/secondary", dim.name());
        s.setValue("appearance/background", background.name());
        s.setValue("appearance/background_alpha", backgroundAlpha);
        s.setValue("appearance/border", frame);
        s.setValue("appearance/scanlines", scanlines);
        s.setValue("appearance/glow", glow);
        s.setValue("appearance/scanline_strength", scanlineStrength);
        s.setValue("appearance/scanline_speed", scanlineSpeed);
        s.setValue("appearance/grille_strength", grilleStrength);
        s.setValue("appearance/ripple_strength", rippleStrength);
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

QImage drawCanvas(const Settings &cfg, int pose, double clockIntensity, const pet::JoyTail &tail) {
    QImage glyphs(W, H, QImage::Format_ARGB32_Premultiplied);
    glyphs.fill(Qt::transparent);
    QPainter text(&glyphs);
    text.setRenderHint(QPainter::Antialiasing, true);
    text.setRenderHint(QPainter::TextAntialiasing, true);

    QFont time(cfg.clockFont);
    time.setPixelSize(97);
    time.setWeight(QFont::Bold);
    QFont date(cfg.clockFont);
    date.setPixelSize(32);
    date.setWeight(QFont::Bold);
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
    pet::paint(text, QRectF(10, 217, 460, 82), cfg.petFont,
               cfg.pet, pose, petColor, cfg.petFontSize, cfg.petStroke, tail);
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
        rippleDue_ = QRandomGenerator::global()->bounded(6500, 12000);
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
        const bool joyful = cfg_.pet == QStringLiteral("(づ｡◕‿‿◕｡)づ") &&
                           pet::hasJoyfulEyes(pet::frame(animator_.pose()));
        dirty = joyTail_.advance(now, joyful) || dirty;
        const quint64 count = keyboard_->pressCount();
        if (count > keyPressCount_) {
            // Each detected press creates one decorative glyph, not the typed
            // character. Respect the user's typing-response toggle.
            if (cfg_.typingResponse) {
                const quint64 newPresses = std::min<quint64>(count - keyPressCount_, 64);
                for (quint64 i = 0; i < newPresses; ++i)
                    matrix_.addPress(now);
            }
            keyPressCount_ = count;
        }
        if (matrix_.advance(now, typing)) {
            rebuildMatrix();
            update();
        }

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
        // Draw only the occasional ripple band; no expensive canvas rebuild.
        if (cfg_.rippleStrength > 0 && now >= rippleDue_) {
            rippleStart_ = now;
            rippleDue_ = now + QRandomGenerator::global()->bounded(9000, 17000);
        }
        if (rippleStart_ >= 0) {
            if (now - rippleStart_ > perspective::sweepDurationMs(cfg_.scanlineSpeed)) rippleStart_ = -1;
            update();
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
        dialog->setAttribute(Qt::WA_QuitOnClose, false);
        dialog->setWindowTitle("CRT Clock / Settings");
        dialog->resize(470, 595);
        auto *layout = new QVBoxLayout(dialog);
        auto *scroll = new QScrollArea(dialog);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        auto *fields = new QWidget(scroll);
        auto *form = new QFormLayout(fields);
        scroll->setWidget(fields);
        layout->addWidget(scroll, 1);

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
        addSlider("Static scanline strength", cfg_.scanlineStrength, 0, 100,
                  [](Settings &s, int x) { s.scanlineStrength = x; s.scanlines = x > 0; });
        addSlider("Moving scanline speed (slow to fast)", cfg_.scanlineSpeed, 0, 100,
                  [](Settings &s, int x) { s.scanlineSpeed = x; });
        addSlider("Phosphor aperture grille", cfg_.grilleStrength, 0, 100,
                  [](Settings &s, int x) { s.grilleStrength = x; });
        addSlider("Signal ripple", cfg_.rippleStrength, 0, 100,
                  [](Settings &s, int x) { s.rippleStrength = x; });
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

        // QFontComboBox with MonospacedFonts used to discard font family
        // names not classified as fixed pitch by Qt/fontconfig. Use the full
        // installed family list, editable for newly installed or custom fonts.
        auto addFont = [this, dialog, form](const QString &label, QString initial,
                                            auto setter) {
            auto *combo = new QComboBox(dialog);
            combo->setEditable(true);
            combo->setInsertPolicy(QComboBox::NoInsert);
            combo->addItems(QFontDatabase::families());
            combo->setCurrentText(initial);
            combo->setToolTip("All installed font families. You can also type a family name. "
                              "Restart CRT Clock to refresh fonts installed while it was running.");
            form->addRow(label, combo);
            connect(combo, &QComboBox::currentTextChanged, dialog,
                    [this, setter](const QString &family) {
                        if (family.trimmed().isEmpty()) return;
                        setter(cfg_, family);
                        redraw();
                    });
        };
        addFont("Clock and date font", cfg_.clockFont,
                [](Settings &s, const QString &f) { s.clockFont = f; });
        addFont("Pet font", cfg_.petFont,
                [](Settings &s, const QString &f) { s.petFont = f; });
        addSlider("Pet font size (px)", cfg_.petFontSize, 30, 54,
                  [](Settings &s, int x) { s.petFontSize = x; });
        addSlider("Pet stroke thickness", cfg_.petStroke, 0, 100,
                  [](Settings &s, int x) { s.petStroke = x; });

        auto *calibrate = new QPushButton("Edit 9 calibration handles (Super+Shift+C)", dialog);
        form->addRow(calibrate);
        connect(calibrate, &QPushButton::clicked, dialog, [this, dialog] {
            // Commit appearance values before closing settings so Cancel in the
            // old dialog cannot undo a newly saved eight-point calibration.
            cfg_.save();
            dialog->accept();
            edit();
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
        perspective::draw(p, frame_, cfg_.quad, origin);
        if (matrix_.active()) perspective::draw(p, matrixFrame_, cfg_.quad, origin);
        perspective::scanlines(p, cfg_.quad, origin,
                              cfg_.scanlines ? cfg_.scanlineStrength : 0);
        perspective::grille(p, cfg_.quad, origin, cfg_.grilleStrength);
        perspective::ripple(p, cfg_.quad, origin,
                             rippleStart_ < 0 ? -1.0 :
                               (elapsed_.elapsed() - rippleStart_) /
                               double(perspective::sweepDurationMs(cfg_.scanlineSpeed)),
                             cfg_.rippleStrength);
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
            QFont f(cfg_.clockFont); f.setPixelSize(12); f.setBold(true);
            p.setFont(f);
            p.drawText(QRectF(point.x()-10, point.y()-10, 20, 20),
                       Qt::AlignCenter, QString::number(i+1));
        }
        // Nine handles: eight independent contour points and one translation handle.
        const QPointF move = perspective::center(cfg_.quad) - origin;
        p.setPen(QPen(QColor("#102A1C"), 2));
        p.setBrush(selected_ == 8 ? QColor("#FFFFA1") : QColor("#FFCA82"));
        p.drawEllipse(move, 13, 13);
        p.setPen(QColor("#102A1C"));
        QFont handleFont(cfg_.clockFont); handleFont.setPixelSize(12); handleFont.setBold(true);
        p.setFont(handleFont);
        p.drawText(QRectF(move.x()-12, move.y()-12, 24, 24),
                   Qt::AlignCenter, QStringLiteral("9"));
        QRect top(14, 14, std::min(width()-28, 890), 80);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(5, 24, 15, 230));
        p.drawRoundedRect(top, 5, 5);
        p.setPen(QColor("#B0FFBB"));
        QFont f(cfg_.clockFont); f.setPixelSize(15); p.setFont(f);
        p.drawText(top.adjusted(12, 6, -6, -6), Qt::AlignLeft | Qt::TextWordWrap,
            QStringLiteral("CRT-CLOCK / КАЛИБРОВКА\n"
                           "8 крайних: деформация • центр 9: перенос • Enter: сохранить • Esc: отмена • B: фон • F: рамка"));
    }

    void mousePressEvent(QMouseEvent *e) override {
        if (!editing_ || e->button() != Qt::LeftButton) return;
        QPointF pt = e->position() + QPointF(geometry().topLeft());
        if (QLineF(pt, perspective::center(cfg_.quad)).length() < 20) {
            selected_ = 8;
            last_ = pt;
            update();
            return;
        }
        for (int i = 0; i < 8; ++i) {
            if (QLineF(pt, cfg_.quad[i]).length() < 20) {
                selected_ = i;
                last_ = pt;
                update();
                return;
            }
        }
        // Only the center handle translates the whole widget.
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if (!editing_ || selected_ < 0) return;
        QPointF pt = e->position() + QPointF(geometry().topLeft());
        QPointF clamped(std::clamp(pt.x(), qreal(screen_.left()+2), qreal(screen_.right()-2)),
                        std::clamp(pt.y(), qreal(screen_.top()+2), qreal(screen_.bottom()-2)));
        if (selected_ < 8) cfg_.quad[selected_] = clamped;
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
    void contextMenuEvent(QContextMenuEvent *e) override {
        if (editing_) return;
        QMenu menu(this);
        auto *settingsAction = menu.addAction(QStringLiteral("Настройки CRT"));
        auto *calibrateAction = menu.addAction(QStringLiteral("Калибровка перспективы"));
        menu.addSeparator();
        auto *quitAction = menu.addAction(QStringLiteral("Закрыть виджет"));
        QAction *selected = menu.exec(e->globalPos());
        if (selected == settingsAction) settings();
        else if (selected == calibrateAction) edit();
        else if (selected == quitAction) QApplication::quit();
        e->accept();
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
    void rebuildMatrix() {
        if (matrix_.active()) {
            const QImage glyphs = matrix_.image(cfg_.green, cfg_.brightness);
            // Run the same pixel scale and phosphor glow as the clock/pet.
            // Static scanlines, aperture grille and ripple are shared by both
            // layers in paintEvent after the perspective transformation.
            matrixFrame_ = crtComposite(glyphs, Qt::transparent, 0,
                                         cfg_.glow, cfg_.pixelScale, false,
                                         cfg_.dim);
        } else {
            matrixFrame_ = QImage();
        }
    }
    void redraw() {
        frame_ = drawCanvas(cfg_, animator_.pose(), clockIntensity_, joyTail_);
        if (matrix_.active()) rebuildMatrix();
        update();
    }
    void applyMode() {
        hide();
        Qt::WindowFlags flags = Qt::Tool | Qt::FramelessWindowHint;
        if (editing_) flags |= Qt::WindowStaysOnTopHint;
        else flags |= Qt::WindowDoesNotAcceptFocus |
                      Qt::WindowStaysOnBottomHint;
        setWindowFlags(flags);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating, !editing_);
        setFocusPolicy(editing_ ? Qt::StrongFocus : Qt::NoFocus);
        setGeometry(editing_ ? screen_ : bounds(cfg_));
        show();
        QTimer::singleShot(0, this, [this] { keepOnEveryDesktop(winId()); });
        if (editing_) { raise(); activateWindow(); setFocus(); }
    }

    QRect screen_;
    Settings cfg_;
    perspective::Handles before_{};
    QImage frame_;
    MatrixRain matrix_;
    pet::JoyTail joyTail_;
    QImage matrixFrame_;
    QTimer tick_;
    QElapsedTimer elapsed_;
    std::unique_ptr<KeyboardActivity> keyboard_;
    pet::Animator animator_;
    QPointF last_;
    int selected_ = -1;
    quint64 keyPressCount_ = 0;
    qint64 nextMusicCheck_ = 0;
    qint64 rippleDue_ = 10000;
    qint64 rippleStart_ = -1;
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
    app.setQuitOnLastWindowClosed(false);

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
