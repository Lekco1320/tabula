/**
 * @file BitmapWorkspace.cpp
 * @brief EBM bitmap asset workspace implementation for the tabula desktop client.
 *
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2026-05-20
 * @license MIT
 */

#include <QComboBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <oclero/qlementine/widgets/Label.hpp>
#include <epd_asset/bitmap_generator.h>

#include "controls/widgets/EpdFramePreviewer.hpp"
#include "controls/workspaces/BitmapWorkspace.hpp"
#include "controls/workspaces/CanvasWorkspace.hpp"
#include "project/BitmapAssetIO.hpp"

LEKCO_BEGIN_NAMESPACE

BEGIN_NAMESPACE()

constexpr int kDetailsSpacing = 4;
constexpr int kSectionSpacing = 12;
constexpr int kTitleContentSpacing = 10;

QString FormatBytes(qint64 bytes)
{
    return QStringLiteral("%1 bytes").arg(bytes);
}

END_NAMESPACE

BitmapWorkspace::BitmapWorkspace(Project& project, QWidget* parent)
    : QWidget(parent)
    , m_project(project)
{
    m_contentStack = new QStackedWidget(this);

    m_resourcesPage = new QWidget(m_contentStack);
    auto* resourcesLayout = new QVBoxLayout(m_resourcesPage);
    resourcesLayout->setContentsMargins(24, 24, 24, 24);
    resourcesLayout->setSpacing(6);
    resourcesLayout->addStretch(1);

    m_resourcesTitle = new oclero::qlementine::Label(QStringLiteral("Bitmap Assets"),
        oclero::qlementine::TextRole::H5, m_resourcesPage);
    m_resourcesTitle->setAlignment(Qt::AlignCenter);
    resourcesLayout->addWidget(m_resourcesTitle, 0, Qt::AlignHCenter);

    m_resourcesMetrics = new QLabel(m_resourcesPage);
    m_resourcesMetrics->setAlignment(Qt::AlignCenter);
    m_resourcesMetrics->setTextFormat(Qt::RichText);
    resourcesLayout->addWidget(m_resourcesMetrics, 0, Qt::AlignHCenter);
    resourcesLayout->addStretch(1);

    m_editorPage = new QWidget(m_contentStack);
    auto* editorLayout = new QHBoxLayout(m_editorPage);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);

    m_previewer = new EpdFramePreviewer(m_editorPage);
    m_previewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    editorLayout->addWidget(m_previewer, 1);

    auto* divider = new QWidget(m_editorPage);
    divider->setFixedWidth(1);
    divider->setStyleSheet(QStringLiteral("background-color: #b0b0b0;"));
    divider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    editorLayout->addWidget(divider);

    auto* detailsPane = new QWidget(m_editorPage);
    detailsPane->setFixedWidth(kWorkspaceDetailsPaneWidth);
    detailsPane->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    auto* detailsLayout = new QVBoxLayout(detailsPane);
    detailsLayout->setContentsMargins(12, 12, 12, 12);
    detailsLayout->setSpacing(kDetailsSpacing);

    m_titleLabel = new oclero::qlementine::Label(QStringLiteral("Bitmap Summary"),
        oclero::qlementine::TextRole::H5, detailsPane);
    detailsLayout->addWidget(m_titleLabel);

    m_statusLabel = new QLabel(detailsPane);
    m_statusLabel->setTextFormat(Qt::RichText);
    m_statusLabel->setVisible(false);
    detailsLayout->addWidget(m_statusLabel);

    m_metricsLabel = new QLabel(detailsPane);
    m_metricsLabel->setTextFormat(Qt::RichText);
    m_metricsLabel->setWordWrap(true);
    detailsLayout->addWidget(m_metricsLabel);
    detailsLayout->addSpacing(kSectionSpacing);

    auto* generatorTitle = new oclero::qlementine::Label(QStringLiteral("Generator"),
        oclero::qlementine::TextRole::H5, detailsPane);
    detailsLayout->addWidget(generatorTitle);
    detailsLayout->addSpacing(kTitleContentSpacing - kDetailsSpacing);

    auto* algorithmForm = new QFormLayout();
    algorithmForm->setContentsMargins(0, 0, 0, 0);
    algorithmForm->setSpacing(kDetailsSpacing);

    m_algorithm = new QComboBox(detailsPane);
    m_algorithm->addItem(QStringLiteral("Threshold"), static_cast<int>(ProjectBitmapAlgorithm::Threshold));
    m_algorithm->addItem(QStringLiteral("Ordered Dither"), static_cast<int>(ProjectBitmapAlgorithm::Ordered));
    m_algorithm->addItem(QStringLiteral("Random"), static_cast<int>(ProjectBitmapAlgorithm::Random));
    m_algorithm->addItem(QStringLiteral("Blue Noise"), static_cast<int>(ProjectBitmapAlgorithm::BlueNoise));
    m_algorithm->addItem(QStringLiteral("Floyd-Steinberg"), static_cast<int>(ProjectBitmapAlgorithm::FloydSteinberg));
    algorithmForm->addRow(QStringLiteral("Algorithm:"), m_algorithm);
    detailsLayout->addLayout(algorithmForm);

    m_paramStack = new QStackedWidget(detailsPane);
    detailsLayout->addWidget(m_paramStack);

    auto* thresholdPage = new QWidget(m_paramStack);
    auto* thresholdForm = new QFormLayout(thresholdPage);
    thresholdForm->setContentsMargins(0, 0, 0, 0);
    thresholdForm->setSpacing(kDetailsSpacing);
    m_blackThreshold = new QSpinBox(thresholdPage);
    m_redThreshold = new QSpinBox(thresholdPage);
    m_redSaturation = new QSpinBox(thresholdPage);
    for (QSpinBox* spin : { m_blackThreshold, m_redThreshold, m_redSaturation }) {
        spin->setRange(0, 255);
    }
    thresholdForm->addRow(QStringLiteral("Black:"), m_blackThreshold);
    thresholdForm->addRow(QStringLiteral("Red:"), m_redThreshold);
    thresholdForm->addRow(QStringLiteral("Saturation:"), m_redSaturation);
    m_paramStack->addWidget(thresholdPage);

    auto* orderedPage = new QWidget(m_paramStack);
    auto* orderedForm = new QFormLayout(orderedPage);
    orderedForm->setContentsMargins(0, 0, 0, 0);
    orderedForm->setSpacing(kDetailsSpacing);
    m_orderedMatrix = new QComboBox(orderedPage);
    m_orderedMatrix->addItem(QStringLiteral("2 x 2"), EPD_ASSET_BITMAP_ORDERED_MATRIX_2X2);
    m_orderedMatrix->addItem(QStringLiteral("4 x 4"), EPD_ASSET_BITMAP_ORDERED_MATRIX_4X4);
    m_orderedMatrix->addItem(QStringLiteral("8 x 8"), EPD_ASSET_BITMAP_ORDERED_MATRIX_8X8);
    orderedForm->addRow(QStringLiteral("Matrix:"), m_orderedMatrix);
    m_paramStack->addWidget(orderedPage);

    auto* blueNoisePage = new QWidget(m_paramStack);
    auto* blueNoiseForm = new QFormLayout(blueNoisePage);
    blueNoiseForm->setContentsMargins(0, 0, 0, 0);
    blueNoiseForm->setSpacing(kDetailsSpacing);
    m_blueNoiseMatrix = new QComboBox(blueNoisePage);
    m_blueNoiseMatrix->addItem(QStringLiteral("64 x 64"), EPD_ASSET_BITMAP_BLUE_NOISE_MATRIX_64X64);
    m_blueNoiseMatrix->addItem(QStringLiteral("128 x 128"), EPD_ASSET_BITMAP_BLUE_NOISE_MATRIX_128X128);
    m_blueNoiseMatrix->addItem(QStringLiteral("256 x 256"), EPD_ASSET_BITMAP_BLUE_NOISE_MATRIX_256X256);
    blueNoiseForm->addRow(QStringLiteral("Matrix:"), m_blueNoiseMatrix);
    m_paramStack->addWidget(blueNoisePage);

    m_paramStack->addWidget(new QWidget(m_paramStack));
    m_paramStack->addWidget(new QWidget(m_paramStack));

    detailsLayout->addStretch(1);
    editorLayout->addWidget(detailsPane);

    m_contentStack->addWidget(m_resourcesPage);
    m_contentStack->addWidget(m_editorPage);
    m_contentStack->setCurrentWidget(m_resourcesPage);

    auto* outer = new QHBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);
    outer->addWidget(m_contentStack, 1);

    connect(m_algorithm, qOverload<int>(&QComboBox::currentIndexChanged), this, &BitmapWorkspace::updateAlgorithm);
    connect(m_blackThreshold, qOverload<int>(&QSpinBox::valueChanged), this, &BitmapWorkspace::autoApply);
    connect(m_redThreshold, qOverload<int>(&QSpinBox::valueChanged), this, &BitmapWorkspace::autoApply);
    connect(m_redSaturation, qOverload<int>(&QSpinBox::valueChanged), this, &BitmapWorkspace::autoApply);
    connect(m_orderedMatrix, qOverload<int>(&QComboBox::currentIndexChanged), this, &BitmapWorkspace::autoApply);
    connect(m_blueNoiseMatrix, qOverload<int>(&QComboBox::currentIndexChanged), this, &BitmapWorkspace::autoApply);

    setControlsEnabled(false);
}

