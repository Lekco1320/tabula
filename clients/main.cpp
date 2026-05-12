/**
 * @file main.cpp
 * @brief Entry for tabula desktop client.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-9
 * @license MIT
 */

#include <QApplication>
#include <QFontDatabase>
#include <QDialog>
#include <oclero/qlementine.hpp>
#include <epd_gfx/canvas.h>

#include "controls/windows/MainWindow.hpp"
#include "controls/windows/SetupDialog.hpp"
#include "controls/Utils.hpp"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    auto* style = new oclero::qlementine::QlementineStyle(&app);
    style->setThemeJsonPath(":/common/themes.json");
    QApplication::setStyle(style);

    lekco::SetupDialog dialog;
    if (dialog.exec() != QDialog::Accepted) {
        return 0;
    }

    epd_gfx_canvas_config_t config {};
    config.width    = static_cast<uint16_t>(dialog.panelWidth());
    config.height   = static_cast<uint16_t>(dialog.panelHeight());
    config.format   = dialog.format();
    config.rotation = dialog.rotation();

    lekco::MainWindow w(config);
    SetWindowCenterScreen(&w);
    w.show();
    return app.exec();
}
