#ifndef FORMOPTIONS_H
#define FORMOPTIONS_H

#include <QWidget>

namespace Ui {
class FormOptions;
}

class FormOptions : public QWidget
{
    Q_OBJECT

public:
    explicit FormOptions(QWidget *parent = 0);
    ~FormOptions();

public slots:
    void switchApplicationLanguage(int index);

private:
    Ui::FormOptions *ui;
};

#endif // FORMOPTIONS_H
