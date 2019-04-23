/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "main.h"
#include <Windows.h>
#include <array>
#include <QScreen>
#include <QSystemTrayIcon>
#include <QToolTip>
#include <QHelpEvent>
#include <QAction>
#include <QMenu>
#include <iostream>
#include <fstream>

void toggleRegkey(bool isChecked)
{
    LSTATUS s;
    HKEY hKey = nullptr;
    LPCWSTR subKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

    if (isChecked)
    {
        WCHAR path[MAX_PATH + 3], tmpPath[MAX_PATH + 3];
        GetModuleFileName(nullptr, tmpPath, MAX_PATH + 1);
        wsprintfW(path, L"\"%s\"", tmpPath);

        s = RegCreateKeyExW(HKEY_CURRENT_USER, subKey, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_64KEY, nullptr, &hKey, nullptr);

        if (s == ERROR_SUCCESS)
        {
            #ifdef dbg
            std::cout << "RegKey opened.\n";
            #endif

            s = RegSetValueExW(hKey, L"Gammy", 0, REG_SZ, LPBYTE(path), int((wcslen(path) * sizeof(WCHAR))));

            #ifdef dbg
                if (s == ERROR_SUCCESS) {
                    std::cout << "RegValue set.\n";
                }
                else {
                    std::cout << "Error when setting RegValue.\n";
                }
            #endif
        }
        #ifdef dbg
        else {
            std::cout << "Error when opening RegKey.\n";
        }
        #endif
    }
    else {
        s = RegDeleteKeyValueW(HKEY_CURRENT_USER, subKey, L"Gammy");

        #ifdef dbg
            if (s == ERROR_SUCCESS)
                std::cout << "RegValue deleted.\n";
            else
                std::cout << "RegValue deletion failed.\n";
        #endif
    }

    if(hKey) RegCloseKey(hKey);
}

void updateFile()
{
    std::ofstream file("gammySettings.cfg", std::ofstream::out | std::ofstream::trunc);

    if(file.is_open())
    {
        std::array<std::string, settings_count> lines = {
            "minBrightness=" + std::to_string(min_brightness),
            "maxBrightness=" + std::to_string(max_brightness),
            "offset=" + std::to_string(offset),
            "speed=" + std::to_string(speed),
            "temp=" + std::to_string(temp),
            "threshold=" + std::to_string(threshold),
            "updateRate=" + std::to_string(polling_rate_ms)
        };

        for(const auto &s : lines) file << s << '\n';
        file.close();
    }
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow), trayIcon(new QSystemTrayIcon(this))
{
    ui->setupUi(this);

    auto appIcon = QIcon(":res/icons/32x32ball.ico");

    /*Set window properties */
    {
        this->setWindowTitle("Gammy");
        this->setWindowIcon(appIcon);
        this->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
        this->setWindowOpacity(0.95);
    }

    /*Move window to bottom right */
    {
        QScreen *screen = QGuiApplication::primaryScreen();
        QRect screenGeometry = screen->availableGeometry();
        int h = screenGeometry.height();
        int w = screenGeometry.width();
        this->move(w - this->width(), h - this->height());
    }

    /*Create tray icon */
    {
        auto menu = this->createMenu();
        this->trayIcon->setContextMenu(menu);
        this->trayIcon->setIcon(appIcon);
        this->trayIcon->setToolTip(QString("Gammy"));
        this->trayIcon->show();
        connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);
    }

    /*Set slider properties*/
    {
        ui->minBrLabel->setText(QStringLiteral("%1").arg(min_brightness));
        ui->maxBrLabel->setText(QStringLiteral("%1").arg(max_brightness));
        ui->offsetLabel->setText(QStringLiteral("%1").arg(offset));
        ui->speedLabel->setText(QStringLiteral("%1").arg(speed));
        ui->tempLabel->setText(QStringLiteral("%1").arg(temp));
        ui->thresholdLabel->setText(QStringLiteral("%1").arg(threshold));
        ui->statusLabel->setText(QStringLiteral("%1").arg(scrBr));

        ui->minBrSlider->setValue(min_brightness);
        ui->maxBrSlider->setValue(max_brightness);
        ui->offsetSlider->setValue(offset);
        ui->speedSlider->setValue(speed);
        ui->tempSlider->setValue(temp);
        ui->thresholdSlider->setValue(threshold);

        /*Set pollingSlider properties*/
        {
            const auto temp = polling_rate_ms; //After setRange, UPDATE_TIME_MS changes to 1000 when using GDI. Perhaps setRange fires valueChanged.
            ui->pollingSlider->setRange(polling_rate_min, polling_rate_max);
            ui->pollingLabel->setText(QString::number(temp));
            ui->pollingSlider->setValue(temp);
        }
    }

    /*Make sliders update settings file*/
    {
        auto signal = &QAbstractSlider::valueChanged;
        auto slot = [=]{
            //Prevents the window from jumping when dragging the slider groove
            mouse = QWidget::mapFromGlobal(QCursor::pos());
            updateFile();
        };

        connect(ui->minBrSlider, signal, slot);
        connect(ui->maxBrSlider, signal, slot);
        connect(ui->offsetSlider, signal, slot);
        connect(ui->speedSlider, signal, slot);
        connect(ui->tempSlider, signal, slot);
        connect(ui->thresholdSlider, signal, slot);
        connect(ui->pollingSlider, signal, slot);
    }
}

QMenu* MainWindow::createMenu()
{
    QAction* startupAction = new QAction("&Run at startup", this);
    startupAction->setCheckable(true);

    connect(startupAction, &QAction::triggered, [=]{toggleRegkey(startupAction->isChecked());});

    LRESULT s = RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", L"Gammy", RRF_RT_REG_SZ, nullptr, nullptr, nullptr);

    if (s == ERROR_SUCCESS)
    {
        startupAction->setChecked(true);
    }
    else startupAction->setChecked(false);

    QAction* quitAction = new QAction("&Quit Gammy", this);
    connect(quitAction, &QAction::triggered, this, [=]{on_closeButton_clicked();});

    auto menu = new QMenu(this);
    menu->addAction(startupAction);
    menu->addAction(quitAction);

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

void MainWindow::updateBrLabel() {
    ui->statusLabel->setText(QStringLiteral("%1").arg(scrBr));
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
   mouse = event->pos();
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    move(event->globalX() - mouse.x(), event->globalY() - mouse.y());
}

void MainWindow::on_hideButton_clicked()
{
    this->hide();
}

void MainWindow::on_closeButton_clicked()
{
    MainWindow::quitClicked = true;
    MainWindow::hide();
    trayIcon->hide();
}

/////////////////////////////////////////////////////////

void MainWindow::on_minBrSlider_valueChanged(int val)
{
    if(val > max_brightness) val = max_brightness;
    min_brightness = val;
}

void MainWindow::on_maxBrSlider_valueChanged(int val)
{
    if(val < min_brightness) val = min_brightness;
    max_brightness = val;
}

void MainWindow::on_offsetSlider_valueChanged(int val)
{
    offset = val;
}

void MainWindow::on_speedSlider_valueChanged(int val)
{
    speed = val;
}

void MainWindow::on_tempSlider_valueChanged(int val)
{
    temp = val;
    setGDIBrightness(scrBr, val);
}

void MainWindow::on_thresholdSlider_valueChanged(int val)
{
    threshold = val;
}

void MainWindow::on_pollingSlider_valueChanged(int val)
{
    polling_rate_ms = val;
}

/////////////////////////////////////////////////////////

MainWindow::~MainWindow()
{
    delete ui;
}
