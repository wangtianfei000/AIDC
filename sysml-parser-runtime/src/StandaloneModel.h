#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>

class Relationship;

class Element : public QObject {
public:
    explicit Element(QString kind, QString name = {}, QObject* parent = nullptr)
        : QObject(parent), m_kind(std::move(kind)), m_name(std::move(name)) {}
    virtual ~Element() = default;

    QString elementType() const { return m_kind; }
    QString declaredName() const { return m_name; }
    QString qualifiedName() const { return m_qualifiedName; }
    QList<Element*> ownedElements() const { return m_elements; }
    QList<Element*> ownedAnnotations() const { return {}; }
    QList<Relationship*> ownedRelationships() const { return m_relationships; }
    void setQualifiedName(const QString& value) { m_qualifiedName = value; }
    void addElement(Element* value) { if (value) { value->setParent(this); m_elements.append(value); } }
    void addRelationship(Relationship* value);

private:
    QString m_kind;
    QString m_name;
    QString m_qualifiedName;
    QList<Element*> m_elements;
    QList<Relationship*> m_relationships;
};

class Relationship : public Element {
public:
    explicit Relationship(QString kind, QString name = {}) : Element(std::move(kind), std::move(name)) {}
};

inline void Element::addRelationship(Relationship* value) {
    if (value) { value->setParent(this); m_relationships.append(value); }
}

class Feature : public Element {
public:
    explicit Feature(QString kind, QString name = {}) : Element(std::move(kind), std::move(name)) {}
    bool isAbstract() const { return false; }
    bool isConst() const { return false; }
    bool isDerived() const { return false; }
    int direction() const { return 0; }
    bool isRef() const { return false; }
    QString unitExpression() const { return m_unit; }
    QString valueExpression() const { return m_value; }
    QString defaultValue() const { return m_value; }
    bool isSimplifiedAttribute() const { return true; }
    bool isRedefines() const { return false; }
    void setUnitExpression(const QString& value) { m_unit = value; }
    void setValueExpression(const QString& value) { m_value = value; }
private:
    QString m_unit;
    QString m_value;
};

class Type : public Element { public: using Element::Element; };
class Namespace : public Element { public: using Element::Element; };

class TypedUsage : public Element {
public:
    TypedUsage(QString kind, QString name, QString typeName = {})
        : Element(std::move(kind), std::move(name)), m_typeName(std::move(typeName)) {}
    QString typeName() const { return m_typeName; }
    void setTypeName(const QString& value) { m_typeName = value; }
private:
    QString m_typeName;
};
class RequirementUsage : public TypedUsage { public: RequirementUsage(QString n, QString t = {}) : TypedUsage("RequirementUsage", std::move(n), std::move(t)) {} };
class PartUsage : public TypedUsage { public: PartUsage(QString n, QString t = {}) : TypedUsage("PartUsage", std::move(n), std::move(t)) {} };
class PortUsage : public TypedUsage { public: PortUsage(QString n, QString t = {}) : TypedUsage("PortUsage", std::move(n), std::move(t)) {} };

class FeatureTyping : public Relationship {
public:
    FeatureTyping(Element* feature, QString typeName)
        : Relationship("FeatureTyping"), m_feature(feature), m_typeName(std::move(typeName)) {}
    QString typeName() const { return m_typeName; }
    Element* typedFeature() const { return m_feature; }
    Type* type() const { return nullptr; }
private:
    Element* m_feature;
    QString m_typeName;
};

class Redefinition : public Relationship {
public:
    using Relationship::Relationship;
    QString redefinedFeatureName() const { return {}; }
    Element* redefiningFeature() const { return nullptr; }
    Element* redefinedFeature() const { return nullptr; }
};
class Subsetting : public Relationship {
public:
    using Relationship::Relationship;
    QString subsettedFeatureName() const { return {}; }
    Element* subsettingFeature() const { return nullptr; }
    Element* subsettedFeature() const { return nullptr; }
};
class Subclassification : public Relationship {
public:
    using Relationship::Relationship;
    Type* subtype() const { return nullptr; }
    Type* supertype() const { return nullptr; }
};
class Conjugation : public Relationship {
public:
    using Relationship::Relationship;
    QString originalTypeName() const { return {}; }
    Type* conjugatedType() const { return nullptr; }
    Type* originalType() const { return nullptr; }
};
class Dependency : public Relationship {
public:
    using Relationship::Relationship;
    QString supplierElementName() const { return {}; }
    Element* clientElement() const { return nullptr; }
    Element* supplierElement() const { return nullptr; }
};
class Membership : public Relationship { public: using Relationship::Relationship; Element* memberElement() const { return nullptr; } };
class Import : public Relationship { public: using Relationship::Relationship; Namespace* importedNamespace() const { return nullptr; } };
class MembershipImport : public Import { public: using Import::Import; Membership* importedMembership() const { return nullptr; } };
class NamespaceImport : public Import {
public:
    explicit NamespaceImport(QString name) : Import("NamespaceImport"), m_name(std::move(name)) {}
    QString importedNamespaceName() const { return m_name; }
private: QString m_name;
};

