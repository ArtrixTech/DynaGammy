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

    void on_tempStartBox_valueChanged(int);

    void on_tempEndBox_valueChanged(int);

    void on_timeStartBox_timeChanged(const QTime &time);

    void on_timeEndBox_timeChanged(const QTime &time);

private:
    Ui::TempScheduler *ui;

    bool *run_temp = nullptr;
    convar *temp_cv;

    int start_temp = max_temp_kelvin;
    int target_temp = min_temp_kelvin;

    QString time_start  = "18:00";
    QString time_end    = "06:00";
};

#endif // TEMPSCHEDULER_H
