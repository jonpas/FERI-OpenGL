#include "mainwindow.h"

#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char *argv[]) {
    // Parameters for loading OpenGL context, version selection
    QSurfaceFormat glFormat;
    glFormat.setVersion(3, 3);
    glFormat.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(glFormat);

    // Desktop OpenGL has to be selected for Windows compatibility
    // http://doc.qt.io/qt-5/windows-requirements.html#graphics-drivers
    QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
