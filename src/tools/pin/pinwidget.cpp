// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "pinwidget.h"
#include <QDateTime>
#include <QDir>
#include <QDrag>
#include <QGraphicsOpacityEffect>
#include <QMimeData>
#include <QPainter>
#include <QUrl>

#include "core/qguiappcurrentscreen.h"
#include "utils/abstractlogger.h"
#include "utils/confighandler.h"
#include "utils/globalvalues.h"
#include "utils/screenshotsaver.h"

#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QMenu>
#include <QPinchGesture>
#include <QScreen>
#include <QShortcut>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QWindow>

namespace {
constexpr int MARGIN = 7;
constexpr int BLUR_RADIUS = 2 * MARGIN;
constexpr qreal STEP = 0.03;
constexpr qreal MIN_SIZE = 100.0;
}


namespace {
class PinCapsuleButton : public QLabel
{
public:
    explicit PinCapsuleButton(const QString& text, QWidget* parent = nullptr)
        : QLabel(text, parent)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setCursor(Qt::OpenHandCursor);
        QFont f = font();
        f.setPixelSize(12);
        f.setWeight(QFont::Medium);
        setFont(f);

        QFontMetrics fm(f);
        const int textW = fm.horizontalAdvance(text);
        const int w = 10 + 8 + 6 + textW + 12;
        const int h = 26;
        setFixedSize(w, h);
    }

protected:
    void enterEvent(QEnterEvent* e) override
    {
        m_hovered = true;
        update();
        QLabel::enterEvent(e);
    }

    void leaveEvent(QEvent* e) override
    {
        m_hovered = false;
        update();
        QLabel::leaveEvent(e);
    }

    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

        const QRectF r(0.75, 0.75, width() - 1.5, height() - 1.5);
        const qreal radius = r.height() / 2.0;

        const QColor bg = m_hovered ? QColor(38, 38, 44, 235) : QColor(20, 20, 24, 195);
        const QColor border = m_hovered ? QColor(255, 255, 255, 130) : QColor(255, 255, 255, 45);

        p.setPen(QPen(border, 1.0));
        p.setBrush(bg);
        p.drawRoundedRect(r, radius, radius);

        const QColor dotColor = m_hovered ? QColor(255, 255, 255, 220) : QColor(255, 255, 255, 160);
        p.setPen(Qt::NoPen);
        p.setBrush(dotColor);
        const qreal dotRadius = 1.1;
        const qreal startX = 10.0;
        const qreal startY = (height() - (2 * 4.0)) / 2.0;
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 2; ++col) {
                p.drawEllipse(QPointF(startX + col * 4.0, startY + row * 4.0), dotRadius, dotRadius);
            }
        }

        p.setPen(m_hovered ? Qt::white : QColor(240, 240, 245));
        p.setFont(font());
        const QRect textRect(24, 0, width() - 24 - 10, height());
        p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text());
    }

private:
    bool m_hovered{ false };
};
} // namespace

PinWidget::PinWidget(const QPixmap& pixmap,
                     const QRect& geometry,
                     QWidget* parent)
  : QWidget(parent)
  , m_pixmap(pixmap)
  , m_layout(new QVBoxLayout(this))
  , m_label(new QLabel())
  , m_shadowEffect(new QGraphicsDropShadowEffect(this))
{
    setWindowIcon(QIcon(GlobalValues::iconPath()));
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint |
                   Qt::Dialog);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("flameshot-pin");
    ConfigHandler conf;
    m_baseColor = conf.uiColor();
    m_hoverColor = conf.contrastUiColor();
    QString closePinnedCaptureShortcut =
      conf.shortcut("TYPE_CLOSE_ACTIVE_PIN_WINDOW");

    m_layout->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);

    m_shadowEffect->setColor(m_baseColor);
    m_shadowEffect->setBlurRadius(BLUR_RADIUS);
    m_shadowEffect->setOffset(0, 0);
    setGraphicsEffect(m_shadowEffect);

    m_labelOpacityEffect = new QGraphicsOpacityEffect(m_label);
    m_labelOpacityEffect->setOpacity(m_opacity);
    m_label->setGraphicsEffect(m_labelOpacityEffect);

    m_label->setPixmap(m_pixmap);
    m_layout->addWidget(m_label);

    m_dragBadge = new PinCapsuleButton(QStringLiteral("Drag Me"), this);
    m_dragBadge->show();

    new QShortcut(
      QKeySequence(closePinnedCaptureShortcut), this, SLOT(close()));
    new QShortcut(Qt::Key_Escape, this, SLOT(close()));

    qreal devicePixelRatio = 1;
    QScreen* currentScreen = QGuiAppCurrentScreen().currentScreen();
    if (currentScreen != nullptr) {
        devicePixelRatio = currentScreen->devicePixelRatio();
    }

    const int margin =
      static_cast<int>(static_cast<double>(MARGIN) * devicePixelRatio);
    QRect adjusted_pos = geometry + QMargins(margin, margin, margin, margin);
    setGeometry(adjusted_pos);

    if (currentScreen != nullptr) {
        QPoint topLeft = currentScreen->geometry().topLeft();
        adjusted_pos.setX((adjusted_pos.x() - topLeft.x()) / devicePixelRatio +
                          topLeft.x());

        adjusted_pos.setY((adjusted_pos.y() - topLeft.y()) / devicePixelRatio +
                          topLeft.y());
        adjusted_pos.setWidth(adjusted_pos.size().width() / devicePixelRatio);
        adjusted_pos.setHeight(adjusted_pos.size().height() / devicePixelRatio);
        resize(0, 0);
        move(adjusted_pos.x(), adjusted_pos.y());
    }

    grabGesture(Qt::PinchGesture);

    this->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this,
            &QWidget::customContextMenuRequested,
            this,
            &PinWidget::showContextMenu);
}

