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
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QSizePolicy>
#include <QStackedWidget>
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
#include "project/FontAssetIO.hpp"

LEKCO_BEGIN_NAMESPACE

BEGIN_NAMESPACE()

constexpr int      kDetailsPaneWidth = 270;
constexpr int      kPreviewSize      = 160;
constexpr uint32_t kProgressMax      = 100000U;

QString ErrorText(epd_err_t err)
{
    return FontAssetIO::errorText(err);
}

QString FormatBytes(qint64 bytes)
{
    return QStringLiteral("%1 bytes").arg(bytes);
}

END_NAMESPACE

FontWorkspace::FontWorkspace(const Project& project, QWidget* parent)
    : QWidget(parent)
    , m_project(project)
{
    m_contentStack = new QStackedWidget(this);

    m_resourcesPage = new QWidget(m_contentStack);
    auto* resourcesLayout = new QVBoxLayout(m_resourcesPage);
    resourcesLayout->setContentsMargins(24, 24, 24, 24);
    resourcesLayout->setSpacing(12);
    resourcesLayout->addStretch(1);

    m_resourcesTitle = new oclero::qlementine::Label(QStringLiteral("Font Assets"),
        oclero::qlementine::TextRole::H5, m_resourcesPage);
    m_resourcesTitle->setAlignment(Qt::AlignCenter);
    resourcesLayout->addWidget(m_resourcesTitle, 0, Qt::AlignHCenter);

    m_resourcesMetrics = new QLabel(m_resourcesPage);
    m_resourcesMetrics->setAlignment(Qt::AlignCenter);
    m_resourcesMetrics->setTextFormat(Qt::RichText);
    resourcesLayout->setSpacing(6);
    resourcesLayout->addWidget(m_resourcesMetrics, 0, Qt::AlignHCenter);
    resourcesLayout->addStretch(1);

    m_editorPage = new QWidget(m_contentStack);
    auto* editorLayout = new QHBoxLayout(m_editorPage);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);

    m_grid = new FontGlyphGridWidget(m_editorPage);
    m_grid->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    editorLayout->addWidget(m_grid, 1);
    connect(m_grid, &FontGlyphGridWidget::sizeSelected, this, &FontWorkspace::selectSize);
    connect(m_grid, &FontGlyphGridWidget::glyphSelected, this, &FontWorkspace::selectGlyph);
    connect(m_grid, &FontGlyphGridWidget::selectionCleared, this, &FontWorkspace::clearGlyphSelection);

    auto* detailsPane = new QWidget(m_editorPage);
    detailsPane->setFixedWidth(kDetailsPaneWidth);
    detailsPane->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    auto* detailsLayout = new QVBoxLayout(detailsPane);
    detailsLayout->setContentsMargins(12, 12, 12, 12);
    detailsLayout->setSpacing(10);

    m_titleLabel = new oclero::qlementine::Label(QStringLiteral("Glyph Preview"),
        oclero::qlementine::TextRole::H5, detailsPane);
    detailsLayout->addWidget(m_titleLabel);

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
    connect(m_deleteButton, &QPushButton::clicked, this, &FontWorkspace::deleteSelected);

    auto* divider = new QWidget(m_editorPage);
    divider->setFixedWidth(1);
    divider->setStyleSheet(QStringLiteral("background-color: #b0b0b0;"));
    divider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    editorLayout->addWidget(divider);
    editorLayout->addWidget(detailsPane);
    m_contentStack->addWidget(m_resourcesPage);
    m_contentStack->addWidget(m_editorPage);
    m_contentStack->setCurrentWidget(m_resourcesPage);

    auto* outer = new QHBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);
    outer->addWidget(m_contentStack, 1);
    setDetailsEnabled(false);
}

FontWorkspace::~FontWorkspace()
{
    clearResource();
}

void FontWorkspace::setResource(const ProjectResource& resource)
{
    clearLoadedFont();
    m_resource = resource;
    m_contentStack->setCurrentWidget(m_editorPage);
    loadResource();
}

void FontWorkspace::setFontResources(const QVector<ProjectResource>& resources)
{
    clearLoadedFont();
    updateFontResourcesSummary(resources);
    m_contentStack->setCurrentWidget(m_resourcesPage);
}

void FontWorkspace::clearResource()
{
    clearLoadedFont();
    m_contentStack->setCurrentWidget(m_resourcesPage);
    m_resourcesMetrics->clear();
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

    m_selectionKind     = SelectionKind::None;
    m_selectedSize      = 0U;
    m_selectedCodepoint = 0U;
    updateFontSummary();
    setDetailsEnabled(false);

    epd_err_t ret = FontAssetIO::loadEditableAsset(m_resource.absolutePath, &m_asset);
    if (ret != EPD_OK) {
        QMessageBox::critical(this, QStringLiteral("Font Error"),
            QStringLiteral("Failed to load font file: %1").arg(ErrorText(ret)));
        return;
    }

    m_grid->setFontAsset(m_asset);
    refreshSections();
    setDetailsEnabled(true);
    updateFontSummary();
}

