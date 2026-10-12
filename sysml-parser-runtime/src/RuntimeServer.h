#pragma once

#include <QByteArray>
#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <atomic>
#include <memory>

namespace Aidc {

class RuntimeServer : public QObject {
    Q_OBJECT

public:
    explicit RuntimeServer(QObject* parent = nullptr);

    bool listen(quint16 port, QString* errorMessage = nullptr);
    quint16 serverPort() const;
    void setMaximumRequestBytes(qint64 bytes);
    void setParseTimeoutMs(int milliseconds);

private:
    struct Request {
        QByteArray method;
        QByteArray path;
        QHash<QByteArray, QByteArray> headers;
        QByteArray body;
    };

    void acceptConnections();
    void readRequest(QTcpSocket* socket);
    void route(QTcpSocket* socket, const Request& request);
    void startParse(QTcpSocket* socket, const QJsonObject& body, const QByteArray& origin);
    void startAnalyze(QTcpSocket* socket, const QJsonObject& body, const QByteArray& origin);
    void handleCancel(QTcpSocket* socket, const QJsonObject& body, const QByteArray& origin);
    void writeJson(QTcpSocket* socket,
                   int status,
                   const QJsonObject& value,
                   const QByteArray& origin = {});
    void writeError(QTcpSocket* socket,
                    int status,
                    const QString& code,
                    const QString& message,
                    const QByteArray& origin = {});
    bool originAllowed(const QByteArray& origin) const;
    static bool decodeRequest(const QByteArray& buffer,
                              Request* request,
                              qsizetype* consumed,
                              qint64 maximumBytes,
                              QString* error);

    QTcpServer m_server;
    QHash<QTcpSocket*, QByteArray> m_buffers;
    QHash<QString, std::shared_ptr<std::atomic_bool>> m_activeRequests;
    qint64 m_maximumRequestBytes = 8 * 1024 * 1024;
    int m_parseTimeoutMs = 30000;
};

} // namespace Aidc
