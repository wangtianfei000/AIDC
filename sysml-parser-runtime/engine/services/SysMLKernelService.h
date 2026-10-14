#pragma once

#include "ModelSnapshotAdapter.h"

namespace Aidc {

struct KernelAnalysisOutcome {
    QJsonObject snapshot;
    bool canceled = false;
};

class SysMLKernelService {
public:
    static constexpr const char* RuntimeVersion = ModelSnapshotAdapter::RuntimeVersion;

    static QJsonObject healthResponse();
    static QJsonObject capabilitiesResponse();

    ParseOutcome parse(const QString& uri,
                       const QString& text,
                       const std::atomic_bool* canceled = nullptr) const;

    KernelAnalysisOutcome analyze(const QString& uri,
                                  const QString& text,
                                  const std::atomic_bool* canceled = nullptr) const;
};

} // namespace Aidc
