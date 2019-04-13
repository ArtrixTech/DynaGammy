/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "main.h"
#include <Windows.h>
#include <QSystemTrayIcon>
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
    this->setWindowFlags(Qt::WindowStaysOnTopHint /*| Qt::FramelessWindowHint*/);

    auto menu = this->createMenu();

    this->trayIcon->setContextMenu(menu);

    auto appIcon = QIcon(":res/icons/32x32ball.ico");
    this->trayIcon->setIcon(appIcon);
    this->setWindowIcon(appIcon);
    this->trayIcon->show();

    this->show();
    //this->hide();
}

void toggleRegkey(QAction* &startupAction)
{
    LSTATUS s;
    HKEY hKey;
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

            s = RegSetValueExW(hKey, L"Gammy", 0, REG_SZ, (BYTE*)path, int((wcslen(path) * sizeof(WCHAR))));

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
    connect(quitAction, &QAction::triggered, this, &QCoreApplication::quit);

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

void updateFile(std::string &subStr, int val)
{
    std::fstream file("gammySettings.cfg");

    if(file.is_open())
    {
        std::string str = subStr + "=" + std::to_string(val);
        auto offset = 0;

        std::string line;

        while (getline(file, line))
        {
            if(line.find(subStr) == 0)
            {
                file.seekp(offset);
                file << str << std::endl;
                file.close();
                return;
            }

            offset += line.length() + 2;
        }
    }
}

void MainWindow::on_minBrSlider_sliderReleased()
{
    std::string str("minBrightness");
    updateFile(str, MIN_BRIGHTNESS);
}

void MainWindow::on_maxBrSlider_sliderReleased()
{
    std::string str("maxBrightness");
    updateFile(str, MAX_BRIGHTNESS);
}

void MainWindow::on_offsetSlider_sliderReleased()
{
    std::string str("offset");
    updateFile(str, OFFSET);
}

void MainWindow::on_speedSlider_sliderReleased()
{
    std::string str("speed");
    updateFile(str, SPEED);
}

void MainWindow::on_tempSlider_sliderReleased()
{
    std::string str("temp");
    updateFile(str, TEMP);
}

void MainWindow::on_thresholdSlider_sliderReleased()
{
    std::string str("threshold");
    updateFile(str, THRESHOLD);
}

void MainWindow::on_pollingSlider_sliderReleased()
{
    std::string str("updateRate");
    updateFile(str, UPDATE_TIME_MS);
}

/////////////////////////////////////////////////////////


