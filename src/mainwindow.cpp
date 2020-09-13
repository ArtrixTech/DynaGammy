/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifdef _WIN32
	#include <Windows.h>
#endif

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tempscheduler.h"
#include "cfg.h"
#include "defs.h"

#include <QScreen>
#include <QMenu>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusConnection>

#include <thread>

#ifndef _WIN32

MainWindow::MainWindow(X11 *x11, convar *ss_cv, convar *temp_cv)
	: ui(new Ui::MainWindow), trayIcon(new QSystemTrayIcon(this))
{
	this->ss_cv = ss_cv;
	this->temp_cv = temp_cv;
	this->x11 = x11;

	init();
}
#endif

MainWindow::MainWindow(QWidget *parent, convar *ss_cv, convar *temp_cv)
	: QMainWindow(parent), ui(new Ui::MainWindow), trayIcon(new QSystemTrayIcon(this))
{
	this->ss_cv = ss_cv;
	this->temp_cv = temp_cv;

	init();
}

bool MainWindow::listenWakeupSignal()
{
	QDBusConnection dbus = QDBusConnection::systemBus();

	if (!dbus.isConnected()) {
		LOGE << "Cannot connect to the system D-Bus.";
		return false;
	}

	const QString service   = "org.freedesktop.login1";
	const QString path      = "/org/freedesktop/login1";
	const QString interface = "org.freedesktop.login1.Manager";
	const QString name      = "PrepareForSleep";

	QDBusInterface iface(service, path, interface, dbus, this);

	if (!iface.isValid()) {
		LOGE << "Wakeup interface not found. Gammy is unable to reset the proper brightness / temperature when resuming from suspend.";
		return false;
	}

	bool connected = dbus.connect(service, path, interface, name, this, SLOT(wakeupSlot(bool)));

	if(!connected) {
		LOGE << "Cannot connect to wakeup signal. Gammy is unable to reset the proper brightness / temperature when resuming from suspend.";
		return false;
	}

	return true;
}

void MainWindow::wakeupSlot(bool status) {

	// The signal emits TRUE when going to sleep. We only care about wakeup (FALSE)	std::this_thread::sleep_for(std::chrono::seconds(3));
	if(status) { return; }

	LOGD << "Waking up from sleep. Resetting screen... (3 sec)";

	std::this_thread::sleep_for(std::chrono::seconds(3));

	if(!cfg["auto_br"])
	{
		if constexpr(os == OS::Windows) {
			setGDIGamma(cfg["brightness"], cfg["temp_step"]);
		}
#ifndef _WIN32
		else x11->setXF86Gamma(cfg["brightness"], cfg["temp_step"]);
#endif
	}

	// Force temperature change (will be ignored if disabled)
	*force_temp_change = true;
	temp_cv->notify_one();
}

void MainWindow::init()
{
#ifndef _WIN32
	if(!listenWakeupSignal()) {
		LOGE << "Gammy is unable to reset the proper brightness / temperature when resuming from suspend.";
	}
#endif

	ui->setupUi(this);

	QIcon icon = QIcon(":res/icons/128x128ball.ico");

	// Set window properties
	{
		this->setWindowTitle("Gammy");
		this->setWindowIcon(icon);
		this->setMinimumHeight(wnd_height);
		this->setMaximumHeight(wnd_height);

		QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);

		this->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);

		if constexpr(os == OS::Windows)
		{
			// Extending brightness range doesn't work yet on Windows
			ui->extendBr->hide();
		}

		//ui->manBrSlider->hide();
		ui->speedWidget->hide();
		ui->threshWidget->hide();
		ui->pollingWidget->hide();

		// Move window to bottom right
		QRect scr = QGuiApplication::primaryScreen()->availableGeometry();
		move(scr.width() - this->width() - wnd_offset_x, scr.height() - this->height() - wnd_offset_y);
	}

	// Create tray icon
	{
		if (!QSystemTrayIcon::isSystemTrayAvailable())
		{
			LOGW << "System tray unavailable. Closing the settings window will quit the app";

			ignore_closeEvent = false;
			show();
		}

		this->trayIcon->setIcon(icon);

		QMenu *menu = createMenu();
		this->trayIcon->setContextMenu(menu);
		this->trayIcon->setToolTip(QString("Gammy"));
		this->trayIcon->show();
		connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);

		if constexpr(os == Windows)
		{
			menu->setStyleSheet("color:black");
		}

		LOGI << "Tray icon created";
	}

	// Set label text
	{
		ui->statusLabel->setText(QStringLiteral("%1 %").arg(int(remap(brt_step, 0, brt_slider_steps, 0, 100))));
		ui->minBrLabel->setText(QStringLiteral("%1 %").arg(int(remap(cfg["min_br"].get<int>(), 0, brt_slider_steps, 0, 100))));
		ui->maxBrLabel->setText(QStringLiteral("%1 %").arg(int(remap(cfg["max_br"].get<int>(), 0, brt_slider_steps, 0, 100))));
		ui->speedLabel->setText(QStringLiteral("%1 s").arg(cfg["speed"].get<int>()));
		ui->thresholdLabel->setText(QStringLiteral("%1").arg(cfg["threshold"].get<int>()));

		double temp_kelvin = remap(temp_slider_steps - cfg["temp_step"].get<int>(), 0, temp_slider_steps, min_temp_kelvin, max_temp_kelvin);
		temp_kelvin = floor(temp_kelvin / 100) * 100;
		ui->tempLabel->setText(QStringLiteral("%1 K").arg(temp_kelvin));
	}

	// Init sliders
	{
		ui->extendBr->setChecked(cfg["extend_br"]);
		toggleBrtSlidersRange(cfg["extend_br"]);

		ui->manBrSlider->setValue(cfg["brightness"]);
		ui->offsetSlider->setValue(cfg["offset"]);

		ui->tempSlider->setRange(0, temp_slider_steps);
		ui->tempSlider->setValue(cfg["temp_step"]);

		ui->speedSlider->setValue(cfg["speed"]);
		ui->thresholdSlider->setValue(cfg["threshold"]);
		ui->pollingSlider->setValue(cfg["polling_rate"]);
	}

	// Set auto checks
	{
		ui->autoCheck->setChecked(cfg["auto_br"]);
		emit on_autoCheck_toggled(cfg["auto_br"]);

		ui->autoTempCheck->setChecked(cfg["auto_temp"]);
	}

	LOGI << "Window initialized";
}

