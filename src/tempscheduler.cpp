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

    this->start_temp = cfg["start_temp"];
    ui->tempStartBox->setValue(start_temp);

    this->target_temp = cfg["target_temp"];
    ui->tempEndBox->setValue(target_temp);

    this->time_start = cfg["time_start"].get<std::string>().c_str();
    this->time_end  = cfg["time_end"].get<std::string>().c_str();

    {
        auto hr = QStringRef(&time_start, 0, 2);
        auto min = QStringRef(&time_start, 3, 2);
        LOGI << "Start time: " << hr << ":" << min;

        ui->timeStartBox->setTime(QTime(hr.toInt(), min.toInt()));
    }

    {
        auto hr = QStringRef(&time_end, 0, 2);
        auto min = QStringRef(&time_end, 3, 2);
        LOGI << "End time: " << hr << ":" << min;

        ui->timeEndBox->setTime(QTime(hr.toInt(), min.toInt()));
    }
}

void TempScheduler::on_buttonBox_accepted()
{
    cfg["time_start"]  = this->time_start.toStdString();
    cfg["time_end"]    = this->time_end.toStdString();

    cfg["start_temp"] = this->start_temp;
    cfg["target_temp"] = this->target_temp;

    *needs_change = true;

    save();
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
