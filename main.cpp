#include "widget.h"
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QTextBlock>

Widget* msgHandlePointer;
void getMsgHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QByteArray localMsg = msg.toLocal8Bit();
    QString str;
    switch (type) {
    case QtDebugMsg:
        str = QString::asprintf("Debug: %s (%s:%u, %s)", localMsg.constData(), context.file,
                                context.line, context.function);
        break;
    case QtInfoMsg:
        // str = QString::asprintf("Info: %s (%s:%u, %s)", localMsg.constData(),
        // context.file, context.line, context.function);
        str = QString::asprintf("Info: %s", localMsg.constData());
        break;
    case QtWarningMsg:
        str = QString::asprintf("Warning: %s (%s:%u, %s)", localMsg.constData(), context.file,
                                context.line, context.function);
        break;
    case QtCriticalMsg:
        str = QString::asprintf("Critical: %s (%s:%u, %s)", localMsg.constData(), context.file,
                                context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line,
                context.function);
        abort();
    default:
        str = QString::asprintf("Msg No%u: %s (%s:%u, %s)", type, localMsg.constData(),
                                context.file, context.line, context.function);
    }
    fprintf(stderr, str.toStdString().c_str());
    msgHandlePointer->displayMsg(str);
}
int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    QCoreApplication::setApplicationName("NeuroscanAcquireLSL");
    QCoreApplication::setOrganizationName("Neuroscan");

    QCommandLineParser parser;
    parser.setApplicationDescription("Neuroscan Acquire -> LSL bridge");
    parser.addHelpOption();

    QCommandLineOption configOpt(QStringList() << "c"
                                               << "config",
                                 "Config file path (INI).", "path");
    QCommandLineOption hostOpt(QStringList() << "host", "Neuroscan host/IP.", "host");
    QCommandLineOption portOpt(QStringList() << "port", "Neuroscan port.", "port");
    QCommandLineOption autoconnectOpt(QStringList() << "autoconnect", "Auto-connect at startup.");
    QCommandLineOption lslNameOpt(QStringList() << "lsl-name", "LSL stream name.", "name");
    QCommandLineOption lslTypeOpt(QStringList() << "lsl-type", "LSL stream type.", "type");
    QCommandLineOption lslSourceOpt(QStringList() << "lsl-source-id", "LSL stream source id.", "id");
    QCommandLineOption pingOpt(QStringList() << "ping-interval-ms", "Ping interval (ms).", "ms");

    parser.addOption(configOpt);
    parser.addOption(hostOpt);
    parser.addOption(portOpt);
    parser.addOption(autoconnectOpt);
    parser.addOption(lslNameOpt);
    parser.addOption(lslTypeOpt);
    parser.addOption(lslSourceOpt);
    parser.addOption(pingOpt);
    parser.process(a);

    auto firstNonEmpty = [](const QString& a, const QString& b, const QString& c, const QString& d) {
        if (!a.isEmpty())
            return a;
        if (!b.isEmpty())
            return b;
        if (!c.isEmpty())
            return c;
        return d;
    };

    QString configPath = parser.value(configOpt);
    if (configPath.isEmpty()) {
        QString cfgDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        if (cfgDir.isEmpty())
            cfgDir = ".";
        QDir().mkpath(cfgDir);
        configPath = cfgDir + "/neuroscanacquire.ini";
    }
    QSettings settings(configPath, QSettings::IniFormat);

    AppConfig cfg;
    cfg.host = firstNonEmpty(parser.value(hostOpt), qEnvironmentVariable("NEUROSCAN_HOST"),
                             settings.value("connection/host").toString(), "10.0.1.100");
    cfg.port = firstNonEmpty(parser.value(portOpt), qEnvironmentVariable("NEUROSCAN_PORT"),
                             settings.value("connection/port").toString(), "4000")
                   .toInt();
    cfg.autoconnect = parser.isSet(autoconnectOpt) || qEnvironmentVariable("NEUROSCAN_AUTOCONNECT") == "1"
                      || settings.value("connection/autoconnect", false).toBool();
    cfg.lslName = firstNonEmpty(parser.value(lslNameOpt), qEnvironmentVariable("NEUROSCAN_LSL_NAME"),
                                settings.value("lsl/name").toString(), "Neuroscan");
    cfg.lslType = firstNonEmpty(parser.value(lslTypeOpt), qEnvironmentVariable("NEUROSCAN_LSL_TYPE"),
                                settings.value("lsl/type").toString(), "EEG");
    cfg.lslSourceId =
        firstNonEmpty(parser.value(lslSourceOpt), qEnvironmentVariable("NEUROSCAN_LSL_SOURCE_ID"),
                      settings.value("lsl/source_id").toString(), "Neuroscan_sdetsff");
    cfg.pingIntervalMs = firstNonEmpty(parser.value(pingOpt), qEnvironmentVariable("NEUROSCAN_PING_INTERVAL_MS"),
                                       settings.value("transport/ping_interval_ms").toString(), "100")
                             .toInt();

    qRegisterMetaType<QTextBlock>("QTextBlock");
    qRegisterMetaType<QTextCursor>("QTextCursor");
    Widget w(cfg);
    msgHandlePointer = &w;
    qInstallMessageHandler(getMsgHandler);
    w.show();

    return a.exec();
}
