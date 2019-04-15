#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool quitClicked = false;

    void updateBrLabel();

private slots:
    void iconActivated(QSystemTrayIcon::ActivationReason reason);

    void on_minBrSlider_valueChanged(int val);
    void on_maxBrSlider_valueChanged(int val);
    void on_offsetSlider_valueChanged(int val);
    void on_speedSlider_valueChanged(int val);
    void on_tempSlider_valueChanged(int val);
    void on_thresholdSlider_valueChanged(int val);
    void on_pollingSlider_valueChanged(int val);

    void on_minBrSlider_sliderReleased();
    void on_maxBrSlider_sliderReleased();
    void on_offsetSlider_sliderReleased();
    void on_speedSlider_sliderReleased();
    void on_tempSlider_sliderReleased();
    void on_thresholdSlider_sliderReleased();
    void on_pollingSlider_sliderReleased();

    void on_closeButton_clicked();
    void on_hideButton_clicked();

private:
    Ui::MainWindow *ui;
    QSystemTrayIcon* trayIcon;
    QMenu* trayIconMenu;
    QMenu* createMenu();

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void on_close_clicked();
    int mouseClickXCoord;
    int mouseClickYCoord;
};

#endif // MAINWINDOW_H
