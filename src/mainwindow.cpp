/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifdef _WIN32
	#include <Windows.h>
#endif

#include <QScreen>
#include <QMenu>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusConnection>

#include <thread>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tempscheduler.h"
#include "cfg.h"
#include "defs.h"
#include "screenctl.h"

MainWindow::MainWindow(GammaCtl *gammactl)
        : ui(new Ui::MainWindow), tray_icon(new QSystemTrayIcon(this))
{
	this->gammactl = gammactl;

	init();
}

void MainWindow::init()
{
	if (!windows && !listenWakeupSignal()) {
		LOGE << "Gammy is unable to reset the proper brightness / temperature when resuming from suspend.";
	}

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

		this->setVisible(cfg["show_on_startup"]);

		// Extending brightness range doesn't work yet on Windows
		if (windows)
			ui->extendBr->hide();

		ui->speedWidget->hide();
		ui->threshWidget->hide();
		ui->pollingWidget->hide();

		// Move window to bottom right
		QRect scr = QGuiApplication::primaryScreen()->availableGeometry();
		move(scr.width() - this->width() - wnd_offset_x, scr.height() - this->height() - wnd_offset_y);
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

	// Set sliders
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

	createTrayIcon(icon);

	std::thread t ([&] { checkTray(); });
	t.detach();

	LOGD << "Window initialized";
}

void MainWindow::checkTray()
{
	int c = 3;
	do {
		systray_available = QSystemTrayIcon::isSystemTrayAvailable();

		if (systray_available) {
			tray_icon->show();
			return;
		}

		std::this_thread::sleep_for(std::chrono::seconds(10));
	} while (c--);

	LOGW << "System tray unavailable. Closing the window will quit the app.";
	this->show();
}

void MainWindow::createTrayIcon(QIcon &icon)
{
	QMenu *menu = createMenu();
	tray_icon->setContextMenu(menu);

	if (windows)
		menu->setStyleSheet("color:black");

	tray_icon->setToolTip(QString("Gammy"));
	tray_icon->setIcon(icon);

	connect(tray_icon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);
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
		LOGE << "Wakeup interface not found.";
		return false;
	}

	bool connected = dbus.connect(service, path, interface, name, this, SLOT(wakeupSlot(bool)));

	if (!connected) {
		LOGE << "Cannot connect to wakeup signal.";
		return false;
	}

	return true;
}

void MainWindow::wakeupSlot(bool status)
{
	// The signal emits TRUE when going to sleep. We only care about wakeup (FALSE)
	if (status)
		return;

	LOGD << "Waking up from sleep.";

	std::thread reset ([&] {
		int i = 0;
		while (i++ < 5) {
			LOGD << "Resetting screen (" << i << ")";
			gammactl->screen->setGamma(brt_step, cfg["temp_step"]);
			std::this_thread::sleep_for(std::chrono::seconds(3));
		}
	});

	reset.detach();

	// Force temperature change (will be ignored if disabled)
	gammactl->notify_temp(true);
}

void MainWindow::quit(bool prev_gamma)
{
	this->prev_gamma = prev_gamma;
	gammactl->close();
	tray_icon->hide();
	QApplication::quit();
}

/**
 * This event gets fired when the window is closed or we quit.
 * If we have a system tray, this event gets ignored,
 * so that we only hide the window,
 * while quitting the app gets done elsewhere.
 */
void MainWindow::closeEvent(QCloseEvent *e)
{
	this->hide();
	write();

	if (systray_available)
		e->ignore();
}

void MainWindow::showOnTop()
{
	if (!isHidden())
		return;

	setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);

	// Move the window to bottom right again. For some reason it moves up.
	QRect scr = QGuiApplication::primaryScreen()->availableGeometry();
	move(scr.width() - this->width() - wnd_offset_x, scr.height() - this->height() - wnd_offset_y);

	show();
	updateBrLabel();
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
	run_startup->setChecked(s == ERROR_SUCCESS);
#else
	QAction *show_wnd = new QAction("&Show Gammy", this);

	connect(show_wnd, &QAction::triggered, this, [=] { showOnTop(); });
	menu->addAction(show_wnd);
#endif

	menu->addSeparator();

	QAction *quit_prev = new QAction("&Quit", this);
	connect(quit_prev, &QAction::triggered, this, [=] { quit(true); });
	menu->addAction(quit_prev);

	if (!windows) {
		QAction *quit_pure = new QAction("&Quit (set pure gamma)", this);
		connect(quit_pure, &QAction::triggered, this, [=] { quit(false); });
		menu->addAction(quit_pure);
	}

	menu->addSeparator();

	QAction *about = new QAction("&Gammy " + QString(g_app_version), this);
	about->setEnabled(false);
	menu->addAction(about);

	return menu;
}

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
	if (reason == QSystemTrayIcon::Trigger) {
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

	gammactl->screen->setGamma(brt_step, cfg["temp_step"]);
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
	gammactl->notify_ss();

	// Toggle visibility of br range and offset sliders
	toggleMainBrSliders(checked);

	// Allow adv. settings button input when auto br is enabled
	ui->advBrSettingsBtn->setEnabled(checked);
	ui->advBrSettingsBtn->setChecked(false);

	const auto btn_color = checked ? "color:white" : "color:rgba(0,0,0,0)";
	ui->advBrSettingsBtn->setStyleSheet(btn_color);

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
	gammactl->notify_temp(true);
}

void MainWindow::on_manBrSlider_valueChanged(int value)
{
	brt_step = value;
	cfg["brightness"] = value;
	gammactl->screen->setGamma(brt_step, cfg["temp_step"]);
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

	if (extend)
		br_limit *= 2;

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
	TempScheduler ts(nullptr, gammactl);
	ts.exec();
}

void MainWindow::setPollingRange(int min, int max)
{
	const int poll = cfg["polling_rate"];

	LOGD << "Setting polling rate slider range to: " << min << ", " << max;

	ui->pollingSlider->setRange(min, max);

	if (poll < min)
		cfg["polling_rate"] = min;
	else if (poll > max)
		cfg["polling_rate"] = max;

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
	if (ui->autoCheck->isChecked())
		ui->autoCheck->setChecked(false);
}

void MainWindow::on_tempSlider_sliderPressed()
{
	if (ui->autoTempCheck->isChecked())
		ui->autoTempCheck->setChecked(false);
}

MainWindow::~MainWindow()
{
	delete ui;
}
