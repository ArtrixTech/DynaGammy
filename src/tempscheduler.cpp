#include "tempscheduler.h"
#include "ui_tempscheduler.h"
#include "cfg.h"

TempScheduler::TempScheduler(QWidget *parent, convar *temp_cv, bool *needs_change) :
    QDialog(parent),
    ui(new Ui::TempScheduler)
{
    ui->setupUi(this);

    this->temp_cv = temp_cv;
    this->needs_change = needs_change;

    this->start_temp = cfg["temp_initial"];
    ui->tempStartBox->setValue(start_temp);

    this->target_temp = cfg["temp_target"];
    ui->tempEndBox->setValue(target_temp);

    {
        this->time_start = cfg["time_start"].get<std::string>().c_str();
        auto hr          = QStringRef(&time_start, 0, 2);
        auto min         = QStringRef(&time_start, 3, 2);

        ui->timeStartBox->setTime(QTime(hr.toInt(), min.toInt()));
    }

    {
        this->time_end  = cfg["time_end"].get<std::string>().c_str();
        auto hr         = QStringRef(&time_end, 0, 2);
        auto min        = QStringRef(&time_end, 3, 2);

        ui->timeEndBox->setTime(QTime(hr.toInt(), min.toInt()));
    }
}

void TempScheduler::on_buttonBox_accepted()
{
    cfg["time_start"]       = this->time_start.toStdString();
    cfg["time_end"]         = this->time_end.toStdString();

    cfg["temp_initial"]     = this->start_temp;
    cfg["temp_target"]      = this->target_temp;

    QDate start_date    = QDate::currentDate();
    QDate end_date      = start_date.addDays(1);

    cfg["jday_start"]   = start_date.toJulianDay();
    cfg["jday_end"]     = end_date.toJulianDay();

    save();

    *needs_change = true;
}

void TempScheduler::on_tempStartBox_valueChanged(int val)
{
    this->start_temp = val;
}

void TempScheduler::on_tempEndBox_valueChanged(int val)
{
    this->target_temp = val;
}

void TempScheduler::on_timeStartBox_timeChanged(const QTime &time)
{
    this->time_start = time.toString();
}

void TempScheduler::on_timeEndBox_timeChanged(const QTime &time)
{
    this->time_end = time.toString();
}

TempScheduler::~TempScheduler()
{
    delete ui;
}
