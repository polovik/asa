#include <QDebug>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include "settings.h"
#include "formoptions.h"
#include "ui_formoptions.h"

FormOptions::FormOptions(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormOptions)
{
    ui->setupUi(this);

    ui->boxLanguage->addItem(QIcon(":/icons/flag_united_states_of_america_usa.ico"), tr("English"), QVariant(QString("en_US")));
    ui->boxLanguage->addItem(QIcon(":/icons/flag_russian_federation.ico"), tr("Russian"), QVariant(QString("ru_RU")));
    Settings *settings = Settings::getSettings();
    QString locale = settings->value("Global/Locale", "en_US").toString();
    if (locale == "en_US") {
        ui->boxLanguage->setCurrentIndex(0);
    } else if (locale == "ru_RU") {
        ui->boxLanguage->setCurrentIndex(1);
    } else {
        qWarning() << "Invalid application's locale:" << locale;
    }
    connect(ui->boxLanguage, SIGNAL(currentIndexChanged(int)), this, SLOT(switchApplicationLanguage(int)));
}

FormOptions::~FormOptions()
{
    delete ui;
}

void FormOptions::switchApplicationLanguage(int index)
{
    Settings *settings = Settings::getSettings();
    QString locale = "";
    if (index >= 0) {
        QVariant data = ui->boxLanguage->itemData(index);
        locale = data.toString();
    } else {
        qWarning() << "Invalid application language index:" << index;
        return;
    }
    qDebug() << "Application language was switched to" << locale;
    settings->setValue("Global/Locale", locale);
    QMessageBox::information(this, tr("Language switching"), tr("Language will be changed after the application restart"));
}
