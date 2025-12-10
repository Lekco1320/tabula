/**
 * @file main.cpp
 * @brief Entry for tabula desktop client.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-9
 * @license MIT
 */

#include "MainWindow.hpp"

#include <QApplication>
#include <QFontDatabase>
#include <oclero/qlementine.hpp>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    auto* style = new oclero::qlementine::QlementineStyle(&app);
    style->setThemeJsonPath(":/common/themes.json");
    QApplication::setStyle(style);

    MainWindow w;
    w.show();
    return app.exec();
}
