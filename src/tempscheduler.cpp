#include "tempscheduler.h"
#include "ui_tempscheduler.h"
#include "QDebug"
TempScheduler::TempScheduler(QWidget *parent, convar *temp_cv, bool *run_temp) :
    QDialog(parent),
    ui(new Ui::TempScheduler)
{
    ui->setupUi(this);
    this->temp_cv = temp_cv;
    this->run_temp = run_temp;
}

void TempScheduler::on_buttonBox_accepted()
{
    LOGI << temp_start_kelvin << " -> " << temp_end_kelvin;

    LOGI << time_start << " -> " << time_end;

    auto cur_time = QTime::currentTime().toString();

    int cur_hour = QStringRef(&cur_time, 0, 2).toInt();
    int cur_min  = QStringRef(&cur_time, 3, 2).toInt();

    LOGI << "Current hour: " << cur_hour << ":" << cur_min;

    int start_hour  = QStringRef(&time_start, 0, 2).toInt();
    int start_min   = QStringRef(&time_end, 3, 2).toInt();

    if(cur_hour > start_hour)
    {
        LOGI << "Adaptive temp should start";
        *run_temp = true;
        temp_cv->notify_one();
    }
    else
    {
        LOGI << "Adaptive temp should stop";
        *run_temp = false;
        temp_cv->notify_one();
    }
}

void TempScheduler::on_spinBox_valueChanged(int arg1)
{
    temp_start_kelvin = arg1;
}

void TempScheduler::on_spinBox_2_valueChanged(int arg1)
{
    temp_end_kelvin = arg1;
}

void TempScheduler::on_timeEdit_timeChanged(const QTime &time)
{
    time_start = time.toString();
}

void TempScheduler::on_timeEdit_2_timeChanged(const QTime &time)
{
    time_end = time.toString();
}

TempScheduler::~TempScheduler()
{
    delete ui;
}
