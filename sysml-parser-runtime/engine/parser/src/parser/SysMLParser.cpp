// 先包含我们自己的解析器头文件（定义了 SysMLTextParser 类）
// 使用相对路径明确指定，避免与生成的代码冲突
#include "parser/SysMLParser.h"

// 然后包含 ANTLR4 生成的代码
#ifndef ANTLR4CPP_STATIC
#define ANTLR4CPP_STATIC
#endif
#include "Token.h"
#include <antlr4-runtime.h>

// 生成的解析器代码（直接包含，因为 generated 目录已在包含路径中）
// 注意：这里包含的是 ANTLR4 生成的 SysMLParser.h，定义了 SysML::SysMLParser 类
// 由于 generated/ 目录在包含路径中，直接使用文件名即可
#include "SysMLLexer.h"
#include "SysMLParser.h"  // ANTLR4 生成的解析器（在 generated/ 目录下）

#include "../visitor/ModelBuilderVisitor.h"
#include "../../core/infrastructure/Resource.h"
#include "../../core/infrastructure/ResourceFactory.h"
#include <QFile>
#include <QDebug>

SYSML_NAMESPACE_BEGIN

// 错误监听器类
class ParserErrorListener : public antlr4::BaseErrorListener {
public:
    ParserErrorListener(QStringList& errors, QStringList& warnings) 
        : m_errors(errors), m_warnings(warnings) {}
    
    void syntaxError(antlr4::Recognizer* /*recognizer*/, antlr4::Token* offendingSymbol,
                     size_t line, size_t charPositionInLine, const std::string& msg,
                     std::exception_ptr /*e*/) override {
        QString errorMsg = QString("[SYNTAX_ERROR] Line %1, Column %2: %3")
            .arg(line)
            .arg(charPositionInLine)
            .arg(QString::fromStdString(msg));
        
        // 如果有关联的符号，添加更多信息
        if (offendingSymbol) {
            QString symbolText = QString::fromStdString(offendingSymbol->getText());
            if (!symbolText.isEmpty() && symbolText != "<EOF>") {
                errorMsg += QString(" (near '%1')").arg(symbolText);
            }
        }
        
        m_errors.append(errorMsg);
        qWarning() << "[SysMLParser]" << errorMsg;
    }
    
private:
    QStringList& m_errors;
    QStringList& m_warnings;
};

SysMLTextParser::SysMLTextParser()
{
}

SysMLTextParser::~SysMLTextParser()
{
}

