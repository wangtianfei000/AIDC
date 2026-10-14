#include "UnitSystem.h"

#include <QRegularExpression>

#include <cmath>

namespace Aidc::Semantic {
namespace {

Dimensions dims(std::initializer_list<std::pair<QString, int>> values)
{
    Dimensions result;
    for (const auto& [name, exponent] : values) {
        if (exponent) result.insert(name, exponent);
    }
    return result;
}

UnitDefinition raised(UnitDefinition value, int exponent)
{
    value.factor = std::pow(value.factor, exponent);
    value.offset = 0.0;
    for (auto it = value.dimensions.begin(); it != value.dimensions.end(); ++it) {
        it.value() *= exponent;
    }
    return value;
}

} // namespace

const UnitSystem& UnitSystem::standard()
{
    static const UnitSystem value;
    return value;
}

UnitSystem::UnitSystem()
{
    auto add = [this](const QString& symbol, double factor, const Dimensions& dimensions) {
        m_units.insert(symbol, UnitDefinition{symbol, factor, 0.0, dimensions});
    };
    add("1", 1.0, {});
    add("g", 0.001, dims({{"M", 1}}));
    add("kg", 1.0, dims({{"M", 1}}));
    add("m", 1.0, dims({{"L", 1}}));
    add("s", 1.0, dims({{"T", 1}}));
    add("ms", 0.001, dims({{"T", 1}}));
    add("min", 60.0, dims({{"T", 1}}));
    add("h", 3600.0, dims({{"T", 1}}));
    add("A", 1.0, dims({{"I", 1}}));
    add("C", 1.0, dims({{"I", 1}, {"T", 1}}));
    add("Ah", 3600.0, dims({{"I", 1}, {"T", 1}}));
    add("V", 1.0, dims({{"M", 1}, {"L", 2}, {"T", -3}, {"I", -1}}));
    add("N", 1.0, dims({{"M", 1}, {"L", 1}, {"T", -2}}));
    add("J", 1.0, dims({{"M", 1}, {"L", 2}, {"T", -2}}));
    add("W", 1.0, dims({{"M", 1}, {"L", 2}, {"T", -3}}));
    add("Wh", 3600.0, dims({{"M", 1}, {"L", 2}, {"T", -2}}));

    // Common standard-library spellings are aliases. Composite spellings are
    // also parsed structurally by resolve(), so this catalog is not a closed
    // list of accepted unit expressions.
    add("kW", 1000.0, m_units.value("W").dimensions);
    add("m/s", 1.0, dims({{"L", 1}, {"T", -1}}));
    add("m/s^2", 1.0, dims({{"L", 1}, {"T", -2}}));
    add("N.m", 1.0, dims({{"M", 1}, {"L", 2}, {"T", -2}}));

    auto type = [this](const QString& name, const QString& unit) {
        m_typeDimensions.insert(name, m_units.value(unit).dimensions);
    };
    type("mass", "kg");
    type("length", "m");
    type("speed", "m/s");
    type("acceleration", "m/s^2");
    type("force", "N");
    type("torque", "N.m");
    type("duration", "s");
    type("time", "s");
    type("electricCurrent", "A");
    type("electricCharge", "C");
    type("electricPotential", "V");
    type("energy", "J");
    type("power", "W");

    m_preferredUnits = {"kg", "m", "m/s", "m/s^2", "s", "A", "C", "V",
                        "N", "N.m", "W", "Wh", "J"};
}

Dimensions UnitSystem::combine(Dimensions left, const Dimensions& right, int sign)
{
    for (auto it = right.begin(); it != right.end(); ++it) {
        const int value = left.value(it.key()) + sign * it.value();
        if (value) left.insert(it.key(), value);
        else left.remove(it.key());
    }
    return left;
}

bool UnitSystem::sameDimensions(const Dimensions& left, const Dimensions& right)
{
    return left == right;
}

bool UnitSystem::resolveFactor(const QString& rawToken, UnitDefinition* definition) const
{
    static const QRegularExpression factorPattern(
        QStringLiteral("^([A-Za-z]+)(?:\\^(-?[0-9]+))?$"));
    const QRegularExpressionMatch match = factorPattern.match(rawToken.trimmed());
    if (!match.hasMatch()) return false;
    const QString symbol = match.captured(1);
    const int exponent = match.captured(2).isEmpty() ? 1 : match.captured(2).toInt();
    if (!m_units.contains(symbol)) return false;
    if (definition) *definition = raised(m_units.value(symbol), exponent);
    return true;
}

bool UnitSystem::resolve(const QString& rawExpression, UnitDefinition* definition) const
{
    QString expression = rawExpression.trimmed();
    if (expression.isEmpty()) expression = "1";
    if (m_units.contains(expression)) {
        if (definition) *definition = m_units.value(expression);
        return true;
    }

    expression.replace(QChar(0x00B7), '*');
    expression.replace('.', '*');
    UnitDefinition result{rawExpression, 1.0, 0.0, {}};
    int operation = 1;
    QString token;
    auto consume = [&](const QString& factorText) -> bool {
        if (factorText.trimmed().isEmpty()) return false;
        UnitDefinition factor;
        if (!resolveFactor(factorText, &factor)) return false;
        result.factor = operation > 0 ? result.factor * factor.factor
                                      : result.factor / factor.factor;
        result.dimensions = combine(result.dimensions, factor.dimensions, operation);
        return true;
    };
    for (const QChar character : expression) {
        if (character == '*' || character == '/') {
            if (!consume(token)) return false;
            token.clear();
            operation = character == '*' ? 1 : -1;
        } else {
            token.append(character);
        }
    }
    if (!consume(token)) return false;
    if (definition) *definition = result;
    return true;
}

Dimensions UnitSystem::dimensionsForType(const QString& typeName) const
{
    return m_typeDimensions.value(typeName.section("::", -1));
}

QString UnitSystem::preferredUnit(const Dimensions& dimensions) const
{
    for (const QString& symbol : m_preferredUnits) {
        if (m_units.value(symbol).dimensions == dimensions) return symbol;
    }
    return {};
}

QString UnitSystem::dimensionsText(const Dimensions& dimensions) const
{
    if (dimensions.isEmpty()) return "1";
    QStringList keys = dimensions.keys();
    keys.sort();
    QStringList parts;
    for (const QString& key : keys) {
        parts.append(QStringLiteral("%1^%2").arg(key).arg(dimensions.value(key)));
    }
    return parts.join(' ');
}

QStringList UnitSystem::symbols() const
{
    QStringList result = m_units.keys();
    result.sort();
    return result;
}

} // namespace Aidc::Semantic
