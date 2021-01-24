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

	bool prev_gamma = false;
	void init();
	void setTempSlider(int);
	void setBrtSlider(int);
	void setPollingRange(int, int);
	void shutdown();

private slots:
	void trayIconActivated(QSystemTrayIcon::ActivationReason reason);

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
	QMenu *createTrayMenu();

	bool listenWakeupSignal();
	void setWindowProperties(QIcon &icon);
	void setLabels();
	void setSliders();
	void toggleBrtSliders(bool show);
	void toggleBrtSlidersRange(bool);
	void updateBrtLabel(int);
	void updateTempLabel(int);
	void closeEvent(QCloseEvent *);
	void setPos();
	void savePos();
	void restoreDefaultBrt();
	void restoreDefaultTemp();

	void createTrayIcon(QIcon &icon);
	void checkTray();
	bool systray_available = false;

	QAction *tray_wnd_toggle;
	const QString show_txt = "Show Gammy";
	const QString hide_txt = "Hide Gammy";

	QAction *tray_brt_toggle;
	QAction *tray_temp_toggle;

	// Height values
	const int wnd_h_min                 = 170;
	const int wnd_h_min_auto_brt_on     = 282;
	const int c_wdgt_h_max              = 170;
	const int c_wdgt_h_max_auto_brt_on  = 445;
};

#endif // MAINWINDOW_H
