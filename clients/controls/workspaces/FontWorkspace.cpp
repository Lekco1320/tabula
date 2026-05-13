/**
 * @file FontWorkspace.cpp
 * @brief EGF font asset workspace implementation for the tabula desktop client.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-13
 * @license MIT
 */

#include <QByteArray>
#include <QFile>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QSaveFile>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>
#include <oclero/qlementine/widgets/Label.hpp>
#include <epd_asset/font_face.h>
#include <epd_core/common.h>
#include <epd_gfx/glyph.h>

#include "controls/widgets/FontGlyphGridWidget.hpp"
#include "controls/widgets/FontGlyphImage.hpp"
#include "controls/widgets/FontGlyphPreviewWidget.hpp"
#include "controls/windows/AddGlyphDialog.hpp"
#include "controls/workspaces/FontWorkspace.hpp"
#include "project/EpdStreamAdapter.hpp"

LEKCO_BEGIN_NAMESPACE

BEGIN_NAMESPACE()

constexpr int      kDetailsPaneWidth = 270;
constexpr int      kPreviewSize      = 160;
constexpr uint32_t kProgressMax      = 100000U;

QString ErrorText(epd_err_t err)
{
    return QString::fromLatin1(epd_err_to_str(err));
}

END_NAMESPACE

FontWorkspace::FontWorkspace(QWidget* parent)
    : ResourceWorkspace(parent)
{
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_grid = new FontGlyphGridWidget(this);
    m_grid->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    root->addWidget(m_grid, 1);
    connect(m_grid, &FontGlyphGridWidget::glyphSelected, this, &FontWorkspace::selectGlyph);
    connect(m_grid, &FontGlyphGridWidget::selectionCleared, this, &FontWorkspace::clearGlyphSelection);

    auto* detailsPane = new QWidget(this);
    detailsPane->setFixedWidth(kDetailsPaneWidth);
    detailsPane->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    auto* detailsLayout = new QVBoxLayout(detailsPane);
    detailsLayout->setContentsMargins(12, 12, 12, 12);
    detailsLayout->setSpacing(10);

    auto* titleLabel = new oclero::qlementine::Label(QStringLiteral("Glyph Preview"),
        oclero::qlementine::TextRole::H5, detailsPane);
    detailsLayout->addWidget(titleLabel);

    m_previewWidget = new FontGlyphPreviewWidget(detailsPane);
    m_previewWidget->setFixedSize(kPreviewSize, kPreviewSize);
    detailsLayout->addWidget(m_previewWidget, 0, Qt::AlignHCenter);

    m_codepointLabel = new QLabel(QStringLiteral("Select a glyph"), detailsPane);
    m_codepointLabel->setAlignment(Qt::AlignCenter);
    QFont codepointFont = m_codepointLabel->font();
    codepointFont.setBold(true);
    m_codepointLabel->setFont(codepointFont);
    detailsLayout->addWidget(m_codepointLabel);

    m_metricsLabel = new QLabel(detailsPane);
    m_metricsLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_metricsLabel->setTextFormat(Qt::RichText);
    m_metricsLabel->setWordWrap(true);
    detailsLayout->addWidget(m_metricsLabel);
    detailsLayout->addStretch(1);

    m_addButton    = new QPushButton(QStringLiteral("Add Glyph"), detailsPane);
    m_deleteButton = new QPushButton(QStringLiteral("Delete Glyph"), detailsPane);
    m_addButton->setDefault(true);
    detailsLayout->addWidget(m_addButton);
    detailsLayout->addWidget(m_deleteButton);
    connect(m_addButton, &QPushButton::clicked, this, &FontWorkspace::addGlyph);
    connect(m_deleteButton, &QPushButton::clicked, this, &FontWorkspace::deleteGlyph);

    auto* divider = new QWidget(this);
    divider->setFixedWidth(1);
    divider->setStyleSheet(QStringLiteral("background-color: #b0b0b0;"));
    divider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    root->addWidget(divider);
    root->addWidget(detailsPane);
    setDetailsEnabled(false);
}

FontWorkspace::~FontWorkspace()
{
    clearResource();
}

void FontWorkspace::setResource(const ProjectResource& resource)
{
    clearResource();
    m_resource = resource;
    loadResource();
}

void FontWorkspace::clearResource()
{
    m_grid->clear();
    if (m_asset) {
        epd_asset_font_asset_destroy(m_asset);
        m_asset = nullptr;
    }

    m_resource          = ProjectResource();
    m_selectedSize      = 0U;
    m_selectedCodepoint = 0U;
    m_previewWidget->clearGlyph();
    m_codepointLabel->setText(QStringLiteral("Select a glyph"));
    m_metricsLabel->clear();
    setDetailsEnabled(false);
}

