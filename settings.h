#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QVariant>

class QSettings;

class Settings : public QObject
{
    Q_OBJECT
public:
    static Settings *getSettings();
    ~Settings();

    void setValue(const QString &key, const QVariant &value);
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;

private:
    Settings(QObject *parent = 0);

    QSettings *m_settings;
};

#endif // SETTINGS_H
