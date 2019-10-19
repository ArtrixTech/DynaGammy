#include "tempscheduler.h"
#include "ui_tempscheduler.h"

TempScheduler::TempScheduler(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TempScheduler)
{
    ui->setupUi(this);
}

TempScheduler::~TempScheduler()
{
    delete ui;
}
