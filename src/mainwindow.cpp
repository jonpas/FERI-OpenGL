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
