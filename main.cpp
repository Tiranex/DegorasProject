
#include "class_mainwindow.h"
#include <DP_Core/include/global_utils.h>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    GlobalUtils::initApp("AlgorithmsTester", "AlgorithmsTester Error",
                      "", "");
    MainWindow window = MainWindow();
    window.resize(900, 600);
    window.show();
    return a.exec();
}

