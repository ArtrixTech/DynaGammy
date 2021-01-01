/**
 * Copyright (C) Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#include "tempscheduler.h"
#include "ui_tempscheduler.h"
#include "cfg.h"
#include "mediator.h"

TempScheduler::TempScheduler(IMediator *m) :ui(new Ui::TempScheduler), mediator(m)
{
	ui->setupUi(this);

	ui->tempStartBox->setValue(high_temp = cfg["temp_high"]);
	ui->tempEndBox->setValue(low_temp = cfg["temp_low"]);
	ui->doubleSpinBox->setValue(adaptation_time_m = cfg["temp_speed"]);

	const auto sunrise = cfg["temp_sunrise"].get<std::string>();
	sunrise_h = std::stoi(sunrise.substr(0, 2));
	sunrise_m = std::stoi(sunrise.substr(3, 2));

	const auto sunset = cfg["temp_sunset"].get<std::string>();
	sunset_h = std::stoi(sunset.substr(0, 2));
	sunset_m = std::stoi(sunset.substr(3, 2));

	ui->timeStartBox->setTime(QTime(sunset_h, sunset_m));
	ui->timeEndBox->setTime(QTime(sunrise_h, sunrise_m));
}

void TempScheduler::on_buttonBox_accepted()
{
	QTime t_sunrise = QTime(sunrise_h, sunrise_m);
	QTime t_sunset  = QTime(sunset_h, sunset_m);
	QTime t_sunset_adaptated = t_sunset.addSecs(-adaptation_time_m * 60);

	if (t_sunrise >= t_sunset_adaptated) {
		LOGW << "Sunrise time is later or equal to sunset - adaptation. Setting to sunset - adaptation.";
		t_sunrise = t_sunset_adaptated;
	}

	cfg["temp_sunset"]  = t_sunset.toString().toStdString();
	cfg["temp_sunrise"] = t_sunrise.toString().toStdString();
	cfg["temp_high"]    = high_temp;
	cfg["temp_low"]     = low_temp;
	cfg["temp_speed"]   = adaptation_time_m;

	config::write();
	mediator->notify(nullptr, Component::AUTO_TEMP_TOGGLED);
}

void TempScheduler::on_tempStartBox_valueChanged(int val)
{
	high_temp = val;
}

void TempScheduler::on_tempEndBox_valueChanged(int val)
{
	low_temp = val;
}

void TempScheduler::on_timeStartBox_timeChanged(const QTime &time)
{
	sunset_h = time.hour();
	sunset_m = time.minute();
}

void TempScheduler::on_timeEndBox_timeChanged(const QTime &time)
{
	sunrise_h = time.hour();
	sunrise_m = time.minute();
}

void TempScheduler::on_doubleSpinBox_valueChanged(double arg1)
{
	adaptation_time_m = arg1;
}

TempScheduler::~TempScheduler()
{
	delete ui;
}