void BitmapWorkspace::setResource(const ProjectResource& resource)
{
    m_resource = resource;
    m_contentStack->setCurrentWidget(m_editorPage);
    loadResource();
}

void BitmapWorkspace::setBitmapResources(const QVector<ProjectResource>& resources)
{
    clearResource();
    updateBitmapResourcesSummary(resources);
    m_contentStack->setCurrentWidget(m_resourcesPage);
}

void BitmapWorkspace::clearResource()
{
    m_resource = ProjectResource();
    m_info     = ProjectBitmapInfo();
    m_hasInfo  = false;
    m_editable = false;
    m_previewer->clear();
    m_metricsLabel->clear();
    m_statusLabel->clear();
    m_statusLabel->setVisible(false);
    setControlsEnabled(false);
}

QString BitmapWorkspace::resourcePath() const
{
    return m_resource.absolutePath;
}

void BitmapWorkspace::loadResource()
{
    m_hasInfo  = m_project.bitmapInfo(m_resource.fileName, &m_info);
    m_editable = m_hasInfo && m_project.bitmapSourceExists(m_resource.fileName)
        && m_info.width != 0U && m_info.height != 0U;

    loadPreview();
    syncControls();
    updateBitmapSummary();
    setControlsEnabled(m_editable);
}

void BitmapWorkspace::loadPreview()
{
    QImage image;
    if (BitmapAssetIO::loadEbmImage(m_resource.absolutePath, &image)) {
        m_previewer->setImage(image);
    } else {
        m_previewer->clear();
    }
}

