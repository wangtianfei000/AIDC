#ifndef SYSML_UTILS_H
#define SYSML_UTILS_H

#include "Global.h"
#include <QString>
#include <QStringList>

SYSML_NAMESPACE_BEGIN

/**
 * @brief 工具函数集合
 */
class Utils {
public:
    // 字符串工具
    static QStringList splitQualifiedName(const QString& qualifiedName);
    static QString joinQualifiedName(const QStringList& parts);
    
    // 验证工具
    static bool isValidIdentifier(const QString& name);
    static bool isValidQualifiedName(const QString& qualifiedName);
    
    // 格式化工具
    static QString indent(int level, const QString& text = QString());
};

SYSML_NAMESPACE_END

#endif // SYSML_UTILS_H



