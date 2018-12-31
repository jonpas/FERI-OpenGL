#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    this->installEventFilter(this);

    // Link WidgetOpenGLDraw and ComboBox (object selection)
    ui->widget->objectSelection = ui->objectSelection;
}

MainWindow::~MainWindow() {
    delete ui;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
    if (event->type() == QEvent::KeyPress) {
        pressedKeys += keyEvent->key();
        ui->widget->handleKeys(pressedKeys, keyEvent->modifiers());
        return true;
    } else if (event->type() == QEvent::KeyRelease) {
        pressedKeys -= keyEvent->key();
        ui->widget->handleKeys(pressedKeys, keyEvent->modifiers());
        return true;
    } else {
        // Standard event processing
        return QObject::eventFilter(obj, event);
    }
}

void MainWindow::resetOpenGLContext() {
    // This function has to be called for working with OpenGL
    // We are executing OpenGL on this surface (applications may have more drawing surfaces!)
    // Dialogs for opening files have their own surfaces and with that their own context!
    // http://doc.qt.io/qt-5/qopenglwidget.html#makeCurrent
    ui->widget->makeCurrent();
}

void MainWindow::on_loadObjectButton_clicked() {
    QStringList paths = QFileDialog::getOpenFileNames(this, "Select Object(s)", "Object (*.obj)");

    resetOpenGLContext();

    if (!paths.empty()) {
        ui->widget->loadModelsFromFile(paths);
    }
}

void MainWindow::on_applyTextureButton_clicked() {
    QString path = QFileDialog::getOpenFileName(this, "Select Texture", "Texture (*.png *.jpg *.jpeg)");

    resetOpenGLContext();

    if (path != "") {
        ui->widget->applyTextureFromFile(path);
    }
}