void BitmapWorkspace::syncControls()
{
    m_updating = true;

    const QSignalBlocker algorithmBlocker(m_algorithm);
    const QSignalBlocker blackBlocker(m_blackThreshold);
    const QSignalBlocker redBlocker(m_redThreshold);
    const QSignalBlocker saturationBlocker(m_redSaturation);
    const QSignalBlocker orderedBlocker(m_orderedMatrix);
    const QSignalBlocker blueNoiseBlocker(m_blueNoiseMatrix);

    m_algorithm->setCurrentIndex(m_algorithm->findData(static_cast<int>(m_info.algorithm)));
    m_blackThreshold->setValue(m_info.blackThreshold);
    m_redThreshold->setValue(m_info.redThreshold);
    m_redSaturation->setValue(m_info.redSaturation);
    m_orderedMatrix->setCurrentIndex(m_orderedMatrix->findData(static_cast<int>(m_info.orderedMatrix)));
    m_blueNoiseMatrix->setCurrentIndex(m_blueNoiseMatrix->findData(static_cast<int>(m_info.blueNoiseMatrix)));

    updateAlgorithmPage();
    m_updating = false;
}

void BitmapWorkspace::setControlsEnabled(bool enabled)
{
    m_algorithm->setEnabled(enabled);
    m_paramStack->setEnabled(enabled);
}

