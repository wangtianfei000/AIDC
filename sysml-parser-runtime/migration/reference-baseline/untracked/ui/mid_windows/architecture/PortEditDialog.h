#ifndef PORTEDITDIALOG_H
#define PORTEDITDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include "../../../core/common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief PortEditDialog 端口编辑对话框
 * 
 * 用于添加或编辑端口
 */
class PortEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit PortEditDialog(QWidget* parent = nullptr);
    
    // 设置初始值（用于编辑）
    void setName(const QString& name);
    void setPosition(const QString& position);
    void setType(const QString& type);
    
    // 获取输入值
    QString name() const;
    QString position() const;
    QString type() const;
    
    // 设置现有端口名称列表（用于验证重复）
    void setExistingNames(const QStringList& names);
    
    // 设置可用的端口类型列表（接口类型）
    void setAvailableTypes(const QStringList& types);

private slots:
    // 校验输入是否合法（不关闭对话框）
    bool validateInput();
    // OK 按钮点击时调用：先校验再决定是否 accept
    void onOkClicked();

private:
    QLineEdit* m_nameEdit;
    QComboBox* m_positionCombo;
    QComboBox* m_typeCombo;  // 改为下拉列表
    QStringList m_existingNames;
};

SYSML_NAMESPACE_END

#endif // PORTEDITDIALOG_H
