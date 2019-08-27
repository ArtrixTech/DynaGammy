#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>

#ifndef _WIN32
#include "x11.h"
#endif

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

#ifndef _WIN32
    explicit MainWindow(X11* x11);
#endif

    ~MainWindow();

    bool quit;
    bool set_previous_gamma;

    void updateBrLabel();
    void updatePollingSlider(int, int);

private slots:
    void init();
    void iconActivated(QSystemTrayIcon::ActivationReason reason);

    void on_minBrSlider_valueChanged(int val);
    void on_maxBrSlider_valueChanged(int val);
    void on_offsetSlider_valueChanged(int val);
    void on_speedSlider_valueChanged(int val);
    void on_tempSlider_valueChanged(int val);
    void on_thresholdSlider_valueChanged(int val);
    void on_pollingSlider_valueChanged(int val);

    void on_closeButton_clicked();
    void on_hideButton_clicked();

private:
    Ui::MainWindow *ui;
    QSystemTrayIcon* trayIcon;
    QMenu* trayIconMenu {};
    QMenu* createMenu();

    QPoint mouse;
    void mousePressEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    bool windowPressed = false;

    void closeEvent(QCloseEvent *);

    #ifndef _WIN32
        X11* x11;
    #endif
};

#endif // MAINWINDOW_H
