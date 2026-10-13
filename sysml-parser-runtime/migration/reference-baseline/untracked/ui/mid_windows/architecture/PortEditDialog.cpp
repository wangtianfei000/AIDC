#include "PortEditDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>

SYSML_NAMESPACE_BEGIN

PortEditDialog::PortEditDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("添加端口");
    setModal(true);
    resize(400, 150);
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    // 名称
    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel("名称:", this));
    m_nameEdit = new QLineEdit(this);
    nameLayout->addWidget(m_nameEdit);
    layout->addLayout(nameLayout);
    
    // 位置
    QHBoxLayout* positionLayout = new QHBoxLayout();
    positionLayout->addWidget(new QLabel("位置:", this));
    m_positionCombo = new QComboBox(this);
    m_positionCombo->addItems({"left", "right", "top", "bottom"});
    m_positionCombo->setCurrentText("left");
    positionLayout->addWidget(m_positionCombo);
    layout->addLayout(positionLayout);
    
    // 类型（下拉列表）
    QHBoxLayout* typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("类型:", this));
    m_typeCombo = new QComboBox(this);
    m_typeCombo->setEditable(true);  // 允许手动输入类型名称
    typeLayout->addWidget(m_typeCombo);
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
    connect(okButton, &QPushButton::clicked, this, &PortEditDialog::onOkClicked);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    // 连接信号以实时验证（仅用于提示，不自动关闭对话框）
    connect(m_nameEdit, &QLineEdit::textChanged, this, [this](const QString&) {
        validateInput();
    });

    // 明确将初始焦点放在名称输入框
    m_nameEdit->setFocus();
}

void PortEditDialog::setName(const QString& name) {
    m_nameEdit->setText(name);
}

void PortEditDialog::setPosition(const QString& position) {
    int index = m_positionCombo->findText(position);
    if (index >= 0) {
        m_positionCombo->setCurrentIndex(index);
    }
}

void PortEditDialog::setType(const QString& type) {
    int index = m_typeCombo->findText(type);
    if (index >= 0) {
        m_typeCombo->setCurrentIndex(index);
    } else {
        m_typeCombo->setCurrentText(type);
    }
}

QString PortEditDialog::name() const {
    return m_nameEdit->text().trimmed();
}

QString PortEditDialog::position() const {
    return m_positionCombo->currentText();
}

QString PortEditDialog::type() const {
    return m_typeCombo->currentText().trimmed();
}

void PortEditDialog::setAvailableTypes(const QStringList& types) {
    m_typeCombo->clear();
    m_typeCombo->addItems(types);
}

void PortEditDialog::setExistingNames(const QStringList& names) {
    m_existingNames = names;
}

bool PortEditDialog::validateInput() {
    QString nameValue = name();
    
    // 验证名称不能为空
    if (nameValue.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "端口名称不能为空！");
        return false;
    }
    
    // 验证名称不能重复
    if (m_existingNames.contains(nameValue)) {
        QMessageBox::warning(this, "输入错误", QString("端口名称 \"%1\" 已存在！").arg(nameValue));
        return false;
    }
    
    return true;
}

void PortEditDialog::onOkClicked()
{
    if (validateInput()) {
        accept();
    }
}

SYSML_NAMESPACE_END
