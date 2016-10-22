#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QMessageBox>
#include <QDir>
#include <QDebug>
#include <QTextCodec>
#include <QLocale>
#include <QTime>
#include "settings.h"

bool g_verboseOutput = false;
static QTextCodec *logCodec = NULL;
static FILE *logStream = NULL;
QString g_logFilePath = "";

/** @brief For convenient parsing log files, messages have to be formatted as:
 *      level: message (`placeInSource`)
 *  where:
 *      level - Debug, Warning, Critical, Fatal
 *      message - log message
 *      placeInSource - point, where message was emited in format: (`filename:line, function_signature`)
 */
void logging(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = logCodec->fromUnicode(msg);
    
    QString fileName(context.file);
    fileName.remove(0, fileName.lastIndexOf("\\") + 1);
    fileName.remove(0, fileName.lastIndexOf("/") + 1);
    QByteArray file = logCodec->fromUnicode(fileName);
    
    QTime time = QTime::currentTime();
    QString formatedTime = time.toString("hh:mm:ss.zzz");
    fprintf(logStream, "%s ", qPrintable(formatedTime));
    
    switch (type) {
    case QtDebugMsg:
        fprintf(logStream, "Debug: %s (`%s:%u, %s`)\n", localMsg.constData(), file.constData(), context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(logStream, "Warning: %s (`%s:%u, %s`)\n", localMsg.constData(), file.constData(), context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(logStream, "Critical: %s (`%s:%u, %s`)\n", localMsg.constData(), file.constData(), context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(logStream, "Fatal: %s (`%s:%u, %s`)\n", localMsg.constData(), file.constData(), context.line, context.function);
        abort();
        break;
    }
    fflush(logStream);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    //  Configure and redirect log output to stderr or in text file
    QByteArray envVar = qgetenv("RUN_IN_QTCREATOR");   //  this variable is only set when run application in QtCreator
    if (envVar.isEmpty()) {
        g_logFilePath = QString("%1/log_InjectionPumpDiagnostic.txt").arg(QDir::currentPath());
#ifdef Q_OS_WIN
        logStream = _wfopen(g_logFilePath.toStdWString().c_str(), L"w");
#else
        // for Linux - log file will be put in a User's home directory
        logStream = fopen(g_logFilePath.toUtf8().data(), "w");
#endif
    } else {
        logStream = stderr;
    }
    logCodec = QTextCodec::codecForName("Windows-1251");
    qInstallMessageHandler(logging);
    qDebug() << "Start application. Write log to" << g_logFilePath;
    
    QTranslator translator;
    Settings *settings = Settings::getSettings();
    QString locale = settings->value("Global/Locale", "en_US").toString();
    QString filename = QString(":/languages%1lang_%2").arg(QDir::separator()).arg(locale);
    if (translator.load(filename) ){
        QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf8"));
        if (!QCoreApplication::installTranslator(&translator)) {
            qWarning() << "Translator couldn't be installed";
            Q_ASSERT(false);
        }
    } else {
        qWarning() << "Translation file not loaded:" << filename;
    }

    QStringList appArguments = QApplication::arguments();
    if (appArguments.contains("-v")) {
        qDebug() << "Enable verbose output";
        g_verboseOutput = true;
    } else {
        qDebug() << "Verbose output is disabled";
    }
    
    MainWindow w;
    w.show();
    
    return a.exec();
}
