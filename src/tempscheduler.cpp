#include "tempscheduler.h"
#include "ui_tempscheduler.h"
#include "cfg.h"

static QTime time_start, time_end;

TempScheduler::TempScheduler(QWidget *parent, convar *temp_cv, bool *force_change) :
	QDialog(parent),
	ui(new Ui::TempScheduler)
{
	ui->setupUi(this);

	this->temp_cv = temp_cv;
	this->force_change = force_change;

	ui->tempStartBox->setValue(high_temp = cfg["temp_high"]);
	ui->tempEndBox->setValue(low_temp = cfg["temp_low"]);

	const auto setTime = [] (QTime &t, const std::string &time_str)
	{
		const auto start_hour	= time_str.substr(0, 2);
		const auto start_min	= time_str.substr(3, 2);

		t = QTime(std::stoi(start_hour), std::stoi(start_min));
	};

	setTime(time_start, cfg["time_start"]);
	ui->timeStartBox->setTime(time_start);

	setTime(time_end, cfg["time_end"]);
	ui->timeEndBox->setTime(time_end);
}

void TempScheduler::on_buttonBox_accepted()
{
	if(time_start <= time_end)
	{
		LOGW << "Start time is earlier or equal to end. Adding 1 hour to start time.";
		time_start = time_end.addSecs(3600);
	}

	cfg["time_start"]	= time_start.toString().toStdString();
	cfg["time_end"] 	= time_end.toString().toStdString();

	cfg["temp_high"]	= high_temp;
	cfg["temp_low"] 	= low_temp;

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
	time_start = time;
}

void TempScheduler::on_timeEndBox_timeChanged(const QTime &time)
{
	time_end = time;
}

TempScheduler::~TempScheduler()
{
    delete ui;
}