void BitmapWorkspace::updateBitmapSummary()
{
    if (m_resource.absolutePath.isEmpty()) {
        m_metricsLabel->clear();
        m_statusLabel->clear();
        m_statusLabel->setVisible(false);
        return;
    }

    const QFileInfo fileInfo(m_resource.absolutePath);
    const QString sourceText = m_editable
        ? QFileInfo(m_project.bitmapSourcePath(m_resource.fileName)).fileName()
        : QStringLiteral("<span style=\"color: #d32f2f;\">Missing</span>");
    m_statusLabel->setText(QStringLiteral("<span style=\"color: #d32f2f;\">Missing</span>"));
    m_statusLabel->setVisible(!m_editable);

    epd_gfx_ebm_header_t header = {};
    QString sizeText = QStringLiteral("-");
    QString formatText = QStringLiteral("-");
    if (BitmapAssetIO::readHeader(m_resource.absolutePath, &header)) {
        sizeText   = QStringLiteral("%1 x %2").arg(header.width).arg(header.height);
        formatText = formatName(header.format);
    }

    m_metricsLabel->setText(QStringLiteral(
        "<table cellspacing=\"0\" cellpadding=\"2\" width=\"100%\">"
        "<tr><td>Source</td><td align=\"right\">%1</td></tr>"
        "<tr><td>Bitmap</td><td align=\"right\">%2</td></tr>"
        "<tr><td>Format</td><td align=\"right\">%3</td></tr>"
        "<tr><td>File Size</td><td align=\"right\">%4 bytes</td></tr>"
        "</table>")
        .arg(sourceText)
        .arg(sizeText)
        .arg(formatText)
        .arg(fileInfo.size()));
}

void BitmapWorkspace::updateBitmapResourcesSummary(const QVector<ProjectResource>& resources)
{
    int     bitmapCount = 0;
    qint64  totalBytes  = 0;
    QString rows;

    for (const ProjectResource& resource : resources) {
        epd_gfx_ebm_header_t header = {};
        if (!BitmapAssetIO::readHeader(resource.absolutePath, &header)
            || !BitmapAssetIO::isValidEbmFile(resource.absolutePath)) {
            continue;
        }

        QFileInfo fileInfo(resource.absolutePath);
        ++bitmapCount;
        totalBytes += fileInfo.size();
        rows += QStringLiteral(
            "<tr>"
            "<td style=\"padding-right: 30px;\">%1</td>"
            "<td align=\"center\" style=\"padding-left: 16px; padding-right: 16px;\">%2</td>"
            "<td align=\"center\" style=\"padding-left: 16px; padding-right: 16px;\">%3</td>"
            "<td align=\"right\" style=\"padding-left: 30px;\">%4</td>"
            "</tr>")
            .arg(fileInfo.completeBaseName())
            .arg(QStringLiteral("%1 x %2").arg(header.width).arg(header.height))
            .arg(formatName(header.format))
            .arg(FormatBytes(fileInfo.size()).replace(QStringLiteral("bytes"), QStringLiteral("Bytes")));
    }

    m_resourcesTitle->setText(QStringLiteral("Bitmap Assets"));
    m_resourcesMetrics->setText(QStringLiteral(
        "<div align=\"center\">"
        "<p>%1 Bitmap%2, %3</p>"
        "<p>&nbsp;</p>"
        "<table cellspacing=\"0\" cellpadding=\"1\">"
        "<tr>"
        "<th align=\"left\" style=\"padding-right: 30px;\">Name</th>"
        "<th align=\"center\" style=\"padding-left: 16px; padding-right: 16px;\">Size</th>"
        "<th align=\"center\" style=\"padding-left: 16px; padding-right: 16px;\">Format</th>"
        "<th align=\"right\" style=\"padding-left: 30px;\">Bytes</th>"
        "</tr>"
        "%4"
        "</table>"
        "</div>")
        .arg(bitmapCount)
        .arg(bitmapCount == 1 ? QString() : QStringLiteral("s"))
        .arg(FormatBytes(totalBytes).replace(QStringLiteral("bytes"), QStringLiteral("Bytes")))
        .arg(rows));
}

