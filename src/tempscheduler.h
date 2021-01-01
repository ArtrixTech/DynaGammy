/**
 * Copyright (C) Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifndef TEMPSCHEDULER_H
#define TEMPSCHEDULER_H

#include <QDialog>
#include "utils.h"
#include "defs.h"
#include "component.h"

class Mediator;

namespace Ui {
class TempScheduler;
}

class TempScheduler : public QDialog
{
	Q_OBJECT

public:
	explicit TempScheduler(IMediator *mediator = nullptr);
	~TempScheduler();

private slots:
	void on_buttonBox_accepted();
	void on_tempStartBox_valueChanged(int);
	void on_tempEndBox_valueChanged(int);
	void on_timeStartBox_timeChanged(const QTime &time);
	void on_timeEndBox_timeChanged(const QTime &time);
	void on_doubleSpinBox_valueChanged(double arg1);

private:
	Ui::TempScheduler *ui;

	int sunrise_h, sunrise_m;
	int sunset_h, sunset_m;

	int high_temp;
	int low_temp;
	double adaptation_time_m;

	IMediator *mediator;

	void setDates();
};

#endif // TEMPSCHEDULER_H
