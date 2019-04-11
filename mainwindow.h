#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

     void updateLabels(unsigned short labelValue, unsigned short targetValue, size_t t, size_t const &stop, short sleeptime);

private slots:
    void on_statusBtn_clicked();
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
private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
