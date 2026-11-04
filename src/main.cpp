#include <QApplication>
#include <QColor>
#include <QDateTime>
#include <QDebug>
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
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QElapsedTimer>
#include <QComboBox>
#include <QFontDatabase>
#include <QScrollArea>
#include <QFrame>
#include <QMessageBox>
#include <QSizePolicy>
#include <QLocale>
#include <QLineF>
#include <QPainterPath>
#include <QTimer>
#include <memory>
#include <functional>
#include "pet_animation.hpp"
#include "perspective.hpp"
#include "perspective_raster_cache.hpp"
#include "keyboard_activity.hpp"
#include <QHBoxLayout>
#include <QColorDialog>
#include <QList>
#include "crt_style.hpp"
#include "pet_poses.hpp"
#include "pet_render.hpp"
#include "pet_sprite_cache.hpp"
#include "pet_gift_render.hpp"
#include "pet_tail.hpp"
#include "clock_flicker.hpp"
#include "desktop_autostart.hpp"
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
// Xlib defines None as a macro; include this only after every Qt header.
#include "desktop_workspace.hpp"

namespace {
constexpr int W = 480;
constexpr int H = 320;

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
    int timeFontSize = 97;
    int dateFontSize = 39;
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
    int petAnimationSpeed = pet::kDefaultAutonomousSpeed;
    int flickerFrequency = crt::kDefaultFlickerFrequency;
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
        timeFontSize = std::clamp(s.value("appearance/time_font_size", timeFontSize).toInt(), 68, 132);
        dateFontSize = std::clamp(s.value("appearance/date_font_size", dateFontSize).toInt(), 26, 50);
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
        petAnimationSpeed = std::clamp(
            s.value("pet/autonomous_speed", petAnimationSpeed).toInt(),
            pet::kMinAutonomousSpeed, pet::kMaxAutonomousSpeed);
        // Read the old checkbox value on first migration, then use the new slider.
        const int legacyFrequency = s.value("appearance/clock_flicker", true).toBool()
                                    ? crt::kDefaultFlickerFrequency : 0;
        flickerFrequency = std::clamp(
            s.value("appearance/flicker_frequency", legacyFrequency).toInt(), 0, 100);
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
        s.setValue("appearance/time_font_size", timeFontSize);
        s.setValue("appearance/date_font_size", dateFontSize);
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
        s.setValue("pet/autonomous_speed", petAnimationSpeed);
        s.setValue("appearance/flicker_frequency", flickerFrequency);
        s.setValue("appearance/clock_flicker", flickerFrequency > 0); // downgrade compatibility
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

// Cache the whole clock/date/background CRT composite. Only the minute,
// clock flicker, or an appearance-setting change invalidates this expensive
// full-size phosphor/glow image, not each paw/motion frame.
struct ClockCacheKey {
    QString minute;
    QString clockFont;
    QColor green, dim, background;
    int timeSize = 0, dateSize = 0, brightness = 0;
    int backgroundAlpha = 0, glow = 0, pixelScale = 0;
    bool border = false;
    double clockIntensity = 0.0;
    bool operator==(const ClockCacheKey &) const = default;
};

class CanvasCache {
public:
    [[nodiscard]] QImage render(const Settings &cfg, int pose,
                                double clockIntensity, const pet::JoyTail &tail,
                                int musicStreamPhase, int idleBreathPhase,
                                const pet::GiftVisual &gift,
                                const pet::MotionVisual &motion) {
        const auto now = QDateTime::currentDateTime();
        const ClockCacheKey key{
            now.toString("yyyyMMddHHmm"), cfg.clockFont,
            cfg.green, cfg.dim, cfg.background,
            cfg.timeFontSize, cfg.dateFontSize, cfg.brightness,
            cfg.backgroundAlpha, cfg.glow, cfg.pixelScale,
            cfg.frame, clockIntensity
        };
        if (!validClock_ || !(key == clockKey_)) {
            clockKey_ = key;
            validClock_ = true;
            rebuildClock(cfg, now, clockIntensity);
        }

        const bool stockPet =
            cfg.pet == QStringLiteral("(づ｡◕‿‿◕｡)づ");
        QColor petColor = cfg.green;
        petColor.setAlphaF(cfg.brightness / 100.0);

        sprites_.configure(cfg.petFont, cfg.pet, petColor,
                           cfg.petFontSize, cfg.petStroke);

        // Most activity never paints higher than y=192. Processing the CRT
        // phosphor/glow on 480x128 instead of 480x320 cuts repeated
        // scaling costs substantially without changing FPS/effect quality.
        const bool highGift = stockPet &&
            (gift.cue == pet::GiftCue::Transfer ||
             gift.cue == pet::GiftCue::FlyStraight ||
             gift.cue == pet::GiftCue::FlyArc ||
             gift.cue == pet::GiftCue::Burst);
        constexpr int top = pet::SpriteCache::kCropTop;
        const int cropTop = highGift ? 0 : top;
        const bool absent = stockPet && pet::frame(pose).hidden &&
                            !gift.active() && musicStreamPhase < 0;
        if (absent) return clockFrame_;

        QImage moving(W, H - cropTop, QImage::Format_ARGB32_Premultiplied);
        moving.fill(Qt::transparent);
        QPainter text(&moving);
        text.setRenderHint(QPainter::Antialiasing, true);
        text.setRenderHint(QPainter::TextAntialiasing, true);
        text.setRenderHint(QPainter::SmoothPixmapTransform, true);
        text.translate(0, -cropTop);

        // Use a cached supersampled Unicode bitmap. Only the inexpensive
        // position/mirror/vertical-breath transform changes between ticks.
        if (!stockPet || !pet::frame(pose).hidden) {
            text.save();
            const int breath = stockPet && idleBreathPhase >= 0 ?
                (idleBreathPhase == 2 ? -1 : (idleBreathPhase == 5 ? 1 : 0)) : 0;
            // The historic IdleStep poses contain a +/-9px body offset.
            // That was fine for stepping in place, but caused ~18px
            // teleports whenever the walk exchanged left/right feet.
            const int poseShift = stockPet && motion.gait
                ? pet::frame(pose).bodyDx : 0;
            const double correctedX = motion.offsetX +
                (motion.mirror ? poseShift : -poseShift);
            text.translate(correctedX, motion.offsetY + breath);
            if (motion.mirror && stockPet) {
                constexpr qreal pivot = 190.0; // QRectF(-40,217,460,82).center().x()
                text.translate(pivot, 0);
                text.scale(-1.0, 1.0);
                text.translate(-pivot, 0);
            }
            text.drawImage(QPointF(0, top),
                           sprites_.sprite(pet::visiblePose(pose, gift), tail));
            text.restore();
        }

        if (stockPet) {
            if (motion.blush)
                pet::paintShyBlush(text, QRectF(-40, 217, 460, 82), petColor);
            pet::paintGestureStreak(text, QRectF(-40, 217, 460, 82),
                                    petColor, motion.streak);
            pet::paintGiftEffect(text, gift, petColor);
            pet::paintMusicStream(text, petColor, musicStreamPhase);
        }
        text.end();

        const QImage movingCrt = crtComposite(moving, Qt::transparent, 0,
                                             cfg.glow, cfg.pixelScale, false,
                                             cfg.dim);
        QImage output = clockFrame_.copy();
        QPainter composite(&output);
        composite.drawImage(QPoint(0, cropTop), movingCrt);
        if (cfg.frame) {
            // The frame always remains above flying gifts at the edge,
            // as it did in the original monolithic CRT composite.
            QColor rim = cfg.dim;
            rim.setAlpha(135);
            composite.setPen(QPen(rim, 1));
            composite.setBrush(Qt::NoBrush);
            composite.drawRect(QRectF(1, 1, W - 2, H - 2));
        }
        return output;
    }

