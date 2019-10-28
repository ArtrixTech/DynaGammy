/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#include "ui_mainwindow.h"
#include "main.h"
#include "utils.h"
#include "cfg.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <QScreen>
#include <QSystemTrayIcon>
#include <QToolTip>
#include <QHelpEvent>
#include <QAction>
#include <QMenu>

#include <iostream>

#include "mainwindow.h"

#ifndef _WIN32

MainWindow::MainWindow(X11 *x11, convar *auto_cv)
    : ui(new Ui::MainWindow), trayIcon(new QSystemTrayIcon(this))
{
    this->auto_cv = auto_cv;
    this->x11 = x11;

    init();
}
#endif

MainWindow::MainWindow(QWidget *parent, convar *auto_cv)
    : QMainWindow(parent), ui(new Ui::MainWindow), trayIcon(new QSystemTrayIcon(this))
{
    this->auto_cv = auto_cv;

    init();
}

void MainWindow::init()
{
    ui->setupUi(this);

    readConfig();

    QIcon icon = QIcon(":res/icons/128x128ball.ico");

    // Set window properties
    {
        this->setWindowTitle("Gammy");
        this->setWindowIcon(icon);
        resize(335, 333);

        QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);

        this->setWindowFlags(Qt::Dialog |
                             Qt::WindowStaysOnTopHint);

        // Deprecated buttons, to be removed altogether
        ui->closeButton->hide();
        ui->hideButton->hide();

        if constexpr(os == OS::Windows)
        {
            // Extending brightness range doesn't work yet on Windows
            ui->extendBr->hide();
        }

        ui->manBrSlider->hide();

        // Move window to bottom right
        QRect scr = QGuiApplication::primaryScreen()->availableGeometry();
        move(scr.width() - this->width(), scr.height() - this->height());
    }

    // Create tray icon
    {
        if (!QSystemTrayIcon::isSystemTrayAvailable())
        {
#ifdef dbg
            std::cout << "Qt: System tray unavailable.\n";
#endif
            ignore_closeEvent = false;
            MainWindow::show();
        }

        this->trayIcon->setIcon(icon);

        QMenu *menu = createMenu();
        this->trayIcon->setContextMenu(menu);
        this->trayIcon->setToolTip(QString("Gammy"));
        this->trayIcon->show();
        connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);
    }

    // Set slider properties
    {
        ui->extendBr->setChecked(cfg[toggleLimit]);
        setBrSlidersRange(cfg[toggleLimit]);

        ui->tempSlider->setRange(0, temp_arr_entries * temp_mult);
        ui->minBrSlider->setValue(cfg[MinBr]);
        ui->maxBrSlider->setValue(cfg[MaxBr]);
        ui->offsetSlider->setValue(cfg[Offset]);
        ui->speedSlider->setValue(cfg[Speed]);
        ui->tempSlider->setValue(cfg[Temp]);
        ui->thresholdSlider->setValue(cfg[Threshold]);
        ui->pollingSlider->setValue(cfg[Polling_rate]);
    }

    // Set auto brightness toggle
    {
        ui->autoCheck->setChecked(cfg[isAuto]);

        run = cfg[isAuto];
        auto_cv->notify_one();

        toggleSliders(cfg[isAuto]);
    }

    LOGI << "Qt window initialized";
}

QMenu* MainWindow::createMenu()
{
    QMenu *menu = new QMenu(this);

#ifdef _WIN32
    QAction *run_startup = new QAction("&Run at startup", this);
    run_startup->setCheckable(true);

    connect(run_startup, &QAction::triggered, [=]{toggleRegkey(run_startup->isChecked());});
    menu->addAction(run_startup);

    LRESULT s = RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", L"Gammy", RRF_RT_REG_SZ, nullptr, nullptr, nullptr);

    s == ERROR_SUCCESS ? run_startup->setChecked(true):
                         run_startup->setChecked(false);
#else

    QAction *show_wnd = new QAction("&Show Gammy", this);

    auto show_on_top = [&]()
    {
        if(this->isHidden())
        {
            setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);

            // Move the window to bottom right again.
            // For some reason it moves up otherwise.
            QRect scr = QGuiApplication::primaryScreen()->availableGeometry();
            move(scr.width() - this->width(), scr.height() - this->height());

            show();
            updateBrLabel();
        };
    };

    connect(show_wnd, &QAction::triggered, this, show_on_top);
    menu->addAction(show_wnd);
#endif
    menu->addSeparator();

    QAction *quit_prev = new QAction("&Quit", this);
    connect(quit_prev, &QAction::triggered, this, [=]{on_closeButton_clicked();});
    menu->addAction(quit_prev);

#ifndef _WIN32
    QAction *quit_pure = new QAction("&Quit (set pure gamma)", this);
    connect(quit_pure, &QAction::triggered, this, [=]{set_previous_gamma = false; on_closeButton_clicked(); });
    menu->addAction(quit_pure);
