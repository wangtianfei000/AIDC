#include "RuntimeServer.h"
#include "services/SysMLKernelService.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QTextStream>

namespace {

void runtimeMessageHandler(QtMsgType type,
                           const QMessageLogContext&,
                           const QString& message)
{
    if (type == QtDebugMsg || type == QtInfoMsg || type == QtWarningMsg) {
        return;
    }
    QTextStream stream(stderr);
    stream << message << '\n';
    stream.flush();
    if (type == QtFatalMsg) {
        abort();
    }
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    qInstallMessageHandler(runtimeMessageHandler);
    QCoreApplication::setApplicationName("aidc-sysml-parser-runtime");
    QCoreApplication::setApplicationVersion(Aidc::SysMLKernelService::RuntimeVersion);

    QCommandLineParser parser;
    parser.setApplicationDescription("AIDC read-only SysML v2 parser runtime");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({{"p", "port"}, "Loopback TCP port; 0 selects a free port", "port", "47321"});
    parser.addOption({"max-request-bytes", "Maximum HTTP request body", "bytes", "8388608"});
    parser.addOption({"parse-timeout-ms", "Cooperative parse timeout", "milliseconds", "30000"});
    parser.addOption({"standard-library", "SysML v2 official sysml.library directory", "path"});
    parser.process(app);

    if (parser.isSet("standard-library")) {
        qputenv("AIDC_SYSML_LIBRARY_ROOT", parser.value("standard-library").toLocal8Bit());
    }

    bool portOk = false;
    const uint portValue = parser.value("port").toUInt(&portOk);
    bool sizeOk = false;
    const qint64 maximumBytes = parser.value("max-request-bytes").toLongLong(&sizeOk);
    bool timeoutOk = false;
    const int timeoutMs = parser.value("parse-timeout-ms").toInt(&timeoutOk);
    if (!portOk || portValue > 65535 || !sizeOk || maximumBytes < 1024 ||
        !timeoutOk || timeoutMs < 1) {
        QTextStream(stderr) << "Invalid command-line option value\n";
        return 2;
    }

    Aidc::RuntimeServer server;
    server.setMaximumRequestBytes(maximumBytes);
    server.setParseTimeoutMs(timeoutMs);
    QString error;
    if (!server.listen(static_cast<quint16>(portValue), &error)) {
        QTextStream(stderr) << "Failed to listen: " << error << '\n';
        return 1;
    }

    QJsonObject startup = {
        {"status", "listening"},
        {"address", "127.0.0.1"},
        {"port", static_cast<int>(server.serverPort())}
    };
    QTextStream(stdout) << QJsonDocument(startup).toJson(QJsonDocument::Compact) << '\n';
    QTextStream(stdout).flush();
    return app.exec();
}