QMenu* MainWindow::createMenu()
{
	QMenu *menu = new QMenu(this);

#ifdef _WIN32
	QAction *run_startup = new QAction("&Run at startup", this);
	run_startup->setCheckable(true);

	connect(run_startup, &QAction::triggered, [=]{ toggleRegkey(run_startup->isChecked()); });
	menu->addAction(run_startup);

	LRESULT s = RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", L"Gammy", RRF_RT_REG_SZ, nullptr, nullptr, nullptr);

	s == ERROR_SUCCESS ? run_startup->setChecked(true): run_startup->setChecked(false);
#else

	QAction *show_wnd = new QAction("&Show Gammy", this);

	const auto show_on_top = [this]
	{
		if(!this->isHidden()) return;

		setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);

		// Move the window to bottom right again. For some reason it moves up.
		QRect scr = QGuiApplication::primaryScreen()->availableGeometry();
		move(scr.width() - this->width() - wnd_offset_x, scr.height() - this->height() - wnd_offset_y);

		show();
		updateBrLabel();
	};

	connect(show_wnd, &QAction::triggered, this, show_on_top);
	menu->addAction(show_wnd);
#endif

	menu->addSeparator();

	const auto exit = [this] (bool set_previous_gamma)
	{
		// Boolean read before quitting
		this->set_previous_gamma = set_previous_gamma;

		quit = true;
		temp_cv->notify_one();
		ss_cv->notify_one();

		QCloseEvent e;
		e.setAccepted(true);
		emit closeEvent(&e);

		trayIcon->hide();
	};

	QAction *quit_prev = new QAction("&Quit", this);
	connect(quit_prev, &QAction::triggered, this, [=] { exit(true); });
	menu->addAction(quit_prev);

#ifndef _WIN32
	QAction *quit_pure = new QAction("&Quit (set pure gamma)", this);
	connect(quit_pure, &QAction::triggered, this, [=] { exit(false); });
	menu->addAction(quit_pure);
#endif

	return menu;
}

//___________________________________________________________

void MainWindow::on_brRange_lowerValueChanged(int val)
{
	cfg["min_br"] = val;

	val = int(ceil(remap(val, 0, brt_slider_steps, 0, 100)));

	ui->minBrLabel->setText(QStringLiteral("%1 %").arg(val));
}

void MainWindow::on_brRange_upperValueChanged(int val)
{
	cfg["max_br"] = val;

	val = int(ceil(remap(val, 0, brt_slider_steps, 0, 100)));

	ui->maxBrLabel->setText(QStringLiteral("%1 %").arg(val));
}

void MainWindow::updateBrLabel()
{
	int val = int(ceil(remap(brt_step, 0, brt_slider_steps, 0, 100)));

	ui->statusLabel->setText(QStringLiteral("%1 %").arg(val));
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::Trigger)
	{
		MainWindow::updateBrLabel();
		MainWindow::show();
	}
}

void MainWindow::on_offsetSlider_valueChanged(int val)
{
	cfg["offset"] = val;

	ui->offsetLabel->setText(QStringLiteral("%1 %").arg(int(remap(val, 0, brt_slider_steps, 0, 100))));
}

void MainWindow::on_speedSlider_valueChanged(int val)
{
	cfg["speed"] = val;
	ui->speedLabel->setText(QStringLiteral("%1 s").arg(val));
}