void PinWidget::closePin()
{
    update();
    close();
}
bool PinWidget::scrollEvent(QWheelEvent* event)
{
    const auto phase = event->phase();
    if (phase == Qt::ScrollPhase::ScrollUpdate
#if defined(Q_OS_UNIX) || defined(Q_OS_WINDOWS)
        || phase == Qt::ScrollPhase::NoScrollPhase
#endif
    ) {
        const auto angle = event->angleDelta();
        if (angle.y() == 0) {
            return true;
        }
        m_currentStepScaleFactor = angle.y() > 0
                                     ? m_currentStepScaleFactor + STEP
                                     : m_currentStepScaleFactor - STEP;
        m_expanding = m_currentStepScaleFactor >= 1.0;
    }
#if defined(Q_OS_MACOS)
    // ScrollEnd is currently supported only on Mac OSX
    if (phase == Qt::ScrollPhase::ScrollEnd) {
#else
    else {
#endif
        m_scaleFactor *= m_currentStepScaleFactor;
        m_currentStepScaleFactor = 1.0;
        m_expanding = false;
    }

    m_sizeChanged = true;
    update();
    return true;
}

void PinWidget::enterEvent(QEnterEvent*)
{
    m_shadowEffect->setColor(m_hoverColor);
}

void PinWidget::leaveEvent(QEvent*)
{
    m_shadowEffect->setColor(m_baseColor);
}

void PinWidget::mouseDoubleClickEvent(QMouseEvent*)
{
    closePin();
}

void PinWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateDragBadgePosition();
}

void PinWidget::updateDragBadgePosition()
{
    if (m_dragBadge != nullptr) {
        m_dragBadge->adjustSize();
        const int x = (width() - m_dragBadge->width()) / 2;
        const int y = std::max(6, height() - m_dragBadge->height() - 14);
        m_dragBadge->move(x, y);
        m_dragBadge->raise();
    }
}

void PinWidget::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        const bool ctrlPressed = bool(e->modifiers() & Qt::ControlModifier);
        const bool clickBadge = (m_dragBadge != nullptr &&
                                 m_dragBadge->geometry().contains(e->pos()));
        if (ctrlPressed || clickBadge) {
            m_pendingFileDrag = true;
            m_dragStartPos = e->pos();
            if (m_dragBadge != nullptr) {
                m_dragBadge->setCursor(Qt::ClosedHandCursor);
            }
            e->accept();
            return;
        }

        if (QWindow* window = windowHandle(); window != nullptr) {
            window->startSystemMove();
            return;
        }
        m_dragPosition = e->globalPosition().toPoint() - frameGeometry().topLeft();
        e->accept();
        return;
    }
    QWidget::mousePressEvent(e);
}

void PinWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (m_pendingFileDrag && (e->buttons() & Qt::LeftButton)) {
        if ((e->pos() - m_dragStartPos).manhattanLength() >= 4) {
            m_pendingFileDrag = false;
            const bool keepOpen = bool(e->modifiers() & Qt::ShiftModifier);
            startFileDrag(keepOpen);
            if (m_dragBadge != nullptr) {
                m_dragBadge->setCursor(Qt::OpenHandCursor);
            }
            e->accept();
            return;
        }
    }
    if ((e->buttons() & Qt::LeftButton) && !m_pendingFileDrag) {
        move(e->globalPosition().toPoint() - m_dragPosition);
        e->accept();
        return;
    }
    QWidget::mouseMoveEvent(e);
}

void PinWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        m_pendingFileDrag = false;
        if (m_dragBadge != nullptr) {
            m_dragBadge->setCursor(Qt::OpenHandCursor);
        }
    }
    QWidget::mouseReleaseEvent(e);
}

void PinWidget::startFileDrag(bool keepOpen)
{
    if (m_pixmap.isNull()) {
        return;
    }

    if (m_dragBadge != nullptr) {
        m_dragBadge->hide();
    }
    if (m_labelOpacityEffect != nullptr) {
        m_labelOpacityEffect->setOpacity(0.15);
    }
    if (m_shadowEffect != nullptr) {
        m_shadowEffect->setEnabled(false);
    }

    const QString tempDir = QDir::tempPath();
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_hh-mm-ss-zzz"));
    const QString filePath = QStringLiteral("%1/flameshot_pin_%2.png").arg(tempDir, timestamp);

    if (!m_pixmap.save(filePath, "PNG")) {
        AbstractLogger::error() << tr("Failed to save dragged image to temporary file");
        if (m_dragBadge != nullptr) {
            m_dragBadge->show();
        }
        if (m_labelOpacityEffect != nullptr) {
            m_labelOpacityEffect->setOpacity(m_opacity);
        }
        if (m_shadowEffect != nullptr) {
            m_shadowEffect->setEnabled(true);
        }
        return;
    }

    auto* drag = new QDrag(this);
    auto* mimeData = new QMimeData();

    QList<QUrl> urls;
    urls.append(QUrl::fromLocalFile(filePath));
    mimeData->setUrls(urls);
    mimeData->setImageData(m_pixmap.toImage());

    drag->setMimeData(mimeData);

    QPixmap dragThumbnail = m_pixmap;
    if (dragThumbnail.width() > 300 || dragThumbnail.height() > 300) {
        dragThumbnail = dragThumbnail.scaled(300, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    drag->setPixmap(dragThumbnail);
    drag->setHotSpot(QPoint(std::min(dragThumbnail.width() / 2, 24), std::min(dragThumbnail.height() / 2, 24)));

    const Qt::DropAction action = drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);

    if (action != Qt::IgnoreAction && !keepOpen) {
        closePin();
    } else {
        if (m_dragBadge != nullptr) {
            m_dragBadge->show();
        }
        if (m_labelOpacityEffect != nullptr) {
            m_labelOpacityEffect->setOpacity(m_opacity);
        }
        if (m_shadowEffect != nullptr) {
            m_shadowEffect->setEnabled(true);
        }
    }
}

void PinWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_0) {
        m_opacity = 1.0;
    } else if (event->key() == Qt::Key_9) {
        m_opacity = 0.9;
    } else if (event->key() == Qt::Key_8) {
        m_opacity = 0.8;
    } else if (event->key() == Qt::Key_7) {
        m_opacity = 0.7;
    } else if (event->key() == Qt::Key_6) {
        m_opacity = 0.6;
    } else if (event->key() == Qt::Key_5) {
        m_opacity = 0.5;
    } else if (event->key() == Qt::Key_4) {
        m_opacity = 0.4;
    } else if (event->key() == Qt::Key_3) {
        m_opacity = 0.3;
    } else if (event->key() == Qt::Key_2) {
        m_opacity = 0.2;
    } else if (event->key() == Qt::Key_1) {
        m_opacity = 0.1;
    }

    if (m_labelOpacityEffect != nullptr) {
        m_labelOpacityEffect->setOpacity(m_opacity);
    }
    m_shadowEffect->setColor(QColor(m_baseColor.red(), m_baseColor.green(), m_baseColor.blue(), static_cast<int>(255 * m_opacity)));
}
bool PinWidget::gestureEvent(QGestureEvent* event)
{
    if (QGesture* pinch = event->gesture(Qt::PinchGesture)) {
        pinchTriggered(static_cast<QPinchGesture*>(pinch));
    }
    return true;
}

void PinWidget::rotateLeft()
{
    m_sizeChanged = true;

    auto rotateTransform = QTransform().rotate(270);
    m_pixmap = m_pixmap.transformed(rotateTransform);
}

void PinWidget::rotateRight()
{
    m_sizeChanged = true;

    auto rotateTransform = QTransform().rotate(90);
    m_pixmap = m_pixmap.transformed(rotateTransform);
}

void PinWidget::increaseOpacity()
{
    m_opacity = std::min(1.0, m_opacity + 0.05);
    if (m_labelOpacityEffect != nullptr) {
        m_labelOpacityEffect->setOpacity(m_opacity);
    }
    m_shadowEffect->setColor(QColor(m_baseColor.red(), m_baseColor.green(), m_baseColor.blue(), static_cast<int>(255 * m_opacity)));
}