QString FontWorkspace::resourcePath() const
{
    return m_resource.absolutePath;
}

void FontWorkspace::loadResource()
{
    m_grid->clear();
    if (m_asset) {
        epd_asset_font_asset_destroy(m_asset);
        m_asset = nullptr;
    }
    m_selectedSize      = 0U;
    m_selectedCodepoint = 0U;
    clearGlyphSelection();
    setDetailsEnabled(false);

    QFile file(m_resource.absolutePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, QStringLiteral("Font Error"), QStringLiteral("Failed to open font file."));
        return;
    }

    EpdStreamAdapter stream(&file);
    epd_err_t ret = epd_asset_font_asset_load_egf(stream.stream(), &m_asset);
    if (ret != EPD_OK) {
        QMessageBox::critical(this, QStringLiteral("Font Error"),
            QStringLiteral("Failed to load font file: %1").arg(ErrorText(ret)));
        return;
    }

    m_grid->setFontAsset(m_asset);
    refreshSections();
    setDetailsEnabled(true);
}

bool FontWorkspace::saveResource()
{
    if (!m_asset || m_resource.absolutePath.isEmpty()) {
        return false;
    }

    QSaveFile file(m_resource.absolutePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, QStringLiteral("Font Error"), QStringLiteral("Failed to open font file for writing."));
        loadResource();
        return false;
    }

    EpdStreamAdapter stream(&file);
    const epd_err_t ret = epd_asset_font_asset_write_egf(m_asset, stream.stream());
    if (ret != EPD_OK || !file.commit()) {
        QMessageBox::critical(this, QStringLiteral("Font Error"),
            QStringLiteral("Failed to save font file: %1").arg(ret == EPD_OK ? QStringLiteral("commit failed") : ErrorText(ret)));
        loadResource();
        return false;
    }

    return true;
}

void FontWorkspace::refreshSections()
{
    QVector<FontGlyphGridWidget::Section> sections;
    if (!m_asset) {
        m_grid->setSections(sections);
        return;
    }

    uint32_t sizeCount = 0U;
    if (epd_asset_font_asset_get_sizes(m_asset, nullptr, &sizeCount) != EPD_OK) {
        m_grid->setSections(sections);
        return;
    }

    QVector<uint16_t> sizes(static_cast<int>(sizeCount));
    if (sizeCount > 0U && epd_asset_font_asset_get_sizes(m_asset, sizes.data(), &sizeCount) != EPD_OK) {
        m_grid->setSections(sections);
        return;
    }
    sizes.resize(static_cast<int>(sizeCount));

    for (uint16_t size : sizes) {
        uint32_t codepointCount = 0U;
        if (epd_asset_font_asset_get_codepoints(m_asset, size, nullptr, &codepointCount) != EPD_OK) {
            continue;
        }

        epd_asset_font_asset_size_config_t sizeConfig;
        if (epd_asset_font_asset_get_size_config(m_asset, size, &sizeConfig) != EPD_OK) {
            continue;
        }

        FontGlyphGridWidget::Section section;
        section.size       = size;
        section.ascent     = sizeConfig.ascent;
        section.descent    = sizeConfig.descent;
        section.lineHeight = sizeConfig.line_height;
        section.codepoints.resize(static_cast<int>(codepointCount));
        if (codepointCount > 0U
            && epd_asset_font_asset_get_codepoints(m_asset, size, section.codepoints.data(), &codepointCount) != EPD_OK) {
            continue;
        }
        section.codepoints.resize(static_cast<int>(codepointCount));
        sections.append(section);
    }

    m_grid->setFontAsset(m_asset);
    m_grid->setSections(sections);
}

void FontWorkspace::selectGlyph(uint16_t size, uint32_t codepoint)
{
    m_selectedSize      = size;
    m_selectedCodepoint = codepoint;
    updateGlyphDetails(size, codepoint);
}

void FontWorkspace::clearGlyphSelection()
{
    m_selectedSize      = 0U;
    m_selectedCodepoint = 0U;
    m_previewWidget->clearGlyph();
    m_codepointLabel->setText(QStringLiteral("Select a glyph"));
    m_metricsLabel->clear();
    m_deleteButton->setEnabled(false);
}

