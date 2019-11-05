#include "tempscheduler.h"
#include "ui_tempscheduler.h"
#include "QDebug"
#include "cfg.h"

TempScheduler::TempScheduler(QWidget *parent, convar *temp_cv, bool *run_temp) :
    QDialog(parent),
    ui(new Ui::TempScheduler)
{
    ui->setupUi(this);
    this->temp_cv = temp_cv;
    this->run_temp = run_temp;

    ui->tempStartBox->setValue(cfg[TempStart]);
    ui->tempEndBox->setValue(cfg[TempEnd]);

    auto time_start = QTime(cfg[HourStart], 0);
    auto time_end   = QTime(cfg[HourEnd], 0);

    ui->timeStartBox->setTime(time_start);
    ui->timeEndBox->setTime(time_end);
}

void TempScheduler::on_buttonBox_accepted()
{
    LOGI << temp_start_kelvin << "K -> " << temp_end_kelvin << " K";
    LOGI << time_start << " -> " << time_end;

    auto cur_time = QTime::currentTime().toString();

    int cur_hour = QStringRef(&cur_time, 0, 2).toInt();
    int cur_min  = QStringRef(&cur_time, 3, 2).toInt();

    LOGI << "Current hour: " << cur_hour << ':' << cur_min;

    cfg[HourStart]  = QStringRef(&time_start, 0, 2).toInt();
    cfg[HourEnd]    = QStringRef(&time_end, 0, 2).toInt();

    if(cur_hour >= cfg[HourStart])
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

    saveConfig();
}

void TempScheduler::on_tempStartBox_valueChanged(int arg1)
{
    cfg[TempStart] = arg1;
}

void TempScheduler::on_tempEndBox_valueChanged(int arg1)
{
    cfg[TempEnd] = arg1;
}

void TempScheduler::on_timeStartBox_timeChanged(const QTime &time)
{
    time_start = time.toString();
}

void TempScheduler::on_timeEndBox_timeChanged(const QTime &time)
{
    time_end = time.toString();
}

TempScheduler::~TempScheduler()
{
    delete ui;
}
