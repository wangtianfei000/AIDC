#include "Utils.h"
#include <QRegularExpression>

SYSML_NAMESPACE_BEGIN

QStringList Utils::splitQualifiedName(const QString& qualifiedName)
{
    return qualifiedName.split("::", Qt::SkipEmptyParts);
}

QString Utils::joinQualifiedName(const QStringList& parts)
{
    return parts.join("::");
}

bool Utils::isValidIdentifier(const QString& name)
{
    if (name.isEmpty()) {
        return false;
    }
    
    // 标识符规则: 字母、数字、下划线，不能以数字开头
    static QRegularExpression regex("^[a-zA-Z_][a-zA-Z0-9_]*$");
    return regex.match(name).hasMatch();
}

bool Utils::isValidQualifiedName(const QString& qualifiedName)
{
    if (qualifiedName.isEmpty()) {
        return false;
    }
    
    QStringList parts = splitQualifiedName(qualifiedName);
    for (const QString& part : parts) {
        if (!isValidIdentifier(part)) {
            return false;
        }
    }
    
    return true;
}

QString Utils::indent(int level, const QString& text)
{
    QString indentStr = QString("    ").repeated(level);
    if (text.isEmpty()) {
        return indentStr;
    }
    
    QStringList lines = text.split('\n');
    QStringList indentedLines;
    for (const QString& line : lines) {
        indentedLines << indentStr + line;
    }
    return indentedLines.join('\n');
}

SYSML_NAMESPACE_END