void FontWorkspace::clearLoadedFont()
{
    m_grid->clear();
    if (m_asset) {
        epd_asset_font_asset_destroy(m_asset);
        m_asset = nullptr;
    }

    m_resource          = ProjectResource();
    m_selectionKind     = SelectionKind::None;
    m_selectedSize      = 0U;
    m_selectedCodepoint = 0U;
    updateFontSummary();
    setDetailsEnabled(false);
}

bool FontWorkspace::saveResource()
{
    if (!m_asset || m_resource.absolutePath.isEmpty()) {
        return false;
    }

    QString error;
    if (!FontAssetIO::saveEditableAsset(m_resource.absolutePath, m_asset, &error)) {
        QMessageBox::critical(this, QStringLiteral("Font Error"), error);
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

        epd_asset_font_asset_size_info_t sizeInfo;
        if (epd_asset_font_asset_get_size_info(m_asset, size, &sizeInfo) != EPD_OK) {
            continue;
        }

        FontGlyphGridWidget::Section section;
        section.size       = size;
        section.ascent     = sizeInfo.ascent;
        section.descent    = sizeInfo.descent;
        section.lineHeight = sizeInfo.line_height;
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

void FontWorkspace::selectSize(uint16_t size)
{
    m_selectionKind     = SelectionKind::Size;
    m_selectedSize      = size;
    m_selectedCodepoint = 0U;
    updateSizeSummary(size);
}

void FontWorkspace::selectGlyph(uint16_t size, uint32_t codepoint)
{
    m_selectionKind     = SelectionKind::Glyph;
    m_selectedSize      = size;
    m_selectedCodepoint = codepoint;
    updateGlyphDetails(size, codepoint);
}

void FontWorkspace::clearGlyphSelection()
{
    m_selectionKind     = SelectionKind::None;
    m_selectedSize      = 0U;
    m_selectedCodepoint = 0U;
    updateFontSummary();
    m_deleteButton->setEnabled(false);
}

void FontWorkspace::addGlyph()
{
    if (!m_asset || !hasEditableSource()) {
        return;
    }

    AddGlyphDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QFile fontFile(m_project.fontSourcePath(m_resource.fileName));
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

    epd_asset_font_asset_size_info_t sizeInfo;
    ret = epd_asset_font_asset_get_size_info(m_asset, size, &sizeInfo);
    if (ret == EPD_ERR_NOT_FOUND) {
        ret = epd_asset_font_face_get_size_info(face, size, &sizeInfo);
        if (ret == EPD_OK) {
            ret = epd_asset_font_asset_set_size_info(m_asset, &sizeInfo);
        }
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
        renderConfig.mode      = dialog.renderMode();
        renderConfig.threshold = dialog.threshold();
        renderConfig.bias      = dialog.bias();

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

void FontWorkspace::deleteSelected()
{
    if (m_selectionKind == SelectionKind::Size) {
        deleteSize();
        return;
    }
    if (m_selectionKind == SelectionKind::Glyph) {
        deleteGlyph();
    }
}

void FontWorkspace::deleteSize()
{
    if (!m_asset || m_selectedSize == 0U) {
        return;
    }

    const uint16_t size       = m_selectedSize;
    uint32_t       glyphCount = 0U;
    (void)epd_asset_font_asset_get_codepoints(m_asset, size, nullptr, &glyphCount);

    const int result = QMessageBox::question(this, QStringLiteral("Delete Size"),
        QStringLiteral("Delete size %1 and all %2 glyphs?").arg(size).arg(glyphCount));
    if (result != QMessageBox::Yes) {
        return;
    }

    const epd_err_t ret = epd_asset_font_asset_remove_size(m_asset, size);
    if (ret != EPD_OK) {
        QMessageBox::critical(this, QStringLiteral("Font Error"),
            QStringLiteral("Failed to delete size: %1").arg(ErrorText(ret)));
        return;
    }

    if (!saveResource()) {
        return;
    }

    uint16_t adjacentSize = 0U;
    uint32_t sizeCount    = 0U;
    if (epd_asset_font_asset_get_sizes(m_asset, nullptr, &sizeCount) == EPD_OK && sizeCount > 0U) {
        QVector<uint16_t> sizes(static_cast<int>(sizeCount));
        if (epd_asset_font_asset_get_sizes(m_asset, sizes.data(), &sizeCount) == EPD_OK) {
            sizes.resize(static_cast<int>(sizeCount));
            adjacentSize = sizes.last();
            for (uint16_t value : sizes) {
                if (value > size) {
                    adjacentSize = value;
                    break;
                }
            }
        }
    }

    refreshSections();
    if (adjacentSize != 0U) {
        selectSize(adjacentSize);
        m_grid->selectSize(adjacentSize);
    } else {
        m_grid->clearSelection();
        clearGlyphSelection();
    }
}

void FontWorkspace::deleteGlyph()
{
    if (!m_asset || m_selectedSize == 0U || m_selectionKind != SelectionKind::Glyph) {
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
        m_grid->clearSelection();
        clearGlyphSelection();
    }
}

bool FontWorkspace::hasEditableSource() const
{
    return m_asset && m_project.fontSourceExists(m_resource.fileName);
}

void FontWorkspace::setDetailsEnabled(bool enabled)
{
    m_addButton->setEnabled(enabled && hasEditableSource());
    m_deleteButton->setEnabled(enabled && m_selectionKind != SelectionKind::None);
}

void FontWorkspace::updateFontSummary()
{
    m_previewWidget->clearGlyph();
    m_titleLabel->setText(QStringLiteral("Font Summary"));
    m_titleLabel->setVisible(true);
    m_previewWidget->setVisible(false);
    m_codepointLabel->setVisible(false);
    m_deleteButton->setVisible(false);
    m_deleteButton->setText(QStringLiteral("Delete Glyph"));
    m_deleteButton->setEnabled(false);

    if (!m_asset) {
        m_metricsLabel->clear();
        return;
    }

    uint32_t sizeCount = 0U;
    if (epd_asset_font_asset_get_sizes(m_asset, nullptr, &sizeCount) != EPD_OK) {
        m_metricsLabel->clear();
        return;
    }

    QVector<uint16_t> sizes(static_cast<int>(sizeCount));
    if (sizeCount > 0U && epd_asset_font_asset_get_sizes(m_asset, sizes.data(), &sizeCount) != EPD_OK) {
        m_metricsLabel->clear();
        return;
    }
    sizes.resize(static_cast<int>(sizeCount));

    const QFileInfo fileInfo(m_resource.absolutePath);
    uint32_t glyphCount = 0U;
    QString  sizeRows;
    for (uint16_t size : sizes) {
        uint32_t codepointCount = 0U;
        if (epd_asset_font_asset_get_codepoints(m_asset, size, nullptr, &codepointCount) != EPD_OK) {
            continue;
        }

        glyphCount += codepointCount;
        sizeRows   += QStringLiteral("<tr><td style=\"padding-left: 12px;\">- Size %1</td><td align=\"right\">%2 glyphs</td></tr>")
            .arg(size)
            .arg(codepointCount);
    }

    const QString sourcePath = m_project.fontSourcePath(m_resource.fileName);
    const QString sourceText = hasEditableSource()
        ? QFileInfo(sourcePath).fileName()
        : QStringLiteral("<span style=\"color: #d32f2f;\">Missing</span>");

    m_metricsLabel->setText(QStringLiteral(
        "<table cellspacing=\"0\" cellpadding=\"2\" width=\"100%\">"
        "<tr><td>Source</td><td align=\"right\">%1</td></tr>"
        "<tr><td>File Size</td><td align=\"right\">%2 bytes</td></tr>"
        "<tr><td>Sizes</td><td align=\"right\">%3</td></tr>"
        "<tr><td>Total Glyphs</td><td align=\"right\">%4</td></tr>"
        "%5"
        "</table>")
        .arg(sourceText)
        .arg(fileInfo.size())
        .arg(sizeCount)
        .arg(glyphCount)
        .arg(sizeRows));
}

void FontWorkspace::updateFontResourcesSummary(const QVector<ProjectResource>& resources)
{
    int     fontCount  = 0;
    qint64  totalBytes = 0;
    QString rows;

    for (const ProjectResource& resource : resources) {
        epd_gfx_egf_header_t header = {};
        if (!FontAssetIO::readHeader(resource.absolutePath, &header)
            || !FontAssetIO::isValidEgfFile(resource.absolutePath)) {
            continue;
        }

        QFileInfo fileInfo(resource.absolutePath);
        ++fontCount;
        totalBytes += fileInfo.size();
        rows += QStringLiteral(
            "<tr>"
            "<td style=\"padding-right: 30px;\">%1</td>"
            "<td align=\"center\" style=\"padding-left: 16px; padding-right: 16px;\">%2</td>"
            "<td align=\"center\" style=\"padding-left: 16px; padding-right: 16px;\">%3</td>"
            "<td align=\"right\" style=\"padding-left: 30px;\">%4</td>"
        "</tr>")
            .arg(fileInfo.completeBaseName())
            .arg(header.size_count)
            .arg(header.glyph_count)
            .arg(FormatBytes(fileInfo.size()).replace(QStringLiteral("bytes"), QStringLiteral("Bytes")));
    }

    m_resourcesTitle->setText(QStringLiteral("Font Assets"));
    m_resourcesMetrics->setText(QStringLiteral(
        "<div align=\"center\">"
        "<p>%1 Font%2, %3</p>"
        "<p>&nbsp;</p>"
        "<table cellspacing=\"0\" cellpadding=\"1\">"
        "<tr>"
        "<th align=\"left\" style=\"padding-right: 30px;\">Name</th>"
        "<th align=\"center\" style=\"padding-left: 16px; padding-right: 16px;\">Sizes</th>"
        "<th align=\"center\" style=\"padding-left: 16px; padding-right: 16px;\">Glyphs</th>"
        "<th align=\"right\" style=\"padding-left: 30px;\">Size</th>"
        "</tr>"
        "%4"
        "</table>"
        "</div>")
        .arg(fontCount)
        .arg(fontCount == 1 ? QString() : QStringLiteral("s"))
        .arg(FormatBytes(totalBytes).replace(QStringLiteral("bytes"), QStringLiteral("Bytes")))
        .arg(rows));
}

void FontWorkspace::updateSizeSummary(uint16_t size)
{
    if (!m_asset) {
        clearGlyphSelection();
        return;
    }

    epd_asset_font_asset_size_info_t sizeInfo;
    epd_err_t ret = epd_asset_font_asset_get_size_info(m_asset, size, &sizeInfo);
    if (ret != EPD_OK) {
        clearGlyphSelection();
        return;
    }

    uint32_t glyphCount = 0U;
    ret = epd_asset_font_asset_get_codepoints(m_asset, size, nullptr, &glyphCount);
    if (ret != EPD_OK) {
        clearGlyphSelection();
        return;
    }

    m_previewWidget->clearGlyph();
    m_titleLabel->setText(QStringLiteral("Size %1").arg(size));
    m_titleLabel->setVisible(true);
    m_previewWidget->setVisible(false);
    m_codepointLabel->setVisible(false);
    m_deleteButton->setText(QStringLiteral("Delete Size"));
    m_deleteButton->setVisible(true);
    m_deleteButton->setEnabled(true);

    m_metricsLabel->setText(QStringLiteral(
        "<table cellspacing=\"0\" cellpadding=\"2\" width=\"100%\">"
        "<tr><td>Glyphs</td><td align=\"right\">%1</td></tr>"
        "<tr><td>Ascender</td><td align=\"right\">%2 px</td></tr>"
        "<tr><td>Descender</td><td align=\"right\">%3 px</td></tr>"
        "<tr><td>Line Height</td><td align=\"right\">%4 px</td></tr>"
        "</table>")
        .arg(glyphCount)
        .arg(sizeInfo.ascent)
        .arg(sizeInfo.descent)
        .arg(sizeInfo.line_height));
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
    epd_err_t       ret   = epd_asset_font_asset_copy_glyph(m_asset, key, &glyph);
    if (ret != EPD_OK || !glyph) {
        clearGlyphSelection();
        return;
    }

    epd_asset_font_asset_size_info_t sizeInfo;
    ret = epd_asset_font_asset_get_size_info(m_asset, size, &sizeInfo);
    if (ret != EPD_OK) {
        epd_gfx_glyph_destroy(glyph);
        clearGlyphSelection();
        return;
    }

    const QImage glyphImage = toMonoImage(glyph);
    m_titleLabel->setText(QStringLiteral("Glyph Preview"));
    m_titleLabel->setVisible(true);
    m_previewWidget->setVisible(true);
    m_codepointLabel->setVisible(true);
    m_deleteButton->setText(QStringLiteral("Delete Glyph"));
    m_deleteButton->setVisible(true);
    m_previewWidget->setGlyph(glyphImage, epd_gfx_glyph_get_xoffset(glyph),
        epd_gfx_glyph_get_yoffset(glyph), epd_gfx_glyph_get_advance(glyph),
        sizeInfo.ascent, sizeInfo.line_height);
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
        .arg(sizeInfo.ascent)
        .arg(sizeInfo.descent)
        .arg(sizeInfo.line_height));
    m_deleteButton->setEnabled(true);

    epd_gfx_glyph_destroy(glyph);
}

QString FontWorkspace::formatCodepoint(uint32_t codepoint)
{
    return QStringLiteral("U+%1").arg(codepoint, codepoint <= 0xFFFFU ? 4 : 6, 16, QLatin1Char('0')).toUpper();
}

LEKCO_END_NAMESPACE
