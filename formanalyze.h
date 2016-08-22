#ifndef FORMANALYZE_H
#define FORMANALYZE_H

#include <QWidget>

namespace Ui {
class FormAnalyze;
}

class FormAnalyze : public QWidget
{
    Q_OBJECT

public:
    explicit FormAnalyze(QWidget *parent = 0);
    ~FormAnalyze();

private:
    Ui::FormAnalyze *ui;
};

#endif // FORMANALYZE_H