class ConnectStatement : public Element {
public:
    explicit ConnectStatement(QStringList ends) : Element("ConnectStatement", "connect"), m_ends(std::move(ends)) {}
    QStringList ends() const { return m_ends; }
private: QStringList m_ends;
};

class ExpressionStatement : public Element {
public:
    ExpressionStatement(QString kind, QString name, QString expression)
        : Element(std::move(kind), std::move(name)), m_expression(std::move(expression)) {}
    QString expression() const { return m_expression; }
private: QString m_expression;
};
class RequireStatement : public ExpressionStatement {
public:
    RequireStatement(QString name, QString expression) : ExpressionStatement("RequireStatement", std::move(name), std::move(expression)) {}
    bool hasConstraintKeyword() const { return true; }
};
class AssertStatement : public ExpressionStatement { public: AssertStatement(QString n, QString e) : ExpressionStatement("AssertStatement", std::move(n), std::move(e)) {} };
class AssumeStatement : public ExpressionStatement { public: AssumeStatement(QString n, QString e) : ExpressionStatement("AssumeStatement", std::move(n), std::move(e)) {} };
class ConstraintStatement : public ExpressionStatement { public: ConstraintStatement(QString n, QString e) : ExpressionStatement("ConstraintStatement", std::move(n), std::move(e)) {} };

class CalculationDefinition : public Element { public: explicit CalculationDefinition(QString n) : Element("CalculationDefinition", std::move(n)) {} };
class AnalysisCase : public Element {
public:
    explicit AnalysisCase(QString n) : Element("AnalysisCase", std::move(n)) {}
    QString subjectReference() const { return m_subject; }
    void setSubjectReference(const QString& value) { m_subject = value; }
private: QString m_subject;
};
class AnalysisSubject : public Element { public: using Element::Element; };

class DocStatement : public Element {
public:
    explicit DocStatement(QString content)
        : Element("DocStatement", "doc"), m_content(std::move(content)) {}
    QString content() const { return m_content; }
private:
    QString m_content;
};

class ActionInputOutput : public Element {
public:
    ActionInputOutput(bool input, QString name, QString type, QString value)
        : Element("ActionInputOutput", name), m_input(input), m_name(std::move(name)), m_type(std::move(type)), m_value(std::move(value)) {}
    bool isInput() const { return m_input; }
    QString itemName() const { return m_name; }
    QString itemType() const { return m_type; }
    QString defaultValue() const { return m_value; }
private: bool m_input; QString m_name, m_type, m_value;
};
class ReturnParameter : public Element {
public:
    ReturnParameter(QString name, QString type, QString expression)
        : Element("ReturnParameter", std::move(name)), m_type(std::move(type)), m_expression(std::move(expression)) {}
    QString typeName() const { return m_type; }
    QString expression() const { return m_expression; }
private: QString m_type, m_expression;
};

class RenderingUsage : public Element {
public:
    RenderingUsage(QString name, QString type) : Element("RenderingUsage", std::move(name)), m_type(std::move(type)) {}
    QString renderingType() const { return m_type; }
private: QString m_type;
};
class ViewDefinition : public Element {
public:
    ViewDefinition(QString name, QString type) : Element("ViewDefinition", std::move(name)), m_type(std::move(type)) {}
    QString viewType() const { return m_type; }
    QString filterExpression() const { return {}; }
private: QString m_type;
};
class ViewUsage : public TypedUsage {
public:
    ViewUsage(QString name, QString type) : TypedUsage("ViewUsage", std::move(name), std::move(type)) {}
    QString renderMethod() const { return m_render; }
    QString filterExpression() const { return {}; }
    void setRenderMethod(const QString& value) { m_render = value; }
private: QString m_render;
};
class ExposeStatement : public Element {
public:
    explicit ExposeStatement(QString name) : Element("ExposeStatement", "expose"), m_name(std::move(name)) {}
    QString exposedQualifiedName() const { return m_name; }
    bool isRecursive() const { return false; }
    int exposeType() const { return 0; }
private: QString m_name;
};
class ViewRenderingMember : public Element { public: explicit ViewRenderingMember(QString n) : Element("ViewRenderingMember", std::move(n)) {} };

class Resource {
public:
    ~Resource() { qDeleteAll(m_contents); }
    QList<Element*> contents() const { return m_contents; }
    void add(Element* value) { if (value) m_contents.append(value); }
private: QList<Element*> m_contents;
};
