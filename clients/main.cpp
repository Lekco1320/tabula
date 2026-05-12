/**
 * @file main.cpp
 * @brief Entry for tabula desktop client.
 * 
 * @author Lukaß Zhang <lekco_1320@qq.com>
 * @date 2025-12-9
 * @license MIT
 */

#include <QApplication>
#include <QDialog>
#include <oclero/qlementine.hpp>

#include "controls/windows/MainWindow.hpp"
#include "controls/windows/ProjectDialog.hpp"
#include "controls/Utils.hpp"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    auto* style = new oclero::qlementine::QlementineStyle(&app);
    QApplication::setStyle(style);

    lekco::ProjectDialog dialog;
    if (dialog.exec() != QDialog::Accepted) {
        return 0;
    }

    lekco::MainWindow w(dialog.project());
    SetWindowCenterScreen(&w);
    w.show();
    return app.exec();
}
