#pragma once

#include <QJsonObject>
#include <atomic>

namespace Aidc {

struct AnalysisOutcome {
    QJsonObject snapshot;
    bool canceled = false;
};

class AnalysisEngine {
public:
    static constexpr const char* EngineVersion = "0.1.0";
    AnalysisOutcome analyze(const QJsonObject& modelSnapshot,
                            const std::atomic_bool* canceled = nullptr) const;
};

} // namespace Aidc
