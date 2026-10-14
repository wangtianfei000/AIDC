#include "SysMLKernelService.h"

#include "analysis/AnalysisService.h"

namespace Aidc {

QJsonObject SysMLKernelService::healthResponse()
{
    return ModelSnapshotAdapter::healthResponse();
}

QJsonObject SysMLKernelService::capabilitiesResponse()
{
    return ModelSnapshotAdapter::capabilitiesResponse();
}

ParseOutcome SysMLKernelService::parse(const QString& uri,
                                       const QString& text,
                                       const std::atomic_bool* canceled) const
{
    return ModelSnapshotAdapter().parse(uri, text, canceled);
}

KernelAnalysisOutcome SysMLKernelService::analyze(const QString& uri,
                                                  const QString& text,
                                                  const std::atomic_bool* canceled) const
{
    const ParseOutcome parsed = parse(uri, text, canceled);
    if (parsed.canceled) return {{}, true};

    const AnalysisOutcome analyzed = AnalysisService().analyze(parsed.snapshot, canceled);
    QJsonObject snapshot = analyzed.snapshot;
    snapshot.insert("parseDiagnostics", parsed.snapshot.value("diagnostics"));
    return {snapshot, analyzed.canceled};
}

} // namespace Aidc
