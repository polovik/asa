#include <QDebug>
#include "settings.h"
#include <QSettings>

Settings::Settings(QObject *parent) : QObject(parent)
{
    m_settings = new QSettings("settings.ini", QSettings::IniFormat);
}

Settings *Settings::getSettings()
{
    static Settings *settings = NULL;
    if (settings != NULL) {
        return settings;
    }
    settings = new Settings();
    return settings;
}

Settings::~Settings()
{

}

void Settings::setValue(const QString &key, const QVariant &value)
{
    m_settings->setValue(key, value);
    m_settings->sync();
}

QVariant Settings::value(const QString &key, const QVariant &defaultValue) const
{
    return m_settings->value(key, defaultValue);
}

