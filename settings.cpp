#include <QDebug>
#include "settings.h"
#include <QSettings>

Settings::Settings(QObject *parent) : QObject(parent)
{
    // On Windows settings are stored in the application's folder - because application run out of box without installation
    // On Unix settings are stored in $HOME/.config/asa/asa.ini
#ifdef Q_OS_WIN
    m_settings = new QSettings("settings.ini", QSettings::IniFormat);
#else
    m_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "asa", "asa");
#endif
}

Settings *Settings::getSettings()
{
    static Settings *settings = nullptr;
    if (settings != nullptr) {
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

