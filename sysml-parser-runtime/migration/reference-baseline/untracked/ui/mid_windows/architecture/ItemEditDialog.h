#ifndef ITEMEDITDIALOG_H
#define ITEMEDITDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include "../../../core/common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief ItemEditDialog Item编辑对话框
 * 
 * 用于添加或编辑item
 */
class ItemEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit ItemEditDialog(QWidget* parent = nullptr);
    
    // 设置初始值（用于编辑）
    void setName(const QString& name);
    void setType(const QString& type);
    
    // 获取输入值
    QString name() const;
    QString type() const;
    
    // 设置现有item名称列表（用于验证重复）
    void setExistingNames(const QStringList& names);

private slots:
    // 校验输入是否合法（不关闭对话框）
    bool validateInput();
    // OK 按钮点击时调用：先校验再决定是否 accept
    void onOkClicked();

private:
    QLineEdit* m_nameEdit;
    QLineEdit* m_typeEdit;
    QStringList m_existingNames;
};

SYSML_NAMESPACE_END

#endif // ITEMEDITDIALOG_H
