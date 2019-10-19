#ifndef TEMPSCHEDULER_H
#define TEMPSCHEDULER_H

#include <QDialog>

namespace Ui {
class TempScheduler;
}

class TempScheduler : public QDialog
{
    Q_OBJECT

public:
    explicit TempScheduler(QWidget *parent = nullptr);
    ~TempScheduler();

private:
    Ui::TempScheduler *ui;
};

#endif // TEMPSCHEDULER_H
