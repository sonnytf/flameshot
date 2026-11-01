// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "tools/abstractactiontool.h"

class OcrTool : public AbstractActionTool
{
    Q_OBJECT
public:
    explicit OcrTool(QObject* parent = nullptr);
    bool closeOnButtonPressed() const override { return true; }
    QIcon icon(const QColor& background, bool inEditor) const override;
    QString name() const override { return tr("OCR Text"); }
    QString description() const override { return tr("Extract text from selection via OCR"); }
    CaptureTool* copy(QObject* parent = nullptr) override { return new OcrTool(parent); }

protected:
    CaptureTool::Type type() const override { return CaptureTool::TYPE_OCR; }

public slots:
    void pressed(CaptureContext& context) override;
};
