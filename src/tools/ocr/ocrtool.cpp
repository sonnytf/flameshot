// SPDX-License-Identifier: GPL-3.0-or-later
#include "ocrtool.h"
#include "core/flameshotdaemon.h"
#include "utils/abstractlogger.h"
#include "utils/colorutils.h"
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QMimeData>
#include <QPainter>
#include <QPointer>
#include <QProcess>
#include <QRegularExpression>
#include <QTimer>

namespace {
class OcrClipboardMimeData : public QMimeData
{
public:
    OcrClipboardMimeData(const QString& text, QEventLoop* loop)
        : m_loop(loop)
    {
        setText(text);
    }

protected:
    QVariant retrieveData(const QString& mimeType,
                          QMetaType type) const override
    {
        if (!m_notified && m_loop != nullptr) {
            m_notified = true;
            QTimer::singleShot(30, m_loop, &QEventLoop::quit);
        }
        return QMimeData::retrieveData(mimeType, type);
    }

private:
    mutable bool m_notified{ false };
    QPointer<QEventLoop> m_loop;
};
} // namespace

OcrTool::OcrTool(QObject* parent)
    : AbstractActionTool(parent)
{
}

QIcon OcrTool::icon(const QColor& background, bool inEditor) const
{
    Q_UNUSED(inEditor)
    const int s = 64;
    QPixmap pix(s, s);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    const QColor col = ColorUtils::colorIsDark(background) ? Qt::white : Qt::black;
    QPen pen(col, 4.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    const int m = 8;
    const int b = 12;
    p.drawLine(m, m + b, m, m);
    p.drawLine(m, m, m + b, m);
    p.drawLine(s - m - b, m, s - m, m);
    p.drawLine(s - m, m, s - m, m + b);
    p.drawLine(m, s - m - b, m, s - m);
    p.drawLine(m, s - m, m + b, s - m);
    p.drawLine(s - m - b, s - m, s - m, s - m);
    p.drawLine(s - m, s - m - b, s - m, s - m);

    QFont f = p.font();
    f.setPixelSize(18);
    f.setBold(true);
    p.setFont(f);
    p.drawText(QRect(0, 0, s, s), Qt::AlignCenter, QStringLiteral("OCR"));

    return QIcon(pix);
}

void OcrTool::pressed(CaptureContext& context)
{
    const QPixmap cropped = context.selectedScreenshotArea();
    if (cropped.isNull()) {
        return;
    }

    QImage img = cropped.toImage().convertToFormat(QImage::Format_RGB32);
    if (img.isNull()) {
        return;
    }

    if (img.height() < 80 || img.width() < 160) {
        img = img.scaled(img.width() * 2,
                         img.height() * 2,
                         Qt::KeepAspectRatio,
                         Qt::SmoothTransformation);
    }

    const int pad = 16;
    QImage paddedImg(img.width() + 2 * pad,
                     img.height() + 2 * pad,
                     QImage::Format_RGB32);
    paddedImg.fill(img.pixelColor(0, 0));
    {
        QPainter padPainter(&paddedImg);
        padPainter.drawImage(pad, pad, img);
    }

    const QString tempImg =
      QStringLiteral("%1/flameshot_ocr_%2.png")
        .arg(QDir::tempPath(),
             QDateTime::currentDateTime().toString(
               QStringLiteral("yyyyMMdd_hhmmss_zzz")));
    if (!paddedImg.save(tempImg, "PNG")) {
        return;
    }

    QProcess proc;
    QStringList args;
    args << tempImg << QStringLiteral("stdout") << QStringLiteral("-l")
         << QStringLiteral("chi_tra+eng") << QStringLiteral("--psm")
         << QStringLiteral("6");
    proc.start(QStringLiteral("tesseract"), args);
    if (!proc.waitForFinished(10000) ||
        proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
        proc.kill();
        proc.waitForFinished(1000);
        args.clear();
        args << tempImg << QStringLiteral("stdout") << QStringLiteral("-l")
             << QStringLiteral("eng") << QStringLiteral("--psm")
             << QStringLiteral("6");
        proc.start(QStringLiteral("tesseract"), args);
        if (!proc.waitForFinished(10000)) {
            proc.kill();
            proc.waitForFinished(1000);
        }
    }

    QString text = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    QFile::remove(tempImg);

    static const QRegularExpression cjkSpaceRe(QStringLiteral(
      "([\\x{3000}-\\x{303f}\\x{4e00}-\\x{9fff}\\x{ff00}-\\x{ffef}])[ "
      "\\t]+(?=[\\x{3000}-\\x{303f}\\x{4e00}-\\x{9fff}\\x{ff00}-\\x{ffef}])"));
    text.replace(cjkSpaceRe, QStringLiteral("\\1"));

    context.request.removeTask(CaptureRequest::COPY);

    if (text.isEmpty()) {
        AbstractLogger::info() << tr("未在截圖範圍內辨識出文字");
        if (FlameshotDaemon::instance() != nullptr) {
            FlameshotDaemon::instance()->sendTrayNotification(
              tr("未在截圖範圍內辨識出文字"), QStringLiteral("Flameshot OCR"));
        }
        emit requestAction(REQ_CLEAR_SELECTION);
        emit requestAction(REQ_CAPTURE_DONE_OK);
        emit requestAction(REQ_CLOSE_GUI);
        return;
    }

    QEventLoop loop;
    auto* mimeData = new OcrClipboardMimeData(text, &loop);
    QGuiApplication::clipboard()->setMimeData(mimeData);

    emit requestAction(REQ_HIDE_GUI);

    QTimer::singleShot(500, &loop, &QEventLoop::quit);
    loop.exec();

    AbstractLogger::info() << tr("OCR 文字已複製到剪貼簿");
    if (FlameshotDaemon::instance() != nullptr) {
        FlameshotDaemon::instance()->sendTrayNotification(
          tr("OCR 文字已複製到剪貼簿"), QStringLiteral("Flameshot OCR"));
    }

    emit requestAction(REQ_CLEAR_SELECTION);
    emit requestAction(REQ_CAPTURE_DONE_OK);
    emit requestAction(REQ_CLOSE_GUI);
}
