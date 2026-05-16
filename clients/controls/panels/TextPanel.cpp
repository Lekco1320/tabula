/**
 * @file TextPanel.cpp
 * @brief Panel to configure and draw text.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-16
 * @license MIT
 */

#include <QByteArray>
#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QTextCursor>
#include <QtGlobal>
#include <epd_core/common.h>
#include <epd_gfx/font.h>
#include <epd_gfx/text.h>

#include "controls/panels/TextPanel.hpp"
#include "controls/widgets/ColorButton.hpp"
#include "controls/widgets/FlowButton.hpp"
#include "project/EpdStreamAdapter.hpp"

LEKCO_BEGIN_NAMESPACE

SingleLineTextEdit::SingleLineTextEdit(QWidget* parent)
    : QPlainTextEdit(parent)
{
    setFixedHeight(fontMetrics().lineSpacing() * 3 + 12);
    setLineWrapMode(QPlainTextEdit::WidgetWidth);
}

void SingleLineTextEdit::insertFromMimeData(const QMimeData* source)
{
    insertPlainText(source ? source->text().simplified() : QString());
}

void SingleLineTextEdit::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        event->ignore();
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
    normalizeText();
}

void SingleLineTextEdit::focusOutEvent(QFocusEvent* event)
{
    normalizeText();
    QPlainTextEdit::focusOutEvent(event);
}

void SingleLineTextEdit::normalizeText()
{
    const QString text       = toPlainText();
    const QString normalized = QString(text).replace(QRegularExpression(QStringLiteral("[\\r\\n\\t]+")),
        QStringLiteral(" "));
    if (text == normalized) {
        return;
    }

    const QTextCursor cursor = textCursor();
    const int position       = cursor.position();
    blockSignals(true);
    setPlainText(normalized);
    QTextCursor next = textCursor();
    next.setPosition(qMin(position, normalized.length()));
    setTextCursor(next);
    blockSignals(false);
    emit textChanged();
}

TextPanel::TextPanel(const QString& title, FontProvider* fontProvider, QWidget* parent)
    : ControlPanel(title, parent)
    , m_fontProvider(fontProvider)
    , m_font(new QComboBox(this))
    , m_size(new QComboBox(this))
    , m_text(new SingleLineTextEdit(this))
    , m_x(new QSpinBox(this))
    , m_y(new QSpinBox(this))
    , m_colorBtn(new ColorButton(this))
    , m_background(new ColorButton(ColorButton::Mode::Background, this))
    , m_flow(new FlowButton(this))
    , m_spacing(new QSpinBox(this))
    , m_previewBtn(new QCheckBox(QStringLiteral("Preview"), this))
    , m_draw(new QPushButton(QStringLiteral("Draw"), this))
{
    m_previewBtn->setStyleSheet(QStringLiteral("QCheckBox { spacing: 4px; }"));
    connect(m_previewBtn, &QCheckBox::checkStateChanged, [this](int checked) {
        m_enablePreview = (bool)checked;
        updatePreview();
    });

    m_spacing->setRange(-16, 64);
    m_spacing->setValue(0);

    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Font:"), m_font), 0, 0, 1, 2);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Size:"), m_size), 1, 0, 1, 2);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("X:"), m_x), 2, 0);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Y:"), m_y), 2, 1);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Fore Color:"), m_colorBtn), 3, 0);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Back Color:"), m_background), 3, 1);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Space:"), m_spacing), 4, 0);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Text Flow:"), m_flow), 4, 1);
    m_root->addWidget(m_text, 5, 0, 1, 2);
    m_root->addWidget(m_previewBtn, 6, 0);
    m_root->addWidget(m_draw, 6, 1, Qt::AlignRight);

    connect(m_font, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        refreshSizes();
    });
    connect(m_size, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        updateControls();
        updatePreview();
    });
    connect(m_text, &QPlainTextEdit::textChanged, this, [this]() {
        updateControls();
        updatePreview();
    });
    connect(m_x, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) {
        updatePreview();
    });
    connect(m_y, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) {
        updatePreview();
    });
    connect(m_colorBtn, &ColorButton::colorChanged, this, &TextPanel::updatePreview);
    connect(m_background, &ColorButton::backgroundColorChanged, this, &TextPanel::updatePreview);
    connect(m_flow, &FlowButton::flowChanged, this, &TextPanel::updatePreview);
    connect(m_spacing, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) {
        updatePreview();
    });
    connect(m_draw, &QPushButton::clicked, this, &TextPanel::updateDraw);

    refreshFonts();
}

