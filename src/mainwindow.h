/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>

#ifndef _WIN32
#include "x11.h"
#undef Bool
#endif

#include "utils.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr, convar *auto_cv = nullptr, convar *temp_cv = nullptr);

#ifndef _WIN32
	explicit MainWindow(X11 *x11, convar *auto_cv = nullptr, convar *temp_cv = nullptr);
#endif

	~MainWindow();

	bool quit		= false;
	bool set_previous_gamma = true;
	bool ignore_closeEvent	= true;

	void updateBrLabel();
	void setPollingRange(int, int);

	bool run_ss_thread	= true;
	bool *force		= nullptr;
	convar *auto_cv		= nullptr;

	bool run_temp_thread	= false;
	bool *force_temp_change = nullptr;
	convar *temp_cv		= nullptr;

	void setTempSlider(int);

private slots:
	void init();
	void iconActivated(QSystemTrayIcon::ActivationReason reason);

	void on_minBrSlider_valueChanged(int val);
	void on_maxBrSlider_valueChanged(int val);
	void on_offsetSlider_valueChanged(int val);
	void on_speedSlider_valueChanged(int val);
	void on_tempSlider_valueChanged(int val);
	void on_thresholdSlider_valueChanged(int val);
	void on_pollingSlider_valueChanged(int val);

	void on_closeButton_clicked(bool set_previous_gamma);
	void on_hideButton_clicked();

	void on_manBrSlider_valueChanged(int value);

	void on_extendBr_clicked(bool checked);

	void on_autoCheck_toggled(bool checked);

	void on_autoTempCheck_toggled(bool checked);

	void on_pushButton_clicked();

	void on_tempSlider_sliderPressed();

private:
	Ui::MainWindow *ui;
	QSystemTrayIcon *trayIcon;
	QMenu *createMenu();
	void toggleSliders(bool show);
	void setBrSlidersRange(bool);
	void closeEvent(QCloseEvent *);

	#ifndef _WIN32
	X11 *x11;
	#endif
};

#endif // MAINWINDOW_H