#endif

    return menu;
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger)
    {
        MainWindow::updateBrLabel();
        MainWindow::show();
    }
}

void MainWindow::updateBrLabel()
{
    int val = scr_br * 100 / 255;
    ui->statusLabel->setText(QStringLiteral("%1").arg(val));
}

void MainWindow::on_hideButton_clicked()
{
    this->hide();
}

void MainWindow::on_closeButton_clicked()
{
    run = true;
    quit = true;
    auto_cv->notify_one();

    QCloseEvent e;
    e.setAccepted(true);
    emit closeEvent(&e);

    trayIcon->hide();
}

void MainWindow::closeEvent(QCloseEvent* e)
{
    MainWindow::hide();
    saveConfig();

    if(ignore_closeEvent) e->ignore();
}

//___________________________________________________________

void MainWindow::on_minBrSlider_valueChanged(int val)
{   
    ui->minBrLabel->setText(QStringLiteral("%1").arg(val * 100 / 255));

    if(val > cfg[MaxBr])
    {
        ui->maxBrSlider->setValue(cfg[MaxBr] = val);
    }

    cfg[MinBr] = val;
}

void MainWindow::on_maxBrSlider_valueChanged(int val)
{
    ui->maxBrLabel->setText(QStringLiteral("%1").arg(val * 100 / 255));

    if(val < cfg[MinBr])
    {
        ui->minBrSlider->setValue(cfg[MinBr] = val);
    }

    cfg[MaxBr] = val;
}

void MainWindow::on_offsetSlider_valueChanged(int val)
{
    cfg[Offset] = val;

    ui->offsetLabel->setText(QStringLiteral("%1").arg(val * 100 / 255));
}

void MainWindow::on_speedSlider_valueChanged(int val)
{
    cfg[Speed] = val;
}

void MainWindow::on_tempSlider_valueChanged(int val)
{
    cfg[Temp] = val;

    if constexpr(os == OS::Windows)
    {
        setGDIGamma(scr_br, val);
    }
#ifndef _WIN32 // @TODO: replace this
    else x11->setXF86Gamma(scr_br, val);
#endif

    int temp_kelvin = convertToRange(temp_arr_entries * temp_mult - val,
                                     0, temp_arr_entries * temp_mult,
                                     min_temp_kelvin, max_temp_kelvin);

    temp_kelvin = ((temp_kelvin - 1) / 100 + 1) * 100;

    ui->tempLabel->setText(QStringLiteral("%1").arg(temp_kelvin));
}

void MainWindow::on_thresholdSlider_valueChanged(int val)
{
    cfg[Threshold] = val;
}

void MainWindow::on_pollingSlider_valueChanged(int val)
{
    cfg[Polling_rate] = val;
}

void MainWindow::on_autoCheck_stateChanged(int state)
{
    if(state == 2)
    {
        cfg[isAuto] = 1;
        run = true;
        if(force) *force = true;
    }
    else
    {
        cfg[isAuto] = 0;
        run = false;
    }

    toggleSliders(run);
    auto_cv->notify_one();
}

void MainWindow::toggleSliders(bool is_auto)
{
    if(is_auto)
    {
        ui->manBrSlider->hide();
    }
    else
    {
        ui->manBrSlider->setValue(scr_br);
        ui->manBrSlider->show();
    }
}

void MainWindow::on_manBrSlider_valueChanged(int value)
{
    scr_br = value;

    if constexpr(os == OS::Windows)
    {
        setGDIGamma(scr_br, cfg[Temp]);
    }
#ifndef _WIN32 // @TODO: Replace this
    else x11->setXF86Gamma(scr_br, cfg[Temp]);
#endif

    MainWindow::updateBrLabel();
}

void MainWindow::on_extendBr_clicked(bool checked)
{
    cfg[toggleLimit] = checked;

    setBrSlidersRange(cfg[toggleLimit]);
}

void MainWindow::setBrSlidersRange(bool inc)
{
    int br_limit = default_brightness;

    if(inc) br_limit *= 2;

    ui->manBrSlider->setRange(64, br_limit);
    ui->minBrSlider->setRange(64, br_limit);
    ui->maxBrSlider->setRange(64, br_limit);
    ui->offsetSlider->setRange(0, br_limit);
}

void MainWindow::updatePollingSlider(int min, int max)
{
   const int poll = cfg[Polling_rate];

   ui->pollingSlider->setRange(min, max);

   if(poll < min) {
       cfg[Polling_rate] = min;
   }
   else
   if(poll > max) {
       cfg[Polling_rate] = max;
   }

   ui->pollingLabel->setText(QString::number(poll));
   ui->pollingSlider->setValue(poll);
}

MainWindow::~MainWindow()
{
    delete ui;
}
