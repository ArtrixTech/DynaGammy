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
    this->temp_start_kelvin = cfg["temp_start_kelvin"];

    ui->tempEndBox->setValue(cfg["temp_end_kelvin"]);
    this->temp_end_kelvin = cfg["temp_end_kelvin"];

    this->time_start = std::string(cfg["hour_start"]).c_str(); // Thankfully not performance critical
    this->time_end = std::string(cfg["hour_end"]).c_str();

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
    cfg["hour_start"]  = this->time_start.toStdString();
    cfg["hour_end"]    = this->time_end.toStdString();

    cfg["temp_start_kelvin"] = this->temp_start_kelvin;
    cfg["temp_end_kelvin"] = this->temp_end_kelvin;

    save();
}

void TempScheduler::on_tempStartBox_valueChanged(int val)
{
    this->temp_start_kelvin = val;
}

void TempScheduler::on_tempEndBox_valueChanged(int val)
{
    this->temp_end_kelvin = val;
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
