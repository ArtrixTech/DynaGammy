/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "main.h"
#include <Windows.h>
#include <QScreen>
#include <QSystemTrayIcon>
#include <QToolTip>
#include <QHelpEvent>
#include <QAction>
#include <QMenu>
#include <iostream>
#include <fstream>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    trayIcon(new QSystemTrayIcon(this))
{
    ui->setupUi(this);

    ui->minBrLabel->setText(QStringLiteral("%1").arg(MIN_BRIGHTNESS));
    ui->maxBrLabel->setText(QStringLiteral("%1").arg(MAX_BRIGHTNESS));
    ui->offsetLabel->setText(QStringLiteral("%1").arg(OFFSET));
    ui->speedLabel->setText(QStringLiteral("%1").arg(SPEED));
    ui->tempLabel->setText(QStringLiteral("%1").arg(TEMP));
    ui->thresholdLabel->setText(QStringLiteral("%1").arg(THRESHOLD));
    ui->pollingLabel->setText(QStringLiteral("%1").arg(UPDATE_TIME_MS));
    ui->statusLabel->setText(QStringLiteral("%1").arg(scrBr));

    ui->minBrSlider->setValue(MIN_BRIGHTNESS);
    ui->maxBrSlider->setValue(MAX_BRIGHTNESS);
    ui->offsetSlider->setValue(OFFSET);
    ui->speedSlider->setValue(SPEED);
    ui->tempSlider->setValue(TEMP);
    ui->thresholdSlider->setValue(THRESHOLD);
    ui->pollingSlider->setValue(UPDATE_TIME_MS);

    this->setWindowTitle("Gammy");
    this->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);

    auto appIcon = QIcon(":res/icons/32x32ball.ico");
    this->setWindowIcon(appIcon);

    /*Tray icon */
    auto menu = this->createMenu();
    this->trayIcon->setContextMenu(menu);
    this->trayIcon->setIcon(appIcon);
    this->trayIcon->setToolTip(QString("Gammy"));
    this->trayIcon->show();
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);

    /*Move window to bottom right */
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    int h = screenGeometry.height();
    int w = screenGeometry.width();
    this->move(w - this->width(), h - this->height());
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    mouseClickXCoord = event->x();
    mouseClickYCoord = event->y();
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    move(event->globalX()-mouseClickXCoord,event->globalY()-mouseClickYCoord);
}

void toggleRegkey(QAction* &startupAction)
{
    LSTATUS s;
    HKEY hKey = nullptr;
    LPCWSTR subKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

    if (startupAction->isChecked())
    {
        WCHAR path[MAX_PATH + 3], tmpPath[MAX_PATH + 3];
        GetModuleFileName(nullptr, tmpPath, MAX_PATH + 1);
        wsprintfW(path, L"\"%s\"", tmpPath);

        s = RegCreateKeyExW(HKEY_CURRENT_USER, subKey, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_64KEY, nullptr, &hKey, nullptr);

        if (s == ERROR_SUCCESS)
        {
            #ifdef dbg
            printf("RegKey opened. \n");
            #endif

            s = RegSetValueExW(hKey, L"Gammy", 0, REG_SZ, LPBYTE(path), int((wcslen(path) * sizeof(WCHAR))));

            #ifdef dbg
                if (s == ERROR_SUCCESS) {
                    printf("RegValue set.\n");
                }
                else {
                    printf("Error when setting RegValue.\n");
                }
            #endif
        }
        #ifdef dbg
        else {
            printf("Error when opening RegKey.\n");
        }
        #endif
    }
    else {
        s = RegDeleteKeyValueW(HKEY_CURRENT_USER, subKey, L"Gammy");

        #ifdef dbg
            if (s == ERROR_SUCCESS)
                printf("RegValue deleted.\n");
            else
                printf("RegValue deletion failed.\n");
        #endif
    }

    if(hKey) RegCloseKey(hKey);
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger)
    {
        MainWindow::updateBrLabel();
        MainWindow::show();
    }
}

QMenu* MainWindow::createMenu()
{
    QAction* startupAction = new QAction("&Run at startup", this);
    startupAction->setCheckable(true);
    connect(startupAction, &QAction::triggered, this, std::bind(toggleRegkey, startupAction));

    LRESULT s = RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", L"Gammy", RRF_RT_REG_SZ, nullptr, nullptr, nullptr);

    if (s == ERROR_SUCCESS)
    {
        startupAction->setChecked(true);
    }
    else startupAction->setChecked(false);

    QAction* quitAction = new QAction("&Quit Gammy", this);
    connect(quitAction, &QAction::triggered, this, [&]{on_closeButton_clicked();});

    auto menu = new QMenu(this);
    menu->addAction(startupAction);
    menu->addAction(quitAction);

    return menu;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateBrLabel() {
    ui->statusLabel->setText(QStringLiteral("%1").arg(scrBr));
}

void MainWindow::on_minBrSlider_valueChanged(int val)
{
    if(val > MAX_BRIGHTNESS) val = MAX_BRIGHTNESS;
    MIN_BRIGHTNESS = val;
}

void MainWindow::on_maxBrSlider_valueChanged(int val)
{
    if(val < MIN_BRIGHTNESS) val = MIN_BRIGHTNESS;
    MAX_BRIGHTNESS = val;
}

void MainWindow::on_offsetSlider_valueChanged(int val)
{
    OFFSET = val;
}

void MainWindow::on_speedSlider_valueChanged(int val)
{
    SPEED = val;
}

void MainWindow::on_tempSlider_valueChanged(int val)
{
    TEMP = val;
    setGDIBrightness(scrBr, gdivs[TEMP-1], bdivs[TEMP-1]);
}

void MainWindow::on_thresholdSlider_valueChanged(int val)
{
    THRESHOLD = val;
}

void MainWindow::on_pollingSlider_valueChanged(int val)
{
    UPDATE_TIME_MS = val;
}

/////////////////////////////////////////////////////////

void updateFile()
{
    std::ofstream file("gammySettings.cfg", std::ofstream::out | std::ofstream::trunc);

    if(file.is_open())
    {
        std::string newLines [] =
        {
            "minBrightness=" + std::to_string(MIN_BRIGHTNESS),
            "maxBrightness=" + std::to_string(MAX_BRIGHTNESS),
            "offset=" + std::to_string(OFFSET),
            "speed=" + std::to_string(SPEED),
            "temp=" + std::to_string(TEMP),
            "threshold=" + std::to_string(THRESHOLD),
            "updateRate=" + std::to_string(UPDATE_TIME_MS)
        };

        for(auto s : newLines) file << s << std::endl;
        file.close();
    }
}

void MainWindow::on_minBrSlider_sliderReleased()
{
    updateFile();
}

void MainWindow::on_maxBrSlider_sliderReleased()
{
    updateFile();
}

void MainWindow::on_offsetSlider_sliderReleased()
{
    updateFile();
}

void MainWindow::on_speedSlider_sliderReleased()
{
    updateFile();
}

void MainWindow::on_tempSlider_sliderReleased()
{
    updateFile();
}

void MainWindow::on_thresholdSlider_sliderReleased()
{
    updateFile();
}

void MainWindow::on_pollingSlider_sliderReleased()
{
    updateFile();
}

/////////////////////////////////////////////////////////

void MainWindow::on_closeButton_clicked()
{
    MainWindow::quitClicked = true;
}

void MainWindow::on_hideButton_clicked()
{
    this->hide();
}