Resource* SysMLTextParser::parseText(const QString& text, const QString& uri)
{
    clearErrors();
    
    if (text.isEmpty()) {
        reportError("Input text is empty", -1, -1);
        return nullptr;
    }
    
    // 检查文本长度（防止过大的输入）
    const int MAX_TEXT_LENGTH = 10 * 1024 * 1024; // 10MB
    if (text.length() > MAX_TEXT_LENGTH) {
        reportError(QString("Input text is too large (%1 characters, maximum %2)")
                    .arg(text.length()).arg(MAX_TEXT_LENGTH), -1, -1);
        return nullptr;
    }
    
    // 创建资源
    Resource* resource = ResourceFactory::createResource(uri.isEmpty() ? "memory://" : uri);
    if (!resource) {
        reportError("Failed to create resource", -1, -1);
        return nullptr;
    }
    
    try {
        // 1. 创建输入流（将 QString 转换为 std::string）
        std::string inputText = text.toStdString();
        antlr4::ANTLRInputStream input(inputText);
        
        // 2. 创建词法分析器（使用生成的词法分析器）
        SysML::SysMLLexer lexer(&input);
        antlr4::CommonTokenStream tokens(&lexer);
        
        // 3. 创建解析器（使用 ANTLR4 生成的 SysMLParser）
        SysML::SysMLParser antlrParser(&tokens);
        
        // 4. 添加错误监听器
        ParserErrorListener errorListener(m_errors, m_warnings);
        antlrParser.removeErrorListeners();
        antlrParser.addErrorListener(&errorListener);
        
        // 5. 解析文件（从 file 规则开始）
        auto tree = antlrParser.file();
        
        // 检查是否有语法错误
        int syntaxErrorCount = antlrParser.getNumberOfSyntaxErrors();
        if (syntaxErrorCount > 0) {
            qWarning() << "[SysMLParser] Syntax errors detected:" << syntaxErrorCount;
            // 错误已经通过监听器添加到 m_errors
            
            // 如果语法错误太多，可能无法继续构建模型
            if (syntaxErrorCount > 100) {
                reportWarning("Too many syntax errors, model building may be incomplete", -1, -1);
            }
        }
        
        // 6. 创建访问者并遍历 AST
        ModelBuilderVisitor visitor(resource);
        visitor.visitFile(tree);
        
        // 检查访问者是否有错误
        if (visitor.hasErrors()) {
            m_errors.append(visitor.errors());
            qWarning() << "[SysMLParser] Visitor errors detected:" << visitor.errors().size();
        }
        
        // 检查访问者是否有警告
        if (visitor.hasWarnings()) {
            m_warnings.append(visitor.warnings());
            qDebug() << "[SysMLParser] Visitor warnings detected:" << visitor.warnings().size();
        }
        
        // 输出解析摘要
        qDebug() << "[SysMLParser] Parsing completed. "
                 << "Errors:" << m_errors.size() 
                 << ", Warnings:" << m_warnings.size();
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("Exception during parsing: %1").arg(e.what());
        reportError(errorMsg, -1, -1);
        qCritical() << "[SysMLParser]" << errorMsg;
        
        // 清理资源（如果创建了但未完成）
        if (resource) {
            // 注意：这里不删除 resource，因为它可能已经被返回或使用
            // 调用者负责清理
        }
    } catch (...) {
        reportError("Unknown exception during parsing", -1, -1);
        qCritical() << "[SysMLParser] Unknown exception during parsing";
    }
    
    return resource;
}

Resource* SysMLTextParser::parseFile(const QString& filePath)
{
    clearErrors();
    
    // 检查文件路径
    if (filePath.isEmpty()) {
        reportError("File path is empty", -1, -1);
        return nullptr;
    }
    
    QFile file(filePath);
    if (!file.exists()) {
        reportError(QString("File does not exist: %1").arg(filePath), -1, -1);
        return nullptr;
    }
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        reportError(QString("Failed to open file: %1 (%2)").arg(filePath).arg(file.errorString()), -1, -1);
        return nullptr;
    }
    
    // 检查文件大小
    qint64 fileSize = file.size();
    const qint64 MAX_FILE_SIZE = 10 * 1024 * 1024; // 10MB
    if (fileSize > MAX_FILE_SIZE) {
        reportError(QString("File is too large: %1 bytes (maximum %2 bytes)").arg(fileSize).arg(MAX_FILE_SIZE), -1, -1);
        file.close();
        return nullptr;
    }
    
    QString text = QString::fromUtf8(file.readAll());
    file.close();
    
    if (text.isEmpty()) {
        reportWarning(QString("File is empty: %1").arg(filePath), -1, -1);
    }
    
    Resource* resource = parseText(text, filePath);
    if (resource) {
        resource->setFilePath(filePath);
    }
    
    return resource;
}

void SysMLTextParser::reportError(const QString& message, int line, int column)
{
    QString errorMsg = formatError(message, line, column, "ERROR");
    m_errors.append(errorMsg);
    qWarning() << "[SysMLParser]" << errorMsg;
}

void SysMLTextParser::reportWarning(const QString& message, int line, int column)
{
    QString warningMsg = formatError(message, line, column, "WARNING");
    m_warnings.append(warningMsg);
    qDebug() << "[SysMLParser]" << warningMsg;
}

QString SysMLTextParser::formatError(const QString& message, int line, int column, const QString& severity)
{
    QString formatted = QString("[%1]").arg(severity);
    
    if (line >= 0) {
        formatted += QString(" Line %1").arg(line);
        if (column >= 0) {
            formatted += QString(", Column %1").arg(column);
        }
        formatted += ":";
    }
    
    formatted += " " + message;
    return formatted;
}

SYSML_NAMESPACE_END