void FontWorkspace::addGlyph()
{
    if (!m_asset) {
        return;
    }

    AddGlyphDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QFile fontFile(dialog.fontPath());
    if (!fontFile.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, QStringLiteral("Glyph Error"), QStringLiteral("Failed to read source font file."));
        return;
    }

    const QByteArray fontData = fontFile.readAll();
    if (fontData.isEmpty()) {
        QMessageBox::critical(this, QStringLiteral("Glyph Error"), QStringLiteral("Source font file is empty."));
        return;
    }

    const uint16_t size = dialog.size();

    epd_asset_font_face_config_t faceConfig;
    faceConfig.data      = reinterpret_cast<const uint8_t*>(fontData.constData());
    faceConfig.data_size = static_cast<size_t>(fontData.size());
    faceConfig.px_size   = size;

    epd_asset_font_face_t face = nullptr;
    epd_err_t             ret  = epd_asset_font_face_create(&faceConfig, &face);
    if (ret != EPD_OK) {
        QMessageBox::critical(this, QStringLiteral("Glyph Error"),
            QStringLiteral("Failed to open source font: %1").arg(ErrorText(ret)));
        return;
    }

    epd_asset_font_asset_size_config_t sizeConfig;
    ret = epd_asset_font_face_get_size_config(face, size, &sizeConfig);
    if (ret == EPD_OK) {
        ret = epd_asset_font_asset_set_size(m_asset, &sizeConfig);
    }
    if (ret != EPD_OK) {
        epd_asset_font_face_destroy(face);
        QMessageBox::critical(this, QStringLiteral("Glyph Error"),
            QStringLiteral("Failed to set font size: %1").arg(ErrorText(ret)));
        loadResource();
        return;
    }

    int            added               = 0;
    int            skipped             = 0;
    uint32_t       firstAddedCodepoint = 0U;
    const uint32_t start               = dialog.startCodepoint();
    const uint32_t end                 = dialog.endCodepoint();
    const uint32_t total               = end - start + 1U;
    QProgressDialog progress(QStringLiteral("Adding glyphs..."), QStringLiteral("Cancel"),
        0, static_cast<int>(qMin(total, kProgressMax)), this);
    progress.setWindowModality(Qt::WindowModal);

    for (uint32_t codepoint = start; codepoint <= end; ++codepoint) {
        const uint32_t processed     = codepoint - start;
        const int      progressValue = total > kProgressMax
            ? static_cast<int>((static_cast<quint64>(processed) * kProgressMax) / total)
            : static_cast<int>(processed);
        progress.setValue(progressValue);
        if (progress.wasCanceled()) {
            break;
        }

        if (!epd_asset_font_face_contains_glyph(face, codepoint)) {
            ++skipped;
            continue;
        }

        epd_asset_font_face_render_config_t renderConfig;
        renderConfig.codepoint = codepoint;
        renderConfig.threshold = 128U;
        renderConfig.bias      = 0;

        epd_gfx_glyph_t glyph = nullptr;
        ret = epd_asset_font_face_render_glyph(face, &renderConfig, &glyph);
        if (ret == EPD_ERR_NOT_FOUND) {
            ++skipped;
            continue;
        }
        if (ret != EPD_OK) {
            epd_asset_font_face_destroy(face);
            QMessageBox::critical(this, QStringLiteral("Glyph Error"),
                QStringLiteral("Failed to render glyph %1: %2").arg(formatCodepoint(codepoint), ErrorText(ret)));
            loadResource();
            return;
        }

        epd_asset_font_asset_glyph_key_t key;
        key.codepoint = codepoint;
        key.size      = size;
        ret = epd_asset_font_asset_add_glyph(m_asset, key, glyph);
        epd_gfx_glyph_destroy(glyph);
        if (ret != EPD_OK) {
            epd_asset_font_face_destroy(face);
            QMessageBox::critical(this, QStringLiteral("Glyph Error"),
                QStringLiteral("Failed to add glyph %1: %2").arg(formatCodepoint(codepoint), ErrorText(ret)));
            loadResource();
            return;
        }

        ++added;
        if (added == 1) {
            firstAddedCodepoint = codepoint;
        }
    }
    progress.setValue(progress.maximum());
    epd_asset_font_face_destroy(face);

    if (added == 0) {
        loadResource();
        QMessageBox::information(this, QStringLiteral("Glyphs Added"),
            QStringLiteral("Added 0 glyphs, skipped %1 unsupported codepoints.").arg(skipped));
        return;
    }

    if (!saveResource()) {
        return;
    }

    refreshSections();
    selectGlyph(size, firstAddedCodepoint);
    m_grid->selectGlyph(size, firstAddedCodepoint);

    QMessageBox::information(this, QStringLiteral("Glyphs Added"),
        QStringLiteral("Added %1 glyphs, skipped %2 unsupported codepoints.").arg(added).arg(skipped));
}

