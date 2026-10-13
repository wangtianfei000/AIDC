#pragma once

#include "StandaloneModel.h"
#include <QString>
#include <QStringList>

class SysMLTextParser {
public:
    Resource* parseText(const QString& text, const QString& uri);
    QStringList errors() const { return m_errors; }
    QStringList warnings() const { return m_warnings; }
private:
    QStringList m_errors;
    QStringList m_warnings;
};
