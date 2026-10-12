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
    static constexpr const char* RuntimeVersion = "0.8.1";
    static constexpr const char* ParserBaseline =
        "9bfba0bced98b7586bab8a6d45746064aff70fe1+c2-64a64a94ea26a507";

    ParseOutcome parse(const QString& uri,
                       const QString& text,
                       const std::atomic_bool* canceled = nullptr) const;

    static QJsonObject capabilitiesResponse();
    static QJsonObject healthResponse();
};

} // namespace Aidc
