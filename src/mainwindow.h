/**
 * Copyright (C) Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHelpEvent>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QAbstractSlider>

#include "component.h"
#include "mediator.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow, public Component
{
	Q_OBJECT

public:
	explicit MainWindow();
	~MainWindow();

	bool prev_gamma = true;

	void init();
	void setTempSlider(int);
	void setBrtSlider(int);
	void setPollingRange(int, int);
	void quit(bool prev_gamma);

private slots:
	void iconActivated(QSystemTrayIcon::ActivationReason reason);

	void on_brtSlider_actionTriggered(int action);
	void on_tempSlider_actionTriggered(int action);

	void on_brtSlider_valueChanged(int val);
	void on_tempSlider_valueChanged(int val);

	void on_brRange_lowerValueChanged(int val);
	void on_brRange_upperValueChanged(int val);
	void on_offsetSlider_valueChanged(int val);
	void on_speedSlider_valueChanged(int val);

	void on_thresholdSlider_valueChanged(int val);
	void on_pollingSlider_valueChanged(int val);
	void on_extendBr_clicked(bool checked);
	void on_autoBrtCheck_toggled(bool checked);
	void on_autoTempCheck_toggled(bool checked);
	void on_pushButton_clicked();

	void on_advBrSettingsBtn_toggled(bool checked);
	void wakeupSlot(bool);
private:
	Ui::MainWindow  *ui;
	QSystemTrayIcon *tray_icon;
	QMenu *createMenu();

	bool listenWakeupSignal();
	void setWindowProperties(QIcon &icon);
	void setLabels();
	void setSliders();
	void toggleBrtSliders(bool show);
	void toggleBrtSlidersRange(bool);
	void updateBrtLabel(int);
	void updateTempLabel(int);
	void closeEvent(QCloseEvent *);
	void createTrayIcon(QIcon &icon);
	void checkTray();
	bool systray_available = false;

	void showOnTop();

	int wnd_offset_x = 17;
	int wnd_offset_y = 35;
	int wnd_height   = 300;
};

#endif // MAINWINDOW_H