    [[nodiscard]] quint64 clockBuildCount() const { return clockBuilds_; }
    [[nodiscard]] quint64 spriteBuildCount() const { return sprites_.buildCount(); }

private:
    void rebuildClock(const Settings &cfg, const QDateTime &now,
                      double intensity) {
        QImage glyphs(W, H, QImage::Format_ARGB32_Premultiplied);
        glyphs.fill(Qt::transparent);
        QPainter painter(&glyphs);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        QFont time(cfg.clockFont);
        time.setPixelSize(cfg.timeFontSize);
        time.setWeight(QFont::Bold);
        QFont date(cfg.clockFont);
        date.setPixelSize(cfg.dateFontSize);
        date.setWeight(QFont::Bold);
        QColor timeColor = cfg.green;
        timeColor.setAlphaF(std::clamp(cfg.brightness / 100.0 * intensity, 0.0, 1.0));
        QColor dateColor = cfg.dim.lighter(130);
        dateColor.setAlphaF(std::clamp(cfg.brightness / 100.0 * intensity, 0.0, 1.0));
        centered(painter, QRectF(12, 27, 456, 125),
                 now.toString("HH:mm"), time, timeColor);
        centered(painter, QRectF(14, 154, 452, 52),
                 QLocale(QLocale::English).toString(
                    now.date(), "ddd, dd MMM yyyy").toUpper(),
                 date, dateColor);
        painter.end();

        clockFrame_ = crtComposite(glyphs, cfg.background, cfg.backgroundAlpha,
                                  cfg.glow, cfg.pixelScale, cfg.frame, cfg.dim);
        ++clockBuilds_;
    }

