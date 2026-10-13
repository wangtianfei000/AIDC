#ifndef SYSMLTEXTPARSER_H
#define SYSMLTEXTPARSER_H

#include <QString>
#include <QStringList>
#include "../../core/infrastructure/Resource.h"
#include "../../core/common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief SysMLTextParser SysML 文本解析器
 * 
 * 功能:
 * - 解析 SysML 文本文件
 * - 将文本转换为模型对象
 * - 提供错误报告
 * 
 * 注意：这个类与 ANTLR4 生成的 SysMLParser 不同，它是一个包装类。
 */
class SysMLTextParser {
public:
    explicit SysMLTextParser();
    ~SysMLTextParser();
    
    // 解析文本
    Resource* parseText(const QString& text, const QString& uri = QString());
    Resource* parseFile(const QString& filePath);
    
    // 错误处理
    bool hasErrors() const { return !m_errors.isEmpty(); }
    QStringList errors() const { return m_errors; }
    void clearErrors() { m_errors.clear(); }
    
    // 错误统计
    int errorCount() const { return m_errors.size(); }
    int warningCount() const { return m_warnings.size(); }
    QStringList warnings() const { return m_warnings; }
    bool hasWarnings() const { return !m_warnings.isEmpty(); }
    
private:
    void reportError(const QString& message, int line = -1, int column = -1);
    void reportWarning(const QString& message, int line = -1, int column = -1);
    QString formatError(const QString& message, int line, int column, const QString& severity = "ERROR");
    
    QStringList m_errors;
    QStringList m_warnings;
};

SYSML_NAMESPACE_END

#endif // SYSMLTEXTPARSER_H

