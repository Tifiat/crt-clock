#include <QApplication>
#include <QColor>
#include <QDateTime>
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
        s.sync();
    }
};

QRect bounds(const Settings &cfg) {
    QPolygonF p;
    for (const auto &c : cfg.quad) p << c;
    return p.boundingRect().adjusted(-12, -12, 12, 12).toAlignedRect();
}

void centered(QPainter &p, QRectF r, QString text, QFont font, QColor color) {
    while (font.pixelSize() > 12 && QFontMetricsF(font).horizontalAdvance(text) > r.width() - 8)
        font.setPixelSize(font.pixelSize() - 1);
    p.setFont(font);
    const int align = Qt::AlignCenter | Qt::TextSingleLine;
    QColor glow = color;
    glow.setAlpha(25);
    p.setPen(glow);
    p.drawText(r.translated(-2, 0), align, text);
    p.drawText(r.translated( 2, 0), align, text);
    p.drawText(r.translated(0, -2), align, text);
    p.drawText(r.translated(0,  2), align, text);
    p.setPen(color);
    p.drawText(r, align, text);
}

QImage drawCanvas(const Settings &cfg, int pose) {
    QImage image(W, H, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    if (cfg.backgroundAlpha) {
        QColor bg = cfg.background;
        bg.setAlpha(cfg.backgroundAlpha);
        p.fillRect(image.rect(), bg);
    }

    QFont fTime(cfg.font);
    fTime.setPixelSize(97);
    fTime.setWeight(QFont::Bold);
    QFont fDate(cfg.font);
    fDate.setPixelSize(32);
    fDate.setWeight(QFont::Medium);
    QFont fPet(cfg.font);
    fPet.setPixelSize(45);

    QString face = cfg.pet;
    if (pose == 1) face = QStringLiteral("(づ｡－‿‿－｡)づ");
    if (pose == 2) face = QStringLiteral("(っ｡◕‿‿◕｡)っ");
    if (pose == 3) face = QStringLiteral("(づ｡◕‿‿◕｡)っ");

    const auto date = QDateTime::currentDateTime();
    centered(p, QRectF(12, 27, 456, 125), date.toString("HH:mm"), fTime, cfg.green);
    centered(p, QRectF(14, 154, 452, 52),
             QLocale(QLocale::English).toString(date.date(), "ddd, dd MMM yyyy").toUpper(),
             fDate, cfg.dim);
    centered(p, QRectF(10, 217, 460, 82), face, fPet, cfg.green);

    if (cfg.scanlines) {
        QColor c = cfg.background;
        c.setAlpha(12);
        p.setPen(c);
        for (int y = 1; y < H; y += 5) p.drawLine(0, y, W, y);
    }
    if (cfg.frame) {
        QColor c = cfg.dim;
        c.setAlpha(125);
        p.setPen(QPen(c, 1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(QRectF(1, 1, W - 2, H - 2));
    }
    return image;
}

class ClockWindow final : public QWidget {
public:
    ClockWindow() : screen_(QGuiApplication::primaryScreen()->geometry()), cfg_(screen_) {
        cfg_.load();
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setMouseTracking(true);
        connect(&tick_, &QTimer::timeout, this, [this] {
            if (remaining_ > 0) {
                if (--remaining_ == 0) pose_ = 0;
            } else if (QRandomGenerator::global()->bounded(14) == 0) {
                pose_ = 1 + QRandomGenerator::global()->bounded(3);
                remaining_ = 1 + QRandomGenerator::global()->bounded(2);
            }
            redraw();
        });
        tick_.start(1000);
        redraw();
    }

    void start() { editing_ = false; applyMode(); }
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
    int selected_ = -1, pose_ = 0, remaining_ = 0;
    bool editing_ = false;
};
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setApplicationName("crt-clock");

    const QStringList args = app.arguments();
    const QString command = args.contains("--calibrate") ? "calibrate" :
                            args.contains("--quit") ? "quit" : "start";
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
                else if (cmd == "quit") QApplication::quit();
                else clock.start();
                client->disconnectFromServer();
            });
            QObject::connect(client, &QLocalSocket::disconnected,
                             client, &QObject::deleteLater);
        }
    });
    if (command == "calibrate") clock.edit();
    else clock.start();
    return app.exec();
}
