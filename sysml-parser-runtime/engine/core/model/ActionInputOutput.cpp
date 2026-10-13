#include "ActionInputOutput.h"

SYSML_NAMESPACE_BEGIN

ActionInputOutput::ActionInputOutput(QObject* parent)
    : Element(parent)
{
}

void ActionInputOutput::setIsInput(bool isInput)
{
    if (m_isInput != isInput) {
        m_isInput = isInput;
        emit isInputChanged(isInput);
    }
}

void ActionInputOutput::setHasItemKeyword(bool hasItemKeyword)
{
    if (m_hasItemKeyword != hasItemKeyword) {
        m_hasItemKeyword = hasItemKeyword;
        emit hasItemKeywordChanged(hasItemKeyword);
    }
}

void ActionInputOutput::setItemName(const QString& itemName)
{
    if (m_itemName != itemName) {
        m_itemName = itemName;
        emit itemNameChanged(itemName);
    }
}

void ActionInputOutput::setItemType(const QString& itemType)
{
    if (m_itemType != itemType) {
        m_itemType = itemType;
        emit itemTypeChanged(itemType);
    }
}

void ActionInputOutput::setDefaultValue(const QString& defaultValue)
{
    if (m_defaultValue != defaultValue) {
        m_defaultValue = defaultValue;
        emit defaultValueChanged(defaultValue);
    }
}

void ActionInputOutput::setRedefinesName(const QString& redefinesName)
{
    if (m_redefinesName != redefinesName) {
        m_redefinesName = redefinesName;
        emit redefinesNameChanged(redefinesName);
    }
}

void ActionInputOutput::setMultiplicity(const QString& multiplicity)
{
    if (m_multiplicity != multiplicity) {
        m_multiplicity = multiplicity;
        emit multiplicityChanged(multiplicity);
    }
}

SYSML_NAMESPACE_END

