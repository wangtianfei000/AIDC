#include "RuntimeServer.h"

#include "services/SysMLKernelService.h"

#include <QFutureWatcher>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QTimer>
#include <QUuid>
#include <QtConcurrent>

namespace Aidc {
namespace {

struct WorkerResult {
    QJsonObject body;
    bool canceled = false;
};

QByteArray reasonPhrase(int status)
{
    switch (status) {
    case 200: return "OK";
    case 202: return "Accepted";
    case 400: return "Bad Request";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 408: return "Request Timeout";
    case 409: return "Conflict";
    case 413: return "Content Too Large";
    case 499: return "Client Closed Request";
    default: return "Internal Server Error";
    }
}

} // namespace

RuntimeServer::RuntimeServer(QObject* parent)
    : QObject(parent)
{
    connect(&m_server, &QTcpServer::newConnection, this, &RuntimeServer::acceptConnections);
}

bool RuntimeServer::listen(quint16 port, QString* errorMessage)
{
    const bool ok = m_server.listen(QHostAddress::LocalHost, port);
    if (!ok && errorMessage) {
        *errorMessage = m_server.errorString();
    }
    return ok;
}

quint16 RuntimeServer::serverPort() const
{
    return m_server.serverPort();
}

void RuntimeServer::setMaximumRequestBytes(qint64 bytes)
{
    m_maximumRequestBytes = bytes;
}

void RuntimeServer::setParseTimeoutMs(int milliseconds)
{
    m_parseTimeoutMs = milliseconds;
}

void RuntimeServer::acceptConnections()
{
    while (QTcpSocket* socket = m_server.nextPendingConnection()) {
        m_buffers.insert(socket, {});
        connect(socket, &QTcpSocket::readyRead, this, [this, socket] { readRequest(socket); });
        connect(socket, &QTcpSocket::disconnected, this, [this, socket] {
            m_buffers.remove(socket);
            socket->deleteLater();
        });
        QTimer::singleShot(10000, socket, [this, socket] {
            if (m_buffers.contains(socket)) {
                writeError(socket, 408, "AIDC_REQUEST_TIMEOUT", "HTTP request was not completed in time");
            }
        });
    }
}

void RuntimeServer::readRequest(QTcpSocket* socket)
{
    QByteArray& buffer = m_buffers[socket];
    buffer.append(socket->readAll());
    if (buffer.size() > m_maximumRequestBytes + 16384) {
        writeError(socket, 413, "AIDC_REQUEST_TOO_LARGE", "Request exceeds configured size limit");
        return;
    }

    Request request;
    qsizetype consumed = 0;
    QString error;
    if (!decodeRequest(buffer, &request, &consumed, m_maximumRequestBytes, &error)) {
        if (!error.isEmpty()) {
            writeError(socket, error.contains("size", Qt::CaseInsensitive) ? 413 : 400,
                       "AIDC_BAD_HTTP_REQUEST", error);
        }
        return;
    }
    buffer.remove(0, consumed);
    m_buffers.remove(socket);
    route(socket, request);
}

void RuntimeServer::route(QTcpSocket* socket, const Request& request)
{
    const QByteArray origin = request.headers.value("origin");
    if (!originAllowed(origin)) {
        writeError(socket, 403, "AIDC_ORIGIN_REJECTED", "Only localhost web origins are allowed");
        return;
    }
    if (request.method == "OPTIONS") {
        writeJson(socket, 200, {{"status", "ok"}}, origin);
        return;
    }
    if (request.method == "GET" && request.path == "/health") {
        writeJson(socket, 200, SysMLKernelService::healthResponse(), origin);
        return;
    }
    if (request.method == "GET" && request.path == "/capabilities") {
        writeJson(socket, 200, SysMLKernelService::capabilitiesResponse(), origin);
        return;
    }
    if (request.method != "POST" || (request.path != "/parse" && request.path != "/analyze" && request.path != "/cancel")) {
        writeError(socket, 404, "AIDC_NOT_FOUND", "Unknown runtime endpoint", origin);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(request.body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        writeError(socket, 400, "AIDC_INVALID_JSON", parseError.errorString(), origin);
        return;
    }
    if (request.path == "/parse") {
        startParse(socket, document.object(), origin);
    } else if (request.path == "/analyze") {
        startAnalyze(socket, document.object(), origin);
    } else {
        handleCancel(socket, document.object(), origin);
    }
}

void RuntimeServer::startAnalyze(QTcpSocket* socket,
                                 const QJsonObject& body,
                                 const QByteArray& origin)
{
    const QString uri = body.value("uri").toString();
    const QString text = body.value("text").toString();
    QString requestId = body.value("requestId").toString();
    if (uri.isEmpty() || !body.value("text").isString()) {
        writeError(socket, 400, "AIDC_INVALID_ANALYZE_REQUEST", "uri and text strings are required", origin);
        return;
    }
    if (requestId.isEmpty()) requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (m_activeRequests.contains(requestId)) {
        writeError(socket, 409, "AIDC_DUPLICATE_REQUEST_ID", "requestId is already active", origin);
        return;
    }
    auto canceled = std::make_shared<std::atomic_bool>(false);
    m_activeRequests.insert(requestId, canceled);
    socket->setProperty("aidcRequestId", requestId);
    QPointer<QTcpSocket> guardedSocket(socket);
    auto* watcher = new QFutureWatcher<WorkerResult>(this);
    QTimer::singleShot(m_parseTimeoutMs, this, [this, requestId, canceled] {
        if (m_activeRequests.contains(requestId)) canceled->store(true);
    });
    connect(watcher, &QFutureWatcher<WorkerResult>::finished, this,
            [this, watcher, guardedSocket, requestId, origin] {
        const WorkerResult result = watcher->result();
        m_activeRequests.remove(requestId); watcher->deleteLater();
        if (!guardedSocket) return;
        if (result.canceled) writeError(guardedSocket, 499, "AIDC_ANALYSIS_CANCELED",
                                        "Analysis was canceled or exceeded the soft timeout", origin);
        else writeJson(guardedSocket, 200, result.body, origin);
    });
    watcher->setFuture(QtConcurrent::run([uri, text, canceled] {
        const KernelAnalysisOutcome analyzed =
            SysMLKernelService().analyze(uri, text, canceled.get());
        return WorkerResult{analyzed.snapshot, analyzed.canceled};
    }));
}

void RuntimeServer::startParse(QTcpSocket* socket,
                               const QJsonObject& body,
                               const QByteArray& origin)
{
    const QString uri = body.value("uri").toString();
    const QString text = body.value("text").toString();
    QString requestId = body.value("requestId").toString();
    if (uri.isEmpty() || !body.value("text").isString()) {
        writeError(socket, 400, "AIDC_INVALID_PARSE_REQUEST", "uri and text strings are required", origin);
        return;
    }
    if (requestId.isEmpty()) {
        requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    if (m_activeRequests.contains(requestId)) {
        writeError(socket, 409, "AIDC_DUPLICATE_REQUEST_ID", "requestId is already active", origin);
        return;
    }

    auto canceled = std::make_shared<std::atomic_bool>(false);
    m_activeRequests.insert(requestId, canceled);
    socket->setProperty("aidcRequestId", requestId);
    QPointer<QTcpSocket> guardedSocket(socket);
    auto* watcher = new QFutureWatcher<WorkerResult>(this);

    QTimer::singleShot(m_parseTimeoutMs, this, [this, requestId, canceled] {
        if (m_activeRequests.contains(requestId)) {
            canceled->store(true);
        }
    });

    connect(watcher, &QFutureWatcher<WorkerResult>::finished, this,
            [this, watcher, guardedSocket, requestId, origin] {
        const WorkerResult result = watcher->result();
        m_activeRequests.remove(requestId);
        watcher->deleteLater();
        if (!guardedSocket) {
            return;
        }
        if (result.canceled) {
            writeError(guardedSocket, 499, "AIDC_PARSE_CANCELED",
                       "Parse request was canceled or exceeded the soft timeout", origin);
        } else {
            writeJson(guardedSocket, 200, result.body, origin);
        }
    });

    watcher->setFuture(QtConcurrent::run([uri, text, canceled] {
        const ParseOutcome outcome = SysMLKernelService().parse(uri, text, canceled.get());
        return WorkerResult{outcome.snapshot, outcome.canceled};
    }));
}

void RuntimeServer::handleCancel(QTcpSocket* socket,
                                 const QJsonObject& body,
                                 const QByteArray& origin)
{
    const QString requestId = body.value("requestId").toString();
    const auto it = m_activeRequests.find(requestId);
    const bool accepted = !requestId.isEmpty() && it != m_activeRequests.end();
    if (accepted) {
        it.value()->store(true);
    }
    writeJson(socket, accepted ? 202 : 200,
              {{"requestId", requestId}, {"accepted", accepted}}, origin);
}

void RuntimeServer::writeJson(QTcpSocket* socket,
                              int status,
                              const QJsonObject& value,
                              const QByteArray& origin)
{
    if (!socket || socket->state() == QAbstractSocket::UnconnectedState) {
        return;
    }
    const QByteArray body = QJsonDocument(value).toJson(QJsonDocument::Compact);
    QByteArray response = "HTTP/1.1 " + QByteArray::number(status) + " " + reasonPhrase(status) + "\r\n";
    response += "Content-Type: application/json; charset=utf-8\r\n";
    response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    response += "Cache-Control: no-store\r\n";
    response += "Connection: close\r\n";
    const QString requestId = socket->property("aidcRequestId").toString();
    if (!requestId.isEmpty()) {
        response += "X-Request-Id: " + requestId.toUtf8() + "\r\n";
    }
    if (!origin.isEmpty()) {
        response += "Access-Control-Allow-Origin: " + origin + "\r\n";
        response += "Vary: Origin\r\n";
        response += "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
        response += "Access-Control-Allow-Headers: Content-Type\r\n";
        response += "Access-Control-Expose-Headers: X-Request-Id\r\n";
    }
    response += "\r\n";
    response += body;
    socket->write(response);
    socket->disconnectFromHost();
}

void RuntimeServer::writeError(QTcpSocket* socket,
                               int status,
                               const QString& code,
                               const QString& message,
                               const QByteArray& origin)
{
    writeJson(socket, status,
              {{"error", QJsonObject{{"code", code}, {"message", message}}}}, origin);
}

bool RuntimeServer::originAllowed(const QByteArray& origin) const
{
    if (origin.isEmpty() || origin == "null") {
        return true;
    }
    return origin.startsWith("http://localhost:") || origin == "http://localhost" ||
           origin.startsWith("http://127.0.0.1:") || origin == "http://127.0.0.1" ||
           origin.startsWith("https://localhost:") || origin == "https://localhost" ||
           origin.startsWith("https://127.0.0.1:") || origin == "https://127.0.0.1";
}

bool RuntimeServer::decodeRequest(const QByteArray& buffer,
                                  Request* request,
                                  qsizetype* consumed,
                                  qint64 maximumBytes,
                                  QString* error)
{
    const qsizetype headerEnd = buffer.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        return false;
    }
    const QList<QByteArray> lines = buffer.left(headerEnd).split('\n');
    if (lines.isEmpty()) {
        *error = "Missing request line";
        return false;
    }
    const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
    if (requestLine.size() < 2) {
        *error = "Invalid request line";
        return false;
    }
    request->method = requestLine.at(0).toUpper();
    request->path = requestLine.at(1).split('?').first();
    for (int i = 1; i < lines.size(); ++i) {
        const QByteArray line = lines.at(i).trimmed();
        const qsizetype separator = line.indexOf(':');
        if (separator > 0) {
            request->headers.insert(line.left(separator).trimmed().toLower(),
                                    line.mid(separator + 1).trimmed());
        }
    }
    bool lengthOk = true;
    const qint64 contentLength = request->headers.value("content-length", "0").toLongLong(&lengthOk);
    if (!lengthOk || contentLength < 0) {
        *error = "Invalid Content-Length";
        return false;
    }
    if (contentLength > maximumBytes) {
        *error = "Request body exceeds size limit";
        return false;
    }
    const qsizetype total = headerEnd + 4 + contentLength;
    if (buffer.size() < total) {
        return false;
    }
    request->body = buffer.mid(headerEnd + 4, contentLength);
    *consumed = total;
    return true;
}

} // namespace Aidc
