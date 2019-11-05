#ifndef TEMPSCHEDULER_H
#define TEMPSCHEDULER_H

#include <QDialog>
#include "utils.h"

namespace Ui {
class TempScheduler;
}

class TempScheduler : public QDialog
{
    Q_OBJECT

public:
    explicit TempScheduler(QWidget *parent = nullptr);
    explicit TempScheduler(QWidget *parent = nullptr, convar *temp_cv = nullptr, bool *run_temp = {});
    ~TempScheduler();

private slots:
    void on_buttonBox_accepted();

    void on_spinBox_valueChanged(int arg1);

    void on_spinBox_2_valueChanged(int arg1);

    void on_timeEdit_timeChanged(const QTime &time);

    void on_timeEdit_2_timeChanged(const QTime &time);

private:
    Ui::TempScheduler *ui;

    bool *run_temp = nullptr;
    convar *temp_cv;

    int temp_start_kelvin = max_temp_kelvin;
    int temp_end_kelvin = min_temp_kelvin;

    QString time_start  = "18:00:00";
    QString time_end    = "06:00:00";
};

#endif // TEMPSCHEDULER_H
