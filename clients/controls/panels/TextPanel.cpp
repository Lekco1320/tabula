/**
 * @file TextPanel.cpp
 * @brief Panel to configure and draw text.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-16
 * @license MIT
 */

#include <QCheckBox>
#include <QComboBox>
#include <QFocusEvent>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTextCursor>
#include <QtGlobal>
#include <epd_core/common.h>

#include "controls/panels/TextPanel.hpp"
#include "controls/widgets/ColorButton.hpp"
#include "controls/widgets/TextAlignButton.hpp"
#include "controls/widgets/TextWrapButton.hpp"

LEKCO_BEGIN_NAMESPACE

BEGIN_NAMESPACE()

QString NormalizeTextBoxText(QString text)
{
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    text.replace(QLatin1Char('\t'), QLatin1Char(' '));
    return text;
}

END_NAMESPACE

TextBoxEdit::TextBoxEdit(QWidget* parent)
    : QPlainTextEdit(parent)
{
    setFixedHeight(fontMetrics().lineSpacing() * 4 + 12);
    setLineWrapMode(QPlainTextEdit::WidgetWidth);
}

void TextBoxEdit::insertFromMimeData(const QMimeData* source)
{
    insertPlainText(NormalizeTextBoxText(source ? source->text() : QString()));
}

void TextBoxEdit::focusOutEvent(QFocusEvent* event)
{
    normalizeText();
    QPlainTextEdit::focusOutEvent(event);
}

void TextBoxEdit::normalizeText()
{
    const QString text       = toPlainText();
    const QString normalized = NormalizeTextBoxText(text);
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
    , m_text(new TextBoxEdit(this))
    , m_x(new QSpinBox(this))
    , m_y(new QSpinBox(this))
    , m_width(new QSpinBox(this))
    , m_height(new QSpinBox(this))
    , m_lineSpacing(new QSpinBox(this))
    , m_charSpacing(new QSpinBox(this))
    , m_foreground(new ColorButton(this))
    , m_background(new ColorButton(ColorButton::Mode::Background, this))
    , m_align(new TextAlignButton(this))
    , m_wrap(new TextWrapButton(this))
    , m_previewBtn(createPreviewCheckBox())
    , m_draw(createDrawButton())
{
    m_width->setMinimum(1);
    m_height->setMinimum(1);
    m_lineSpacing->setRange(-16, 64);
    m_lineSpacing->setValue(0);
    m_charSpacing->setRange(-16, 64);
    m_charSpacing->setValue(0);

    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Font:"), m_font), 0, 0, 1, 2);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Size:"), m_size), 1, 0, 1, 2);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("X:"), m_x), 2, 0);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Y:"), m_y), 2, 1);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("W:"), m_width), 3, 0);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("H:"), m_height), 3, 1);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Line:"), m_lineSpacing), 4, 0);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Char:"), m_charSpacing), 4, 1);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Fore Color:"), m_foreground), 5, 0);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Back Color:"), m_background), 5, 1);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Align:"), m_align), 6, 0);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Wrap:"), m_wrap), 6, 1);
    m_root->addWidget(m_text, 7, 0, 1, 2);
    m_root->addWidget(m_previewBtn, 8, 0);
    m_root->addWidget(m_draw, 8, 1, Qt::AlignRight);

    connectSignals();
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
    setPointRange(m_x, m_y, canvas);
    updateBoxRange(true);
}

void TextPanel::refreshProjectResources()
{
    refreshFonts();
}

DrawFunc TextPanel::drawFunc() const
{
    FontProvider* provider = m_fontProvider;

    FontTextDrawRequest request;
    request.fileName                  = m_font->currentData().toString();
    request.text                      = m_text->toPlainText();
    request.box.x                     = static_cast<uint16_t>(m_x->value());
    request.box.y                     = static_cast<uint16_t>(m_y->value());
    request.box.width                 = static_cast<uint16_t>(m_width->value());
    request.box.height                = static_cast<uint16_t>(m_height->value());
    request.style.text.size           = static_cast<uint16_t>(m_size->currentData().toInt());
    request.style.text.color          = m_foreground->currentColor();
    request.style.text.background     = m_background->currentBackgroundColor();
    request.style.text.letter_spacing = static_cast<int16_t>(m_charSpacing->value());
    request.style.align               = m_align->currentAlign();
    request.style.line_spacing        = static_cast<int16_t>(m_lineSpacing->value());
    request.style.wrap                = m_wrap->wrap();

    return [provider, request](epd_gfx_canvas_t canvas) {
        if (!provider) {
            return EPD_ERR_INVALID_ARG;
        }
        return provider->drawText(canvas, request);
    };
}

