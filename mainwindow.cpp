/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "main.h"
#include <iostream>
#include <fstream>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->minBrLabel->setText(QStringLiteral("%1").arg(MIN_BRIGHTNESS));
    ui->maxBrLabel->setText(QStringLiteral("%1").arg(MAX_BRIGHTNESS));
    ui->offsetLabel->setText(QStringLiteral("%1").arg(OFFSET));
    ui->speedLabel->setText(QStringLiteral("%1").arg(SPEED));
    ui->tempLabel->setText(QStringLiteral("%1").arg(TEMP));
    ui->thresholdLabel->setText(QStringLiteral("%1").arg(THRESHOLD));
    ui->pollingLabel->setText(QStringLiteral("%1").arg(UPDATE_TIME_MS));

    ui->minBrSlider->setValue(MIN_BRIGHTNESS);
    ui->maxBrSlider->setValue(MAX_BRIGHTNESS);
    ui->offsetSlider->setValue(OFFSET);
    ui->speedSlider->setValue(SPEED);
    ui->tempSlider->setValue(TEMP);
    ui->thresholdSlider->setValue(THRESHOLD);
    ui->pollingSlider->setValue(UPDATE_TIME_MS);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateLabels(USHORT labelValue, USHORT targetValue, size_t t, size_t const &stop, short sleeptime) {
            //if (/*visibile*/)
            //{
                while (labelValue != targetValue && stop == t)
                {
                    if (labelValue < targetValue) ++labelValue;
                    else --labelValue;

                    ui->statusLabel->setText(QStringLiteral("%1").arg(labelValue));

                    if (labelValue == MIN_BRIGHTNESS || labelValue == MAX_BRIGHTNESS) return;
                    Sleep(sleeptime);
                }
           //}
        }

void MainWindow::on_statusBtn_clicked()
{
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
