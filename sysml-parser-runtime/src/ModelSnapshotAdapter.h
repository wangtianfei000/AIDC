#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <atomic>

namespace Aidc {

struct ParseOutcome {
    QJsonObject snapshot;
    bool canceled = false;
};

class ModelSnapshotAdapter {
public:
    static constexpr const char* SchemaVersion = "0.1";
    static constexpr const char* RuntimeVersion = "0.9.0";
    static constexpr const char* ParserBaseline = "aidc-standalone-parser-0.1";

    ParseOutcome parse(const QString& uri,
                       const QString& text,
                       const std::atomic_bool* canceled = nullptr) const;

    static QJsonObject capabilitiesResponse();
    static QJsonObject healthResponse();
};

} // namespace Aidc