    ClockCacheKey clockKey_;
    bool validClock_ = false;
    QImage clockFrame_;
    pet::SpriteCache sprites_;
    quint64 clockBuilds_ = 0;
};

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
        // Flags are set ONCE: calibration must never hide/reparent/recreate
        // the native X11 window when entering or leaving edit mode.
        setWindowFlags(desktop::widgetWindowFlags());
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setMouseTracking(true);
        elapsed_.start();
        animator_.setAutonomousSpeed(cfg_.petAnimationSpeed, 0);
        keyboard_ = std::make_unique<KeyboardActivity>([this] { edit(); });
        scheduleNextFlicker(0);
        rippleDue_ = QRandomGenerator::global()->bounded(6500, 12000);
        connect(&tick_, &QTimer::timeout, this, [this] { updateState(); });
        tick_.start(40);
        redraw();
    }

    void updateState() {
        const qint64 now = elapsed_.elapsed();
        keyboard_->poll(now);
        // XFWM4 may silently move a Qt::Tool back to the active workspace,
        // particularly when a settings dialog changes window focus. Repair
        // only when the WM no longer advertises sticky/all-workspace state.
        if (now >= nextWorkspaceCheck_) {
            nextWorkspaceCheck_ = now + 3000;
            restoreWorkspaceStickiness();
        }
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
        dirty = joyTail_.advance(now, joyful, cfg_.petAnimationSpeed) || dirty;
        // Three densely staggered vector columns scroll through the old
        // Matrix strip while MPRIS is Playing; typing always hides the stream.
        const int nextStreamPhase = animator_.activity() == pet::Activity::Music
            ? int(((now * qint64(cfg_.petAnimationSpeed) / 100) / 60) % 900) : -1;
        if (nextStreamPhase != musicStreamPhase_) {
            musicStreamPhase_ = nextStreamPhase;
            dirty = true;
        }
        // Keep the +/-1px breath even during long stationary idle expressions.
        // No idle stripes: thin trails are reserved for flying/exploding gifts.
        const int nextBreathPhase = animator_.idleBreathing()
            ? int(((now * qint64(cfg_.petAnimationSpeed) / 100) / 380) % 6) : -1;
        if (nextBreathPhase != idleBreathPhase_) {
            idleBreathPhase_ = nextBreathPhase;
            dirty = true;
        }
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

        // Only the clock flickers. Frequency 0 disables the effect.
        double intensity = 1.0;
        if (cfg_.flickerFrequency > 0) {
            if (flickerStart_ < 0 && now >= flickerDue_) {
                flickerStart_ = now;
                scheduleNextFlicker(now);
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
        if (profileEnabled_ && now >= nextProfileReport_) {
            nextProfileReport_ = now + 6000;
            const auto mean = [](quint64 total, quint64 count) {
                return count ? double(total) / count / 1000000.0 : 0.0;
            };
            qInfo().nospace()
                << "[crt-clock perf] redraws=" << renders_
                << " avgCanvasMs=" << mean(renderTimeNs_, renders_)
                << " paints=" << paints_
                << " avgPaintMs=" << mean(paintTimeNs_, paints_)
                << " warpRebuilds=" << warpRebuilds_
                << " avgWarpMs=" << mean(warpTimeNs_, warpRebuilds_)
                << " cachedWarpMaps=" << perspectiveCache_.buildCount()
                << " matrixRebuilds=" << matrixRebuilds_
                << " clockBuilds=" << canvas_.clockBuildCount()
                << " cachedSpritesBuilt=" << canvas_.spriteBuildCount();
            renders_ = paints_ = matrixRebuilds_ = warpRebuilds_ = 0;
            renderTimeNs_ = paintTimeNs_ = warpTimeNs_ = 0;
        }
    }

    void start() { editing_ = false; applyMode(); }

    void previewGift(const QString &name) {
        int kind = -1;
        if (name == QStringLiteral("star")) kind = int(pet::GiftKind::StarFly);
        else if (name == QStringLiteral("arc")) kind = int(pet::GiftKind::ArcFly);
        else if (name == QStringLiteral("pop")) kind = int(pet::GiftKind::BurstSad);
        else if (name == QStringLiteral("cry")) kind = int(pet::GiftKind::BurstCry);
        else if (name == QStringLiteral("pulse")) kind = int(pet::GiftKind::Pulse);
        if (kind >= 0) requestAnimationPreview(QStringLiteral("gift"), kind);
    }

    // A single preview path is shared by the UI and one-instance IPC.
    // 1.1s debounce avoids the terminal's 950ms keyboard activity window:
    // pressing Enter to request a preview must not cancel that very preview.
    void requestAnimationPreview(const QString &category, int index) {
        const quint64 serial = ++previewSerial_;
        QTimer::singleShot(1100, this, [this, category, index, serial] {
            if (serial != previewSerial_) return;
            const qint64 now = elapsed_.elapsed();
            if (category == QStringLiteral("gift") &&
                index >= 0 && index < pet::kGiftKindCount) {
                animator_.previewGift(static_cast<pet::GiftKind>(index), now);
            } else if (category == QStringLiteral("idle") &&
                       index >= -1 && index < pet::kIdleActionCount) {
                if (index == -1) animator_.previewIdleBreathing(now);
                else animator_.previewIdle(static_cast<pet::IdleAction>(index), now);
            } else if (category == QStringLiteral("music") &&
                       index >= -1 && index < pet::kMusicActionCount) {
                if (index == -1) animator_.previewGroove(now);
                else animator_.previewMusic(static_cast<pet::MusicAction>(index), now);
            } else if (category == QStringLiteral("typing")) {
                animator_.previewTyping(now);
                // Synthetic decorative matrix presses. Never store the keys.
                for (int n = 0; n < 11; ++n) {
                    QTimer::singleShot(n * 170, this, [this, serial] {
                        if (serial != previewSerial_ ||
                            !animator_.previewing(pet::Activity::Typing)) return;
                        matrix_.addPress(elapsed_.elapsed());
                        rebuildMatrix();
                        update();
                    });
                }
            } else return;
            musicStreamPhase_ = -1;
            idleBreathPhase_ = -1;
            redraw();
        });
    }

    void stopAnimationPreview() {
        ++previewSerial_;
        animator_.stopPreview(elapsed_.elapsed());
        redraw();
    }

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
        // Size is calculated from content below instead of hard-coding a stale width.
        auto *layout = new QVBoxLayout(dialog);
        auto *scroll = new QScrollArea(dialog);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        auto *fields = new QWidget(scroll);
        auto *form = new QFormLayout(fields);
        form->setRowWrapPolicy(QFormLayout::WrapAllRows);
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        form->setSizeConstraint(QLayout::SetMinAndMaxSize);
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

        addSlider("Clock flicker frequency (0 = off, 100 = every 4s)",
                  cfg_.flickerFrequency, 0, 100,
                  [this](Settings &s, int value) {
                      s.flickerFrequency = value;
                      flickerStart_ = -1;
                      clockIntensity_ = 1.0;
                      scheduleNextFlicker(elapsed_.elapsed());
                  });

        auto *border = new QCheckBox("Show border", dialog);
        border->setChecked(cfg_.frame);
        form->addRow(border);
        connect(border, &QCheckBox::toggled, dialog, [this](bool on) {
            cfg_.frame = on;
            redraw();
        });
        auto *autostart = new QCheckBox("Start CRT Clock automatically at login", dialog);
        autostart->setToolTip("Create a per-user XDG autostart entry. No background service.");
        autostart->setChecked(desktop::autostartEnabled());
        form->addRow(autostart);

        // QFontComboBox with MonospacedFonts used to discard font family
        // names not classified as fixed pitch by Qt/fontconfig. Use the full
        // installed family list, editable for newly installed or custom fonts.
        auto addFont = [this, dialog, form](const QString &label, QString initial,
                                            auto setter) {
            auto *combo = new QComboBox(dialog);
            combo->setEditable(true);
            combo->setInsertPolicy(QComboBox::NoInsert);
            combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
            combo->setMinimumContentsLength(20);
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
        addSlider("Time font size (px)", cfg_.timeFontSize, 68, 132,
                  [](Settings &s, int x) { s.timeFontSize = x; });
        addSlider("Date font size (px)", cfg_.dateFontSize, 26, 50,
                  [](Settings &s, int x) { s.dateFontSize = x; });
        addSlider("Pet font size (px)", cfg_.petFontSize, 30, 54,
                  [](Settings &s, int x) { s.petFontSize = x; });
        addSlider("Pet stroke thickness", cfg_.petStroke, 0, 100,
                  [](Settings &s, int x) { s.petStroke = x; });

        auto *calibrate = new QPushButton("Edit 9 calibration handles (Super+Shift+C)", dialog);
        form->addRow(calibrate);
        connect(calibrate, &QPushButton::clicked, dialog, [this, dialog, autostart] {
            if (!desktop::setAutostart(autostart->isChecked())) {
                QMessageBox::warning(dialog, "CRT Clock", "Unable to update XDG autostart.");
                return;
            }
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
        connect(buttons, &QDialogButtonBox::accepted, dialog, [this, dialog, autostart] {
            if (!desktop::setAutostart(autostart->isChecked())) {
                QMessageBox::warning(dialog, "CRT Clock", "Unable to update XDG autostart.");
                return;
            }
            cfg_.save();
            dialog->accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, dialog, [this, dialog, before] {
            cfg_ = before;
            animator_.setAutonomousSpeed(cfg_.petAnimationSpeed, elapsed_.elapsed());
            redraw();
            dialog->reject();
        });
        connect(dialog, &QObject::destroyed, this, [this] {
            settingsWindow_ = nullptr;
            // The settings window may change workspace affinity after focus
            // changes. Reassert after dialog teardown and WM updates.
            QTimer::singleShot(0, this, [this] { reassertWorkspaceStickiness(); });
            QTimer::singleShot(250, this, [this] { reassertWorkspaceStickiness(); });
            QTimer::singleShot(900, this, [this] { reassertWorkspaceStickiness(); });
        });
        // Let Qt measure text/fonts after construction, then fit the content
        // within the available desktop. Labels are above fields, so there is
        // no minimum width explosion from a long label + a wide slider.
        const QRect available = QGuiApplication::primaryScreen()->availableGeometry();
        fields->adjustSize();
        const QSize desired = fields->sizeHint();
        const int maxWidth = std::max(320, available.width() - 48);
        const int maxHeight = std::max(380, available.height() - 64);
        dialog->resize(std::min(std::max(desired.width() + 65, 460), maxWidth),
                       std::min(std::max(desired.height() + 108, 520), maxHeight));
        dialog->show();
        QTimer::singleShot(0, this, [this] { reassertWorkspaceStickiness(); });
    }
    void animationSettings() {
        if (animationSettingsWindow_) {
            animationSettingsWindow_->raise();
            animationSettingsWindow_->activateWindow();
            return;
        }
        const int previousSpeed = cfg_.petAnimationSpeed;
        const bool previousMusic = cfg_.musicResponse;
        const bool previousTyping = cfg_.typingResponse;

        auto *dialog = new QDialog(nullptr);
        animationSettingsWindow_ = dialog;
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setAttribute(Qt::WA_QuitOnClose, false);
        dialog->setWindowTitle(QStringLiteral("CRT Clock / Настройки анимации"));
        auto *layout = new QVBoxLayout(dialog);
        auto *scroll = new QScrollArea(dialog);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        layout->addWidget(scroll, 1);
        auto *fields = new QWidget(scroll);
        scroll->setWidget(fields);
        auto *body = new QVBoxLayout(fields);

        auto *controls = new QGroupBox(QStringLiteral("Поведение персонажа"), fields);
        auto *form = new QFormLayout(controls);
        form->setRowWrapPolicy(QFormLayout::WrapAllRows);
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        auto *speed = new QSlider(Qt::Horizontal, controls);
        speed->setRange(pet::kMinAutonomousSpeed, pet::kMaxAutonomousSpeed);
        speed->setValue(cfg_.petAnimationSpeed);
        auto *speedValue = new QLabel(QString::number(cfg_.petAnimationSpeed) + "%", controls);
        auto *speedRow = new QWidget(controls);
        auto *speedLayout = new QHBoxLayout(speedRow);
        speedLayout->setContentsMargins(0, 0, 0, 0);
        speedLayout->addWidget(speed, 1);
        speedLayout->addWidget(speedValue);
        form->addRow(QStringLiteral("Скорость Idle / Music / Gifts"), speedRow);
        connect(speed, &QSlider::valueChanged, dialog, [this, speedValue](int percent) {
            cfg_.petAnimationSpeed = percent;
            animator_.setAutonomousSpeed(percent, elapsed_.elapsed());
            speedValue->setText(QString::number(percent) + "%");
            redraw();
        });

        auto *music = new QCheckBox(QStringLiteral("Танцевать, когда играет музыка (MPRIS)"),
                                    controls);
        music->setChecked(cfg_.musicResponse);
        form->addRow(music);
        connect(music, &QCheckBox::toggled, dialog, [this](bool value) {
            cfg_.musicResponse = value;
            if (!value) musicPlaying_ = false;
            redraw();
        });
        auto *typing = new QCheckBox(QStringLiteral("Реагировать на печать (высший приоритет)"),
                                     controls);
        typing->setChecked(cfg_.typingResponse);
        form->addRow(typing);
        connect(typing, &QCheckBox::toggled, dialog, [this](bool value) {
            cfg_.typingResponse = value;
            redraw();
        });
        body->addWidget(controls);

        auto *description = new QLabel(
            QStringLiteral("Предпросмотр запускается в самом виджете один раз. "
                           "Подождите примерно секунду после нажатия кнопки. "
                           "Случайные очереди при проверке не расходуются."),
            fields);
        description->setWordWrap(true);
        body->addWidget(description);

        auto *tabs = new QTabWidget(fields);
        body->addWidget(tabs, 1);
        auto addTab = [this, tabs](const QString &title, const QString &hint,
                                   const QStringList &items,
                                   std::function<void(int)> preview) {
            auto *page = new QWidget(tabs);
            auto *column = new QVBoxLayout(page);
            auto *label = new QLabel(hint, page);
            label->setWordWrap(true);
            column->addWidget(label);
            auto *selector = new QComboBox(page);
            selector->addItems(items);
            column->addWidget(selector);
            auto *play = new QPushButton(QStringLiteral("▶ Воспроизвести"), page);
            column->addWidget(play);
            column->addStretch(1);
            connect(play, &QPushButton::clicked, page,
                    [selector, preview] { preview(selector->currentIndex()); });
            tabs->addTab(page, title);
        };

        addTab(QStringLiteral("Idle"),
               QStringLiteral("Дыхание в ожидании и все 15 самостоятельных Idle-сцен."),
               {QStringLiteral("Дыхание / ожидание"),
                QStringLiteral("Blink / моргание"),
                QStringLiteral("Peek / взгляд в сторону"),
                QStringLiteral("Smile / улыбка"),
                QStringLiteral("Surprise / удивление"),
                QStringLiteral("Smug / самодовольство"),
                QStringLiteral("Shy / смущение"),
                QStringLiteral("Sleepy / сонный"),
                QStringLiteral("Confused / недоумение"),
                QStringLiteral("Greeting / приветствие"),
                QStringLiteral("Fidget / движения лап"),
                QStringLiteral("Celebrate / радуется"),
                QStringLiteral("Indignant / негодование"),
                QStringLiteral("Patrol / ходит туда-сюда"),
                QStringLiteral("Fetch / уходит и приносит звезду"),
                QStringLiteral("Wink / один раз левым и правым глазом")},
               [this](int i) {
                   requestAnimationPreview(QStringLiteral("idle"), i - 1);
               });
        addTab(QStringLiteral("Music"),
               QStringLiteral("Базовое покачивание и 9 музыкальных танцев без подмигиваний."),
               {QStringLiteral("Groove / базовое покачивание"),
                QStringLiteral("HeadNod / кивает"),
                QStringLiteral("TailWag / спиной и хвостом"),
                QStringLiteral("HandGroove / лапки"),
                QStringLiteral("Bounce / прыжки"),
                QStringLiteral("Sway / раскачивание"),
                QStringLiteral("HappyDance / радуется"),
                QStringLiteral("BigDance / большой танец"),
                QStringLiteral("EyesClosedNotes / закрытые глаза"),
                QStringLiteral("SideStep / шаги в стороны")},
               [this](int i) {
                   requestAnimationPreview(QStringLiteral("music"), i - 1);
               });
        addTab(QStringLiteral("Typing"),
               QStringLiteral("Печать: лапки + 11 тестовых декоративных символов Matrix. "
                              "Текст нажатых клавиш не считывается."),
               {QStringLiteral("Typing + Matrix / печать")},
               [this](int) {
                   requestAnimationPreview(QStringLiteral("typing"), 0);
               });
        addTab(QStringLiteral("Gifts"),
               QStringLiteral("Все пять финалов редкой сцены Fetch с уходом и возвращением."),
               {QStringLiteral("StarFly / звезда и шлейф"),
                QStringLiteral("ArcFly / полёт по дуге"),
                QStringLiteral("BurstSad / взрыв и комичная грусть"),
                QStringLiteral("BurstCry / взрыв и слёзы"),
                QStringLiteral("Pulse / пульсирующий подарок")},
               [this](int i) {
                   requestAnimationPreview(QStringLiteral("gift"), i);
               });
        auto *stop = new QPushButton(QStringLiteral("■ Остановить предпросмотр"), fields);
        body->addWidget(stop);
        connect(stop, &QPushButton::clicked, dialog,
                [this] { stopAnimationPreview(); });

        auto *buttons = new QDialogButtonBox(
            QDialogButtonBox::Save | QDialogButtonBox::Cancel, dialog);
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, dialog, [this, dialog] {
            cfg_.save();
            dialog->accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, dialog,
                [this, dialog, previousSpeed, previousMusic, previousTyping] {
            cfg_.petAnimationSpeed = previousSpeed;
            cfg_.musicResponse = previousMusic;
            cfg_.typingResponse = previousTyping;
            animator_.setAutonomousSpeed(previousSpeed, elapsed_.elapsed());
            stopAnimationPreview();
            dialog->reject();
        });
        connect(dialog, &QObject::destroyed, this, [this] {
            animationSettingsWindow_ = nullptr;
            reassertWorkspaceStickiness();
        });
        const QRect available = QGuiApplication::primaryScreen()->availableGeometry();
        dialog->resize(std::min(570, available.width() - 48),
                       std::min(590, available.height() - 64));
        dialog->show();
        reassertWorkspaceStickiness();
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
        QElapsedTimer timer;
        if (profileEnabled_) timer.start();
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        const QPointF origin(geometry().topLeft());

        if (legacyPerspective_) {
            // Optional diagnostic/visual fallback: the original CPU-heavy
            // 4x or 8x quadToQuad path. Never changes saved perspective data.
            perspective::draw(p, frame_, cfg_.quad, origin);
            if (matrix_.active())
                perspective::draw(p, matrixFrame_, cfg_.quad, origin);
            perspective::scanlines(p, cfg_.quad, origin,
                cfg_.scanlines ? cfg_.scanlineStrength : 0);
            perspective::grille(p, cfg_.quad, origin, cfg_.grilleStrength);
        } else {
        // The projective geometry is recomputed only after perspective
        // calibration or window geometry changes. For each new animated
        // frame, bilinear sampling uses the cached inverse map. A ripple-only
        // repaint simply blits the previously warped image.
        const bool geometryChanged =
            perspectiveCache_.prepare(size(), cfg_.quad, origin);
        if (geometryChanged) {
            projectionDirty_ = true;
            staticOverlayDirty_ = true;
        }
        if (projectionDirty_) {
            QElapsedTimer warpTimer;
            if (profileEnabled_) warpTimer.start();
            projectedFrame_ = perspectiveCache_.render(
                frame_, matrix_.active() ? matrixFrame_ : QImage());
            projectionDirty_ = false;
            if (profileEnabled_) {
                ++warpRebuilds_;
                warpTimeNs_ += warpTimer.nsecsElapsed();
            }
        }
        p.drawImage(QPointF(0, 0), projectedFrame_);

        const int scanStrength =
            cfg_.scanlines ? cfg_.scanlineStrength : 0;
        if (staticOverlayDirty_ ||
            cachedScanlineStrength_ != scanStrength ||
            cachedGrilleStrength_ != cfg_.grilleStrength) {
            staticOverlayDirty_ = false;
            cachedScanlineStrength_ = scanStrength;
            cachedGrilleStrength_ = cfg_.grilleStrength;
            staticOverlay_ = QImage(size(), QImage::Format_ARGB32_Premultiplied);
            staticOverlay_.fill(Qt::transparent);
            QPainter staticPainter(&staticOverlay_);
            staticPainter.setRenderHint(QPainter::Antialiasing, true);
            perspective::scanlines(staticPainter, cfg_.quad, origin, scanStrength);
            perspective::grille(staticPainter, cfg_.quad, origin,
                                cfg_.grilleStrength);
        }
        p.drawImage(QPointF(0, 0), staticOverlay_);
        }
        perspective::ripple(p, cfg_.quad, origin,
                             rippleStart_ < 0 ? -1.0 :
                               (elapsed_.elapsed() - rippleStart_) /
                               double(perspective::sweepDurationMs(cfg_.scanlineSpeed)),
                             cfg_.rippleStrength);
        if (!editing_) {
            if (profileEnabled_) {
                ++paints_;
                paintTimeNs_ += timer.nsecsElapsed();
            }
            return;
        }
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
                           "8 крайних: деформация • центр 9: перенос • Enter: сохранить • Esc: отмена • B: фон • F: рамка"));        if (profileEnabled_) {
            ++paints_;
            paintTimeNs_ += timer.nsecsElapsed();
        }

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
        auto *calibrateAction = menu.addAction(QStringLiteral("Калибровка перспективы"));
        auto *settingsAction = menu.addAction(QStringLiteral("Настройки CRT"));
        auto *animationAction = menu.addAction(QStringLiteral("Настройки анимации"));
        menu.addSeparator();
        auto *quitAction = menu.addAction(QStringLiteral("Закрыть виджет"));
        QAction *selected = menu.exec(e->globalPos());
        if (selected == settingsAction) settings();
        else if (selected == calibrateAction) edit();
        else if (selected == animationAction) animationSettings();
        else if (selected == quitAction) QApplication::quit();
        e->accept();
    }
    void keyPressEvent(QKeyEvent *e) override {
        if (!editing_) return;
        switch (e->key()) {
            case Qt::Key_Escape:
                cfg_.quad = before_;
                finishCalibration(false);
                break;
            case Qt::Key_Enter:
            case Qt::Key_Return:
                finishCalibration(true);
                break;
            case Qt::Key_B:
                cfg_.backgroundAlpha = cfg_.backgroundAlpha ? 0 : 35; redraw(); break;
            case Qt::Key_F:
                cfg_.frame = !cfg_.frame; redraw(); break;
            default: QWidget::keyPressEvent(e);
        }
    }

private:
    void finishCalibration(bool save) {
        if (save) cfg_.save();
        editing_ = false;
        selected_ = -1;
        if (save) redraw();
        applyMode();
        // No native window recreation: only EWMH above/below and geometry
        // change, preserving its original all-desktops assignment.
        for (const int delayMs : {0, 250, 900}) {
            QTimer::singleShot(delayMs, this, [this] {
                if (!editing_) requestWindowMode();
            });
        }
    }

    void reassertWorkspaceStickiness() {
        if (isVisible()) desktop::keepOnEveryWorkspace(winId());
    }
    void restoreWorkspaceStickiness() {
        if (isVisible() && !desktop::isOnEveryWorkspace(winId()))
            reassertWorkspaceStickiness();
    }

    void scheduleNextFlicker(qint64 now) {
        const int interval = crt::flickerIntervalMs(cfg_.flickerFrequency);
        // A small jitter retains the analogue feel without changing the
        // approximate user-selected flicker frequency.
        const int percent = QRandomGenerator::global()->bounded(90, 111);
        flickerDue_ = interval > 0 ? now + qint64(interval) * percent / 100 : 0;
    }

    void rebuildMatrix() {
        if (matrix_.active()) {
            // Only the far-right 108px actually contains typing glyphs.
            // CRT glow on this strip is about 4.4x cheaper than processing
            // all 480 columns every tick while the user types.
            const QImage glyphs = matrix_.stripImage(cfg_.green, cfg_.brightness);
            const QImage strip = crtComposite(glyphs, Qt::transparent, 0,
                                              cfg_.glow, cfg_.pixelScale, false,
                                              cfg_.dim);
            QImage full(W, H, QImage::Format_ARGB32_Premultiplied);
            full.fill(Qt::transparent);
            QPainter p(&full);
            p.drawImage(QPoint(MatrixRain::StripX, 0), strip);
            p.end();
            matrixFrame_ = std::move(full);
        } else {
            matrixFrame_ = QImage();
        }
        projectionDirty_ = true;
        if (profileEnabled_) ++matrixRebuilds_;
        matrixGreen_ = cfg_.green;
        matrixBrightness_ = cfg_.brightness;
        matrixGlow_ = cfg_.glow;
        matrixPixelScale_ = cfg_.pixelScale;
    }
    void redraw() {
        QElapsedTimer timer;
        if (profileEnabled_) timer.start();
        frame_ = canvas_.render(cfg_, animator_.pose(), clockIntensity_,
                                joyTail_, musicStreamPhase_, idleBreathPhase_,
                                animator_.giftVisual(elapsed_.elapsed()),
                                animator_.motionVisual(elapsed_.elapsed()));
        projectionDirty_ = true;
        // Matrix content already rebuilds in matrix_.advance(). Animation
        // redraws must NOT redo a second expensive CRT glow pass on typing
        // glyphs unless their visual settings have actually changed.
        if (matrix_.active() &&
            (matrixGreen_ != cfg_.green ||
             matrixBrightness_ != cfg_.brightness ||
             matrixGlow_ != cfg_.glow ||
             matrixPixelScale_ != cfg_.pixelScale))
            rebuildMatrix();
        if (profileEnabled_) {
            ++renders_;
            renderTimeNs_ += timer.nsecsElapsed();
        }
        update();
    }
    void applyMode() {
        // No Qt flag changes or remapping: one XID survives Enter and Esc.
        desktop::applyWindowMode(*this, editing_,
                                 editing_ ? screen_ : bounds(cfg_));
        for (int delayMs : {0, 250, 900}) {
            QTimer::singleShot(delayMs, this, [this] {
                if (isVisible()) requestWindowMode();
            });
        }
    }

    void requestWindowMode() {
        if (!isVisible()) return;
        desktop::keepOnEveryWorkspace(winId());
        desktop::requestStacking(winId(), editing_);
    }

    QRect screen_;
    Settings cfg_;
    perspective::Handles before_{};
    QImage frame_;
    CanvasCache canvas_;
    perspective::RasterCache perspectiveCache_;
    QImage projectedFrame_;
    QImage staticOverlay_;
    bool projectionDirty_ = true;
    bool staticOverlayDirty_ = true;
    int cachedScanlineStrength_ = -1;
    int cachedGrilleStrength_ = -1;
    MatrixRain matrix_;
    pet::JoyTail joyTail_;
    QImage matrixFrame_;
    QColor matrixGreen_;
    int matrixBrightness_ = -1;
    int matrixGlow_ = -1;
    int matrixPixelScale_ = -1;
    QTimer tick_;
    QElapsedTimer elapsed_;
    std::unique_ptr<KeyboardActivity> keyboard_;
    pet::Animator animator_;
    QPointF last_;
    int selected_ = -1;
    quint64 keyPressCount_ = 0;
    qint64 nextMusicCheck_ = 0;
    qint64 nextWorkspaceCheck_ = 0;
    qint64 rippleDue_ = 10000;
    qint64 rippleStart_ = -1;
    qint64 flickerDue_ = 60000;
    qint64 flickerStart_ = -1;
    double clockIntensity_ = 1.0;
    QString lastMinute_;
    bool musicPlaying_ = false;
    int musicStreamPhase_ = -1;
    int idleBreathPhase_ = -1;
    QDialog *settingsWindow_ = nullptr;
    QDialog *animationSettingsWindow_ = nullptr;
    quint64 previewSerial_ = 0;
    const bool profileEnabled_ = qEnvironmentVariableIsSet("CRT_CLOCK_PROFILE");
    const bool legacyPerspective_ =
        qEnvironmentVariableIsSet("CRT_CLOCK_LEGACY_PERSPECTIVE");
    qint64 nextProfileReport_ = 6000;
    quint64 renders_ = 0;
    quint64 paints_ = 0;
    quint64 matrixRebuilds_ = 0;
    quint64 warpRebuilds_ = 0;
    quint64 renderTimeNs_ = 0;
    quint64 paintTimeNs_ = 0;
    quint64 warpTimeNs_ = 0;
    bool editing_ = false;
};
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setApplicationName("crt-clock");
    app.setQuitOnLastWindowClosed(false);

    const QStringList args = app.arguments();
    QString command = args.contains("--calibrate") ? "calibrate"
                    : args.contains("--settings") ? "settings"
                    : args.contains("--animations") ? "animations"
                    : args.contains("--quit") ? "quit" : "start";
    for (const QString &arg : args) {
        if (arg.startsWith(QStringLiteral("--preview-gift=")))
            command = arg.mid(2); // same command format for IPC and startup
    }
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
                else if (cmd == "animations") clock.animationSettings();
                else if (cmd == "quit") QApplication::quit();
                else if (cmd.startsWith(QStringLiteral("preview-gift=")))
                    clock.previewGift(cmd.mid(QStringLiteral("preview-gift=").size()));
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
        else if (command == "animations") clock.animationSettings();
        else if (command.startsWith(QStringLiteral("preview-gift=")))
            clock.previewGift(command.mid(QStringLiteral("preview-gift=").size()));
    }
    return app.exec();
}
