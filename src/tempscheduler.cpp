#include "tempscheduler.h"
#include "ui_tempscheduler.h"
#include "cfg.h"

TempScheduler::TempScheduler(QWidget *parent, convar *temp_cv, bool *force_change) :
	QDialog(parent),
	ui(new Ui::TempScheduler)
{
	ui->setupUi(this);

	this->temp_cv = temp_cv;
	this->force_change = force_change;

	ui->tempStartBox->setValue(high_temp = cfg["temp_high"]);
	ui->tempEndBox->setValue(low_temp = cfg["temp_low"]);
	ui->doubleSpinBox->setValue(temp_speed_min = cfg["temp_speed"]);

	this->start_hr  = std::stoi(cfg["time_start"].get<std::string>().substr(0, 2));
	this->end_hr    = std::stoi(cfg["time_start"].get<std::string>().substr(3, 2));
	this->start_min = std::stoi(cfg["time_end"].get<std::string>().substr(0, 2));
	this->end_min   = std::stoi(cfg["time_end"].get<std::string>().substr(3, 2));

	ui->timeStartBox->setTime(QTime(start_hr, end_hr));
	ui->timeEndBox->setTime(QTime(start_min, end_min));
}

void TempScheduler::on_buttonBox_accepted()
{
	QTime t_start = QTime(start_hr, end_hr);
	QTime t_end = QTime(start_min, end_min);

	if(t_start <= t_end)
	{
		LOGW << "Start time is earlier or equal to end, setting to end time + 1";
		t_start = t_end.addSecs(3600);
	}

	cfg["time_start"] = t_start.toString().toStdString();
	cfg["time_end"]   = t_end.toString().toStdString();

	cfg["temp_high"]  = high_temp;
	cfg["temp_low"]   = low_temp;

	cfg["temp_speed"] = temp_speed_min;

	save();

	*force_change = true;
	temp_cv->notify_one();
}

void TempScheduler::on_tempStartBox_valueChanged(int val)
{
	this->high_temp = val;
}

void TempScheduler::on_tempEndBox_valueChanged(int val)
{
	this->low_temp = val;
}

void TempScheduler::on_timeStartBox_timeChanged(const QTime &time)
{
	this->start_hr = time.hour();
	this->end_hr = time.minute();
}

void TempScheduler::on_timeEndBox_timeChanged(const QTime &time)
{
	this->start_min = time.hour();
	this->end_min = time.minute();
}

TempScheduler::~TempScheduler()
{
	delete ui;
}

void TempScheduler::on_doubleSpinBox_valueChanged(double arg1)
{
	this->temp_speed_min = arg1;
}