void MainWindow::on_tempSlider_valueChanged(int val)
{
	cfg["temp_step"] = val;

	if(this->quit) return;

	if constexpr(os == OS::Windows) {
		setGDIGamma(brt_step, val);
	}
#ifndef _WIN32
	else x11->setXF86Gamma(brt_step, val);
#endif

	double temp_kelvin = remap(temp_slider_steps - val, 0, temp_slider_steps, min_temp_kelvin, max_temp_kelvin);

	temp_kelvin = floor(temp_kelvin / 10) * 10;

	ui->tempLabel->setText(QStringLiteral("%1 K").arg(temp_kelvin));
}

void MainWindow::on_thresholdSlider_valueChanged(int val)
{
	cfg["threshold"] = val;
}

void MainWindow::on_pollingSlider_valueChanged(int val)
{
	cfg["polling_rate"] = val;
}

void MainWindow::on_autoCheck_toggled(bool checked)
{
	cfg["auto_br"] = checked;
	ss_cv->notify_one();

	// Toggle visibility of br range and offset sliders
	toggleMainBrSliders(checked);

	// Allow adv. settings button input when auto br is enabled
	ui->advBrSettingsBtn->setEnabled(checked);
	ui->advBrSettingsBtn->setChecked(false);

	checked ? ui->advBrSettingsBtn->setStyleSheet("color:white") : ui->advBrSettingsBtn->setStyleSheet("color:rgba(0,0,0,0)");

	const int h = checked ? wnd_height : 170;

	this->setMinimumHeight(h);
	this->setMaximumHeight(h);
}

void MainWindow::toggleMainBrSliders(bool checked)
{
	ui->rangeWidget->setVisible(checked);
	ui->offsetWidget->setVisible(checked);
}

void MainWindow::on_advBrSettingsBtn_toggled(bool checked)
{
	ui->speedWidget->setVisible(checked);
	ui->threshWidget->setVisible(checked);
	ui->pollingWidget->setVisible(checked);

	const int h = checked ? wnd_height + 185 : wnd_height;

	this->setMinimumHeight(h);
	this->setMaximumHeight(h);
}

void MainWindow::on_autoTempCheck_toggled(bool checked)
{
	cfg["auto_temp"] = checked;

	if(force_temp_change)
	{
		*force_temp_change = checked;
	}

	temp_cv->notify_one();
}

void MainWindow::on_manBrSlider_valueChanged(int value)
{
	brt_step = value;
	cfg["brightness"] = value;

	if constexpr(os == OS::Windows) {
		setGDIGamma(brt_step, cfg["temp_step"]);
	}
#ifndef _WIN32
	else x11->setXF86Gamma(brt_step, cfg["temp_step"]);
#endif

	updateBrLabel();
}

void MainWindow::on_extendBr_clicked(bool checked)
{
	cfg["extend_br"] = checked;

	toggleBrtSlidersRange(cfg["extend_br"]);
}

void MainWindow::toggleBrtSlidersRange(bool extend)
{
	int br_limit = brt_slider_steps;

	if(extend) br_limit *= 2;

	int max = cfg["max_br"];
	int min = cfg["min_br"];

	ui->brRange->setMaximum(br_limit);

	// We set the upper/lower values again because they reset after setMaximum
	ui->brRange->setUpperValue(max);
	ui->brRange->setLowerValue(min);

	ui->manBrSlider->setRange(100, br_limit);
	ui->offsetSlider->setRange(0, br_limit);
}

void MainWindow::on_pushButton_clicked()
{
	TempScheduler ts(nullptr, temp_cv, force_temp_change);
	ts.exec();
}

void MainWindow::setPollingRange(int min, int max)
{
	const int poll = cfg["polling_rate"];

	LOGD << "Setting polling rate slider range to: " << min << ", " << max;

	ui->pollingSlider->setRange(min, max);

	if(poll < min) {
		cfg["polling_rate"] = min;
	}
	else
	if(poll > max) {
		cfg["polling_rate"] = max;
	}

	ui->pollingLabel->setText(QString::number(poll));
	ui->pollingSlider->setValue(poll);
}

void MainWindow::setTempSlider(int val)
{
	ui->tempSlider->setValue(val);
}

void MainWindow::setBrtSlider(int val)
{
	ui->manBrSlider->setValue(val);
}

void MainWindow::on_manBrSlider_sliderPressed()
{
	if(!ui->autoCheck->isChecked()) return;

	ui->autoCheck->setChecked(false);
}

void MainWindow::on_tempSlider_sliderPressed()
{
	if(!ui->autoTempCheck->isChecked()) return;

	ui->autoTempCheck->setChecked(false);
}

void MainWindow::closeEvent(QCloseEvent *e)
{
	// This event gets fired when we close the window or we quit.
	// If we have a system tray, this event gets ignored
	// Meaning we only hide the window and save the config,
	// while quitting the app gets done elsewhere

	this->hide();
	write();
	if(ignore_closeEvent) e->ignore();
}

MainWindow::~MainWindow()
{
	delete ui;
}
