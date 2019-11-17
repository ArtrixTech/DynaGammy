#include "tempscheduler.h"
#include "ui_tempscheduler.h"
#include "cfg.h"

TempScheduler::TempScheduler(QWidget *parent, convar *temp_cv, bool *run_temp) :
    QDialog(parent),
    ui(new Ui::TempScheduler)
{
    ui->setupUi(this);
    this->temp_cv = temp_cv;
    this->run_temp = run_temp;

    ui->tempStartBox->setValue(cfg["temp_start_kelvin"]);
    ui->tempEndBox->setValue(cfg["temp_end_kelvin"]);

    ui->timeStartBox->setTime(QTime(cfg["hour_start"], 0));
    ui->timeEndBox->setTime(QTime(cfg["hour_end"], 0));
}

void TempScheduler::on_buttonBox_accepted()
{
    cfg["Hour_start"]  = QStringRef(&time_start, 0, 2).toInt();
    cfg["hour_end"]    = QStringRef(&time_end, 0, 2).toInt();

    save();
}

void TempScheduler::on_tempStartBox_valueChanged(int arg1)
{
    cfg["temp_start"] = arg1;
}

void TempScheduler::on_tempEndBox_valueChanged(int arg1)
{
    cfg["temp_end"] = arg1;
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