void PinWidget::decreaseOpacity()
{
    m_opacity = std::max(0.1, m_opacity - 0.05);
    if (m_labelOpacityEffect != nullptr) {
        m_labelOpacityEffect->setOpacity(m_opacity);
    }
    m_shadowEffect->setColor(QColor(m_baseColor.red(), m_baseColor.green(), m_baseColor.blue(), static_cast<int>(255 * m_opacity)));
}

bool PinWidget::event(QEvent* event)
{
    if (event->type() == QEvent::Gesture) {
        return gestureEvent(static_cast<QGestureEvent*>(event));
    } else if (event->type() == QEvent::Wheel) {
        return scrollEvent(static_cast<QWheelEvent*>(event));
    }
    return QWidget::event(event);
}

void PinWidget::paintEvent(QPaintEvent* event)
{
    if (m_sizeChanged) {
        const auto aspectRatio =
          m_expanding ? Qt::KeepAspectRatioByExpanding : Qt::KeepAspectRatio;
        const auto transformType = ConfigHandler().antialiasingPinZoom()
                                     ? Qt::SmoothTransformation
                                     : Qt::FastTransformation;
        const qreal iw = m_pixmap.width();
        const qreal ih = m_pixmap.height();
        const qreal nw = qBound(MIN_SIZE,
                                iw * m_currentStepScaleFactor * m_scaleFactor,
                                static_cast<qreal>(maximumWidth()));
        const qreal nh = qBound(MIN_SIZE,
                                ih * m_currentStepScaleFactor * m_scaleFactor,
                                static_cast<qreal>(maximumHeight()));

        const QPixmap pix = m_pixmap.scaled(nw, nh, aspectRatio, transformType);

        m_label->setPixmap(pix);
        adjustSize();
        m_sizeChanged = false;
    }
}

void PinWidget::pinchTriggered(QPinchGesture* gesture)
{
    const QPinchGesture::ChangeFlags changeFlags = gesture->changeFlags();
    if (changeFlags & QPinchGesture::ScaleFactorChanged) {
        m_currentStepScaleFactor = gesture->totalScaleFactor();
        m_expanding = m_currentStepScaleFactor > gesture->lastScaleFactor();
    }
    if (gesture->state() == Qt::GestureFinished) {
        m_scaleFactor *= m_currentStepScaleFactor;
        m_currentStepScaleFactor = 1;
        m_expanding = false;
    }
    m_sizeChanged = true;
    update();
}

void PinWidget::showContextMenu(const QPoint& pos)
{
    QMenu contextMenu(tr("Context menu"), this);

    QAction copyToClipboardAction(tr("Copy to clipboard"), this);
    connect(&copyToClipboardAction,
            &QAction::triggered,
            this,
            &PinWidget::copyToClipboard);
    contextMenu.addAction(&copyToClipboardAction);

    QAction saveToFileAction(tr("Save to file"), this);
    connect(
      &saveToFileAction, &QAction::triggered, this, &PinWidget::saveToFile);
    contextMenu.addAction(&saveToFileAction);

    contextMenu.addSeparator();

    QAction rotateRightAction(tr("Rotate Right"), this);
    connect(
      &rotateRightAction, &QAction::triggered, this, &PinWidget::rotateRight);
    contextMenu.addAction(&rotateRightAction);

    QAction rotateLeftAction(tr("Rotate Left"), this);
    connect(
      &rotateLeftAction, &QAction::triggered, this, &PinWidget::rotateLeft);
    contextMenu.addAction(&rotateLeftAction);

    QAction increaseOpacityAction(tr("Increase Opacity"), this);
    connect(&increaseOpacityAction,
            &QAction::triggered,
            this,
            &PinWidget::increaseOpacity);
    contextMenu.addAction(&increaseOpacityAction);

    QAction decreaseOpacityAction(tr("Decrease Opacity"), this);
    connect(&decreaseOpacityAction,
            &QAction::triggered,
            this,
            &PinWidget::decreaseOpacity);
    contextMenu.addAction(&decreaseOpacityAction);

    QAction closePinAction(tr("Close"), this);
    connect(&closePinAction, &QAction::triggered, this, &PinWidget::closePin);
    contextMenu.addSeparator();
    contextMenu.addAction(&closePinAction);

    contextMenu.exec(mapToGlobal(pos));
}

void PinWidget::copyToClipboard()
{
    saveToClipboard(m_pixmap);
}
void PinWidget::saveToFile()
{
    hide();
    saveToFilesystemGUI(m_pixmap);
    show();
}
