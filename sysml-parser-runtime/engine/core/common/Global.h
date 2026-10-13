#ifndef SYSML_GLOBAL_H
#define SYSML_GLOBAL_H

#include <QtCore/QtGlobal>
#include <QString>

// 命名空间
#define SYSML_NAMESPACE_BEGIN namespace SysML {
#define SYSML_NAMESPACE_END }

// 版本信息
#define SYSML_VERSION_MAJOR 0
#define SYSML_VERSION_MINOR 1
#define SYSML_VERSION_PATCH 0
#define SYSML_VERSION_STRING "0.1.0"

// 导出宏
#ifdef SYSML_BUILD_LIBRARY
#  ifdef SYSML_LIBRARY_EXPORT
#    define SYSML_EXPORT Q_DECL_EXPORT
#  else
#    define SYSML_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define SYSML_EXPORT
#endif

SYSML_NAMESPACE_BEGIN

// 版本结构
struct Version {
    int major = SYSML_VERSION_MAJOR;
    int minor = SYSML_VERSION_MINOR;
    int patch = SYSML_VERSION_PATCH;
    
    QString toString() const {
        return QString("%1.%2.%3").arg(major).arg(minor).arg(patch);
    }
    
    static Version current() {
        return {SYSML_VERSION_MAJOR, SYSML_VERSION_MINOR, SYSML_VERSION_PATCH};
    }
};

SYSML_NAMESPACE_END

#endif // SYSML_GLOBAL_H

