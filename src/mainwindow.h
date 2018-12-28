#pragma once

#include <QMainWindow>
#include <QKeyEvent>
#include <QFileDialog>

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
    void on_openFileButton_clicked();
};
