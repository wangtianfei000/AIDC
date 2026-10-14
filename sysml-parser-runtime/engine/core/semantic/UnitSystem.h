#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

namespace Aidc::Semantic {

using Dimensions = QHash<QString, int>;

struct UnitDefinition {
    QString symbol;
    double factor = 1.0;
    double offset = 0.0;
    Dimensions dimensions;
};

class UnitSystem {
public:
    static constexpr const char* CatalogVersion = "sysml.library/2026-08";

    static const UnitSystem& standard();

    bool resolve(const QString& expression, UnitDefinition* definition = nullptr) const;
    Dimensions dimensionsForType(const QString& typeName) const;
    QString preferredUnit(const Dimensions& dimensions) const;
    QString dimensionsText(const Dimensions& dimensions) const;
    QStringList symbols() const;

    static Dimensions combine(Dimensions left, const Dimensions& right, int sign);
    static bool sameDimensions(const Dimensions& left, const Dimensions& right);

private:
    UnitSystem();
    bool resolveFactor(const QString& token, UnitDefinition* definition) const;
    QHash<QString, UnitDefinition> m_units;
    QHash<QString, Dimensions> m_typeDimensions;
    QStringList m_preferredUnits;
};

} // namespace Aidc::Semantic
