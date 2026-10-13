#include "ItemEditDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>

SYSML_NAMESPACE_BEGIN

ItemEditDialog::ItemEditDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("添加Item");
    setModal(true);
    resize(400, 120);
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    // 名称
    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel("名称:", this));
    m_nameEdit = new QLineEdit(this);
    nameLayout->addWidget(m_nameEdit);
    layout->addLayout(nameLayout);
    
    // 类型
    QHBoxLayout* typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("类型:", this));
    m_typeEdit = new QLineEdit(this);
    typeLayout->addWidget(m_typeEdit);
    layout->addLayout(typeLayout);
    
    // 底部按钮行：自定义 OK / Cancel，不使用 QDialogButtonBox
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    QPushButton* okButton = new QPushButton(tr("OK"), this);
    QPushButton* cancelButton = new QPushButton(tr("Cancel"), this);
    okButton->setAutoDefault(false);
    okButton->setDefault(false);
    cancelButton->setAutoDefault(false);
    cancelButton->setDefault(false);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    // OK 按钮单独的 slot：内部调用 validateInput，再根据结果决定是否 accept
    connect(okButton, &QPushButton::clicked, this, &ItemEditDialog::onOkClicked);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    // 连接信号以实时验证（仅用于提示，不自动关闭对话框）
    connect(m_nameEdit, &QLineEdit::textChanged, this, [this](const QString&) {
        validateInput();
    });

    // 明确将初始焦点放在名称输入框
    m_nameEdit->setFocus();
}

void ItemEditDialog::setName(const QString& name) {
    m_nameEdit->setText(name);
}

void ItemEditDialog::setType(const QString& type) {
    m_typeEdit->setText(type);
}

QString ItemEditDialog::name() const {
    return m_nameEdit->text().trimmed();
}

QString ItemEditDialog::type() const {
    return m_typeEdit->text().trimmed();
}

void ItemEditDialog::setExistingNames(const QStringList& names) {
    m_existingNames = names;
}

bool ItemEditDialog::validateInput() {
    QString nameValue = name();
    
    // 验证名称不能为空
    if (nameValue.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "Item名称不能为空！");
        return false;
    }
    
    // 验证名称不能重复
    if (m_existingNames.contains(nameValue)) {
        QMessageBox::warning(this, "输入错误", QString("Item名称 \"%1\" 已存在！").arg(nameValue));
        return false;
    }
    
    return true;
}

void ItemEditDialog::onOkClicked()
{
    if (validateInput()) {
        accept();
    }
}

SYSML_NAMESPACE_END