void TextPanel::connectSignals()
{
    connect(m_font, qOverload<int>(&QComboBox::currentIndexChanged), this, &TextPanel::refreshSizes);
    connect(m_size, qOverload<int>(&QComboBox::currentIndexChanged), this, &TextPanel::updateTextState);
    connect(m_text, &QPlainTextEdit::textChanged, this, &TextPanel::updateTextState);
    connect(m_x, qOverload<int>(&QSpinBox::valueChanged), this, &TextPanel::updatePosition);
    connect(m_y, qOverload<int>(&QSpinBox::valueChanged), this, &TextPanel::updatePosition);
    connect(m_width, qOverload<int>(&QSpinBox::valueChanged), this, &TextPanel::updatePreview);
    connect(m_height, qOverload<int>(&QSpinBox::valueChanged), this, &TextPanel::updatePreview);
    connect(m_foreground, &ColorButton::colorChanged, this, &TextPanel::updatePreview);
    connect(m_background, &ColorButton::backgroundColorChanged, this, &TextPanel::updatePreview);
    connect(m_lineSpacing, qOverload<int>(&QSpinBox::valueChanged), this, &TextPanel::updatePreview);
    connect(m_charSpacing, qOverload<int>(&QSpinBox::valueChanged), this, &TextPanel::updatePreview);
    connect(m_align, &TextAlignButton::currentIndexChanged, this, &TextPanel::updatePreview);
    connect(m_wrap, &TextWrapButton::currentIndexChanged, this, &TextPanel::updatePreview);
}

void TextPanel::updateTextState()
{
    refreshTextRenderable();
    updateControls();
    updatePreview();
}

void TextPanel::updatePosition()
{
    updateBoxRange();
    updatePreview();
}

bool TextPanel::canUseTool() const
{
    return !m_font->currentData().toString().isEmpty()
        && m_size->currentData().toInt() > 0;
}

bool TextPanel::canDraw() const
{
    return canUseTool()
        && !m_text->toPlainText().isEmpty()
        && m_renderable;
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
        auto hasSizes = [this](const QString& fileName) {
            return m_fontProvider && !m_fontProvider->sizes(fileName).isEmpty();
        };
        if (index >= 0 && hasSizes(currentFileName)) {
            m_font->setCurrentIndex(index);
        } else {
            int usableIndex = 0;
            for (int i = 0; i < m_font->count(); ++i) {
                if (hasSizes(m_font->itemData(i).toString())) {
                    usableIndex = i;
                    break;
                }
            }
            m_font->setCurrentIndex(usableIndex);
        }
    }
    m_font->blockSignals(false);

    refreshSizes();
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

    refreshTextRenderable();
    updateControls();
    updatePreview();
}

void TextPanel::refreshTextRenderable()
{
    const QString fontFileName = m_font->currentData().toString();
    const int     size         = m_size->currentData().toInt();
    const QString text         = m_text->toPlainText();
    m_renderable               = text.isEmpty()
        || (m_fontProvider && m_fontProvider->hasRenderableText(fontFileName, static_cast<uint16_t>(size), text));
}

void TextPanel::updateBoxRange(bool expandToMax)
{
    const int maxWidth  = qMax(1, m_x->maximum() - m_x->value() + 1);
    const int maxHeight = qMax(1, m_y->maximum() - m_y->value() + 1);

    m_width->setRange(1, maxWidth);
    m_height->setRange(1, maxHeight);
    if (expandToMax) {
        m_width->setValue(maxWidth);
        m_height->setValue(maxHeight);
    }
}

void TextPanel::updateControls()
{
    const bool hasFont = !m_font->currentData().toString().isEmpty();
    const bool enabled = canUseTool();

    m_size->setEnabled(hasFont);
    m_text->setEnabled(enabled);
    m_x->setEnabled(enabled);
    m_y->setEnabled(enabled);
    m_width->setEnabled(enabled);
    m_height->setEnabled(enabled);
    m_lineSpacing->setEnabled(enabled);
    m_charSpacing->setEnabled(enabled);
    m_foreground->setEnabled(enabled);
    m_background->setEnabled(enabled);
    m_align->setEnabled(enabled);
    m_wrap->setEnabled(enabled);
    m_previewBtn->setEnabled(enabled);
    m_draw->setEnabled(canDraw());
}

LEKCO_END_NAMESPACE
