#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "main.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->minBrSlider, SIGNAL(valueChanged(int)), ui->minBrLabel, SLOT(setNum(int)));

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
    ui->statusLabel->setText(QStringLiteral("%1").arg(res));
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
    setGDIBrightness(res, gdivs[TEMP-1], bdivs[TEMP-1]);
}

void MainWindow::on_thresholdSlider_valueChanged(int val)
{
    THRESHOLD = val;
}

void MainWindow::on_pollingSlider_valueChanged(int val)
{
    UPDATE_TIME_MS = val;
}