void BitmapWorkspace::updateAlgorithm()
{
    updateAlgorithmPage();
    autoApply();
}

void BitmapWorkspace::updateAlgorithmPage()
{
    const ProjectBitmapAlgorithm algorithm = static_cast<ProjectBitmapAlgorithm>(
        m_algorithm->currentData().toInt());
    m_paramStack->setCurrentIndex(static_cast<int>(algorithm));
    m_paramStack->setVisible(algorithm == ProjectBitmapAlgorithm::Threshold
        || algorithm == ProjectBitmapAlgorithm::Ordered
        || algorithm == ProjectBitmapAlgorithm::BlueNoise);
}

void BitmapWorkspace::autoApply()
{
    if (m_updating || !m_editable) {
        return;
    }

    const ProjectBitmapInfo oldInfo = m_info;
    ProjectBitmapInfo nextInfo = infoFromControls();
    const QString sourcePath = m_project.bitmapSourcePath(m_resource.fileName);

    epd_gfx_frame_view_t view = {};
    QString error;
    if (!BitmapAssetIO::generateFrame(sourcePath, nextInfo, &view, &error)) {
        QMessageBox::critical(this, QStringLiteral("Bitmap Error"), error);
        syncControls();
        return;
    }

    QImage preview;
    if (!BitmapAssetIO::frameViewToImage(&view, &preview)) {
        error = QStringLiteral("Failed to build bitmap preview.");
    }
    if (error.isEmpty() && !BitmapAssetIO::saveFrameView(m_resource.absolutePath, &view, &error)) {
        error = error.isEmpty() ? QStringLiteral("Failed to save bitmap file.") : error;
    }
    if (!error.isEmpty()) {
        epd_asset_bitmap_destroy_frame_view(&view);
        QMessageBox::critical(this, QStringLiteral("Bitmap Error"), error);
        syncControls();
        return;
    }
    epd_asset_bitmap_destroy_frame_view(&view);

    if (!m_project.setBitmapInfo(m_resource.fileName, nextInfo, &error)) {
        (void)restoreBitmap(oldInfo);
        m_info = oldInfo;
        loadPreview();
        QMessageBox::critical(this, QStringLiteral("Bitmap Error"), error);
        syncControls();
        return;
    }

    m_info = nextInfo;
    m_previewer->setImage(preview);
    updateBitmapSummary();
}

ProjectBitmapInfo BitmapWorkspace::infoFromControls() const
{
    ProjectBitmapInfo info = m_info;
    info.algorithm = static_cast<ProjectBitmapAlgorithm>(m_algorithm->currentData().toInt());
    info.blackThreshold = static_cast<uint8_t>(m_blackThreshold->value());
    info.redThreshold = static_cast<uint8_t>(m_redThreshold->value());
    info.redSaturation = static_cast<uint8_t>(m_redSaturation->value());
    info.orderedMatrix = static_cast<epd_asset_bitmap_ordered_matrix_size_t>(
        m_orderedMatrix->currentData().toInt());
    info.blueNoiseMatrix = static_cast<epd_asset_bitmap_blue_noise_matrix_size_t>(
        m_blueNoiseMatrix->currentData().toInt());
    return info;
}

bool BitmapWorkspace::restoreBitmap(const ProjectBitmapInfo& info)
{
    QString error;
    return BitmapAssetIO::generateAndSave(m_project.bitmapSourcePath(m_resource.fileName),
        m_resource.absolutePath, info, &error);
}

QString BitmapWorkspace::formatName(epd_gfx_format_t format)
{
    return format == EPD_GFX_FORMAT_PLANES
        ? QStringLiteral("Planes")
        : QStringLiteral("Native");
}

LEKCO_END_NAMESPACE
