#pragma once

#include <QMainWindow>
#include <QKeyEvent>
#include <QFileDialog>
#include <QColorDialog>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::MainWindow *ui;
    QSet<int> pressedKeys;

    void resetOpenGLContext();

private slots:
    void on_loadObjectButton_clicked();
    void on_applyTextureButton_clicked();
    void on_lightColorButton_clicked();
    void on_objectAmbientColorButton_clicked();
    void on_objectDiffuseColorButton_clicked();
    void on_objectSpecularColorButton_clicked();
    void on_objectSpecularPowerSlider_valueChanged(int value);
};