void FontWorkspace::deleteGlyph()
{
    if (!m_asset || m_selectedSize == 0U) {
        return;
    }

    const uint16_t size      = m_selectedSize;
    const uint32_t codepoint = m_selectedCodepoint;
    const int result = QMessageBox::question(this, QStringLiteral("Delete Glyph"),
        QStringLiteral("Delete %1 at size %2?").arg(formatCodepoint(codepoint)).arg(size));
    if (result != QMessageBox::Yes) {
        return;
    }

    epd_asset_font_asset_glyph_key_t key;
    key.codepoint = codepoint;
    key.size      = size;
    const epd_err_t ret = epd_asset_font_asset_remove_glyph(m_asset, key);
    if (ret != EPD_OK) {
        QMessageBox::critical(this, QStringLiteral("Glyph Error"),
            QStringLiteral("Failed to delete glyph: %1").arg(ErrorText(ret)));
        return;
    }

    if (!saveResource()) {
        return;
    }

    bool     hasAdjacent       = false;
    uint32_t adjacentCodepoint = 0U;
    uint32_t codepointCount    = 0U;
    if (epd_asset_font_asset_get_codepoints(m_asset, size, nullptr, &codepointCount) == EPD_OK
        && codepointCount > 0U) {
        QVector<uint32_t> codepoints(static_cast<int>(codepointCount));
        if (epd_asset_font_asset_get_codepoints(m_asset, size, codepoints.data(), &codepointCount) == EPD_OK) {
            codepoints.resize(static_cast<int>(codepointCount));
            adjacentCodepoint = codepoints.last();
            for (uint32_t value : codepoints) {
                if (value > codepoint) {
                    adjacentCodepoint = value;
                    break;
                }
            }
            hasAdjacent = true;
        }
    }

    refreshSections();
    if (hasAdjacent) {
        selectGlyph(size, adjacentCodepoint);
        m_grid->selectGlyph(size, adjacentCodepoint);
    } else {
        clearGlyphSelection();
    }
}

void FontWorkspace::setDetailsEnabled(bool enabled)
{
    m_addButton->setEnabled(enabled);
    m_deleteButton->setEnabled(enabled && m_selectedSize != 0U);
}

void FontWorkspace::updateGlyphDetails(uint16_t size, uint32_t codepoint)
{
    if (!m_asset) {
        clearGlyphSelection();
        return;
    }

    epd_asset_font_asset_glyph_key_t key;
    key.codepoint = codepoint;
    key.size      = size;

    epd_gfx_glyph_t glyph = nullptr;
    epd_err_t       ret   = epd_asset_font_asset_get_glyph(m_asset, key, &glyph);
    if (ret != EPD_OK || !glyph) {
        clearGlyphSelection();
        return;
    }

    epd_asset_font_asset_size_config_t sizeConfig;
    ret = epd_asset_font_asset_get_size_config(m_asset, size, &sizeConfig);
    if (ret != EPD_OK) {
        epd_gfx_glyph_destroy(glyph);
        clearGlyphSelection();
        return;
    }

    const QImage glyphImage = toMonoImage(glyph);
    m_previewWidget->setGlyph(glyphImage, epd_gfx_glyph_get_xoffset(glyph),
        epd_gfx_glyph_get_yoffset(glyph), epd_gfx_glyph_get_advance(glyph),
        sizeConfig.ascent, sizeConfig.line_height);
    m_codepointLabel->setText(formatCodepoint(codepoint));
    m_metricsLabel->setText(QStringLiteral(
        "<table cellspacing=\"0\" cellpadding=\"2\" width=\"100%\">"
        "<tr><td>Font Size</td><td align=\"right\">%1 px</td></tr>"
        "<tr><td>Bitmap Width</td><td align=\"right\">%2 px</td></tr>"
        "<tr><td>Bitmap Height</td><td align=\"right\">%3 px</td></tr>"
        "<tr><td>Horizontal Offset</td><td align=\"right\">%4 px</td></tr>"
        "<tr><td>Vertical Offset</td><td align=\"right\">%5 px</td></tr>"
        "<tr><td>Advance Width</td><td align=\"right\">%6 px</td></tr>"
        "<tr><td>Ascender</td><td align=\"right\">%7 px</td></tr>"
        "<tr><td>Descender</td><td align=\"right\">%8 px</td></tr>"
        "<tr><td>Line Height</td><td align=\"right\">%9 px</td></tr>"
        "</table>")
        .arg(size)
        .arg(epd_gfx_glyph_get_width(glyph))
        .arg(epd_gfx_glyph_get_height(glyph))
        .arg(epd_gfx_glyph_get_xoffset(glyph))
        .arg(epd_gfx_glyph_get_yoffset(glyph))
        .arg(epd_gfx_glyph_get_advance(glyph))
        .arg(sizeConfig.ascent)
        .arg(sizeConfig.descent)
        .arg(sizeConfig.line_height));
    m_deleteButton->setEnabled(true);

    epd_gfx_glyph_destroy(glyph);
}

QString FontWorkspace::formatCodepoint(uint32_t codepoint)
{
    return QStringLiteral("U+%1").arg(codepoint, codepoint <= 0xFFFFU ? 4 : 6, 16, QLatin1Char('0')).toUpper();
}

LEKCO_END_NAMESPACE
