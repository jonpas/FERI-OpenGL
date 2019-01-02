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
    QStringList paths = QFileDialog::getOpenFileNames(this, "Select Object(s)", "", "Object (*.obj)");
    resetOpenGLContext();

    if (!paths.empty()) {
        ui->widget->loadModelsFromFile(paths);
    }
}

void MainWindow::on_applyTextureButton_clicked() {
    if (!ui->widget->isMeshObjectSelected()) {
        std::cerr << "Texture can only be applied to a mesh object" << std::endl;
        return;
    }

    QString path = QFileDialog::getOpenFileName(this, "Select Texture", "", "Texture (*.png *.jpg *.jpeg)");
    resetOpenGLContext();

    if (path != "") {
        QStringList mappingTypes = {"Simple", "Planar", "Cylindrical", "Spherical"};
        QStringList mappingAxes = {"X", "Y", "Z"};

        bool mappingTypeOk = false;
        QString mappingType = QInputDialog::getItem(this, "Select Mapping Type", "Texture Mapping Type:", mappingTypes, 0, false, &mappingTypeOk);
        resetOpenGLContext();

        if (mappingTypeOk) {
            bool mappingAxisOk = false;
            QString mappingAxis = QInputDialog::getItem(this, "Select Mapping Axis", "Texture Mapping Axis:", mappingAxes, 0, false, &mappingAxisOk);
            resetOpenGLContext();

            if (mappingAxisOk) {
                uint32_t type = static_cast<uint32_t>(mappingTypes.indexOf(mappingType));
                uint32_t axis = static_cast<uint32_t>(mappingAxes.indexOf(mappingAxis));
                ui->widget->applyTextureFromFile(path, type, axis);
            }
        }
    }
}

void MainWindow::on_applyBumpMapButton_clicked() {
    if (!ui->widget->isMeshObjectSelected()) {
        std::cerr << "Bump map can only be applied to a mesh object" << std::endl;
        return;
    }

    QString path = QFileDialog::getOpenFileName(this, "Select Bump Map", "", "Bump Map (*.png *.jpg *.jpeg)");
    resetOpenGLContext();

    if (path != "") {
        ui->widget->applyBumpMapFromFile(path);
    }
}

void MainWindow::on_lightColorButton_clicked() {
    QColor color = QColorDialog::getColor();
    resetOpenGLContext();

    if (color.isValid()) {
        ui->widget->light.color = {color.redF(), color.greenF(), color.blueF()};
        ui->widget->update(); // Redraw scene
    }
}

void MainWindow::on_objectAmbientColorButton_clicked() {
    if (!ui->widget->isMeshObjectSelected()) {
        std::cerr << "Ambient color can only be applied to a mesh object" << std::endl;
        return;
    }

    QColor color = QColorDialog::getColor();
    resetOpenGLContext();

    if (color.isValid()) {
        MeshObject *object = static_cast<MeshObject *>(ui->widget->selectedObject);
        object->material.ambientColor = {color.redF(), color.greenF(), color.blueF()};
        ui->widget->update(); // Redraw scene
    }
}

void MainWindow::on_objectDiffuseColorButton_clicked() {
    if (!ui->widget->isMeshObjectSelected()) {
        std::cerr << "Diffuse color can only be applied to a mesh object" << std::endl;
        return;
    }

    QColor color = QColorDialog::getColor();
    resetOpenGLContext();

    if (color.isValid()) {
        MeshObject *object = static_cast<MeshObject *>(ui->widget->selectedObject);
        object->material.diffuseColor = {color.redF(), color.greenF(), color.blueF()};
        ui->widget->update(); // Redraw scene
    }
}

void MainWindow::on_objectSpecularColorButton_clicked() {
    if (!ui->widget->isMeshObjectSelected()) {
        std::cerr << "Specular color can only be applied to a mesh object" << std::endl;
        return;
    }

    QColor color = QColorDialog::getColor();
    resetOpenGLContext();

    if (color.isValid()) {
        MeshObject *object = static_cast<MeshObject *>(ui->widget->selectedObject);
        object->material.specularColor = {color.redF(), color.greenF(), color.blueF()};
        ui->widget->update(); // Redraw scene
    }
}

void MainWindow::on_objectSpecularPowerSlider_valueChanged(int value) {
    if (!ui->widget->isMeshObjectSelected()) {
        std::cerr << "Shininess can only be applied to a mesh object" << std::endl;
        return;
    }

    MeshObject *object = static_cast<MeshObject *>(ui->widget->selectedObject);
    object->material.specularPower = value;
    ui->widget->update(); // Redraw scene
}