void TextPanel::updatePreview() const
{
    if (!canDraw()) {
        emit refreshRequested();
        return;
    }

    ControlPanel::updatePreview();
}

void TextPanel::updateRange(epd_gfx_canvas_t canvas)
{
    const uint16_t width  = epd_gfx_canvas_get_logical_width(canvas);
    const uint16_t height = epd_gfx_canvas_get_logical_height(canvas);

    m_x->setRange(1, width);
    m_y->setRange(1, height);
}

void TextPanel::refreshFonts()
{
    const QString currentFileName = m_font->currentData().toString();

    m_font->blockSignals(true);
    m_font->clear();

    if (m_fontProvider) {
        const QVector<FontResourceInfo> fonts = m_fontProvider->fonts();
        for (const FontResourceInfo& font : fonts) {
            m_font->addItem(font.displayName, font.fileName);
        }
    }

    if (m_font->count() == 0) {
        m_font->addItem(QStringLiteral("No Fonts"), QString());
    } else {
        const int index = m_font->findData(currentFileName);
        m_font->setCurrentIndex(index >= 0 ? index : 0);
    }
    m_font->blockSignals(false);

    refreshSizes();
}

DrawFunc TextPanel::drawFunc() const
{
    const QString             fontFileName = m_font->currentData().toString();
    const QString             text         = m_text->toPlainText();
    const int                 size         = m_size->currentData().toInt();
    const int                 x            = m_x->value();
    const int                 y            = m_y->value();
    const epd_gfx_color_t     color        = m_colorBtn->currentColor();
    const epd_gfx_bg_color_t  background   = m_background->currentBackgroundColor();
    const epd_gfx_text_flow_t flow         = m_flow->currentFlow();
    const int                 spacing      = m_spacing->value();
    FontProvider*             provider     = m_fontProvider;

    return [provider, fontFileName, text, size, x, y, color, background, flow, spacing](epd_gfx_canvas_t canvas) {
        if (!provider || fontFileName.isEmpty() || text.isEmpty() || size <= 0) {
            return EPD_ERR_INVALID_ARG;
        }

        FontResourceInfo info;
        if (!provider->font(fontFileName, &info)) {
            return EPD_ERR_NOT_FOUND;
        }

        QFile file(info.absolutePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return EPD_ERR_INVALID_STATE;
        }

        EpdStreamAdapter stream(&file);
        epd_gfx_font_t font = nullptr;
        epd_err_t ret = epd_gfx_font_load(stream.stream(), &font);
        if (ret != EPD_OK) {
            return ret;
        }

        const QByteArray utf8 = text.toUtf8();
        epd_gfx_text_style_t style;
        style.size           = static_cast<uint16_t>(size);
        style.color          = color;
        style.background     = background;
        style.flow           = flow;
        style.letter_spacing = static_cast<int16_t>(spacing);

        ret = epd_gfx_canvas_draw_utf8(canvas, font, utf8.constData(), epd_gfx_point_t{
            static_cast<uint16_t>(x),
            static_cast<uint16_t>(y),
        }, &style, nullptr);

        epd_gfx_font_destroy(font);
        return ret;
    };
}

bool TextPanel::canDraw() const
{
    return !m_font->currentData().toString().isEmpty()
        && m_size->currentData().toInt() > 0
        && !m_text->toPlainText().isEmpty();
}

void TextPanel::refreshSizes()
{
    const QString fileName = m_font->currentData().toString();

    m_size->blockSignals(true);
    m_size->clear();

    if (m_fontProvider && !fileName.isEmpty()) {
        const QVector<uint16_t> sizes = m_fontProvider->sizes(fileName);
        for (uint16_t size : sizes) {
            m_size->addItem(QString::number(size), size);
        }
    }

    if (m_size->count() == 0) {
        m_size->addItem(QStringLiteral("No Sizes"), 0);
    } else {
        m_size->setCurrentIndex(0);
    }
    m_size->blockSignals(false);

    updateControls();
    updatePreview();
}

void TextPanel::updateControls()
{
    const bool hasFont = !m_font->currentData().toString().isEmpty();
    const bool hasSize = m_size->currentData().toInt() > 0;
    const bool enabled = hasFont && hasSize;

    m_size->setEnabled(hasFont);
    m_text->setEnabled(enabled);
    m_x->setEnabled(enabled);
    m_y->setEnabled(enabled);
    m_colorBtn->setEnabled(enabled);
    m_background->setEnabled(enabled);
    m_flow->setEnabled(enabled);
    m_spacing->setEnabled(enabled);
    m_previewBtn->setEnabled(canDraw());
    m_draw->setEnabled(canDraw());
}

LEKCO_END_NAMESPACE
