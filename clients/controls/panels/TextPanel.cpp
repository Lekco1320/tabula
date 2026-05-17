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
#include <QKeyEvent>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QTextCursor>
#include <QtGlobal>
#include <epd_core/common.h>

#include "controls/panels/TextPanel.hpp"
#include "controls/widgets/ColorButton.hpp"

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
    , m_spacing(new QSpinBox(this))
    , m_previewBtn(createPreviewCheckBox())
    , m_draw(createDrawButton())
{
    m_spacing->setRange(-16, 64);
    m_spacing->setValue(0);

    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Font:"), m_font), 0, 0, 1, 2);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Size:"), m_size), 1, 0, 1, 2);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("X:"), m_x), 2, 0);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Y:"), m_y), 2, 1);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Space:"), m_spacing), 3, 0, 1, 2);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Fore Color:"), m_colorBtn), 4, 0);
    m_root->addWidget(MakeLabeledWidget(this, QStringLiteral("Back Color:"), m_background), 4, 1);
    m_root->addWidget(m_text, 5, 0, 1, 2);
    m_root->addWidget(m_previewBtn, 6, 0);
    m_root->addWidget(m_draw, 6, 1, Qt::AlignRight);

    connect(m_font, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        refreshSizes();
    });
    connect(m_size, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        refreshTextRenderable();
        updateControls();
        updatePreview();
    });
    connect(m_text, &QPlainTextEdit::textChanged, this, [this]() {
        refreshTextRenderable();
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
    connect(m_spacing, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) {
        updatePreview();
    });
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

void TextPanel::refreshProjectResources()
{
    refreshFonts();
}

DrawFunc TextPanel::drawFunc() const
{
    FontProvider* provider = m_fontProvider;

    FontTextDrawRequest request;
    request.fileName             = m_font->currentData().toString();
    request.text                 = m_text->toPlainText();
    request.origin.x             = static_cast<uint16_t>(m_x->value());
    request.origin.y             = static_cast<uint16_t>(m_y->value());
    request.style.size           = static_cast<uint16_t>(m_size->currentData().toInt());
    request.style.color          = m_colorBtn->currentColor();
    request.style.background     = m_background->currentBackgroundColor();
    request.style.letter_spacing = static_cast<int16_t>(m_spacing->value());

    return [provider, request](epd_gfx_canvas_t canvas) {
        if (!provider) {
            return EPD_ERR_INVALID_ARG;
        }
        return provider->drawText(canvas, request);
    };
}

bool TextPanel::canDraw() const
{
    return canUseTool()
        && !m_text->toPlainText().isEmpty()
        && m_renderable;
}

bool TextPanel::canUseTool() const
{
    return !m_font->currentData().toString().isEmpty()
        && m_size->currentData().toInt() > 0;
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

void TextPanel::updateControls()
{
    const bool hasFont = !m_font->currentData().toString().isEmpty();
    const bool hasSize = m_size->currentData().toInt() > 0;
    const bool enabled = canUseTool();

    m_size->setEnabled(hasFont);
    m_text->setEnabled(enabled);
    m_x->setEnabled(enabled);
    m_y->setEnabled(enabled);
    m_colorBtn->setEnabled(enabled);
    m_background->setEnabled(enabled);
    m_spacing->setEnabled(enabled);
    m_previewBtn->setEnabled(enabled);
    m_draw->setEnabled(canDraw());
}

LEKCO_END_NAMESPACE
