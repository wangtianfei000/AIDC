grammar SysML;

// ============================================
// 语法规则
// ============================================

// 文件：包含多个包
file
    : package* EOF
    ;

// 包定义
package
    : 'package' qualifiedName '{' packageBody '}'
    ;

// 包体：可以包含包、定义、使用、导入、关系、依赖
packageBody
    : (package | definition | usage | importStatement | relationship | dependency)*
    ;

// ============================================
// 前缀规则（第一阶段：修复现有语法结构）
// ============================================

// 基础定义前缀
basicDefinitionPrefix
    : 'abstract'
    | 'variation'
    ;

// 定义扩展关键字（元数据前缀，暂时简化）
definitionExtensionKeyword
    : '#' qualifiedName  // 元数据前缀，简化实现
    ;

// 定义前缀（basicDefinitionPrefix 是互斥的，只能选一个）
definitionPrefix
    : basicDefinitionPrefix? definitionExtensionKeyword*
    ;

// 出现定义前缀（用于 part, action, state 等）
occurrenceDefinitionPrefix
    : basicDefinitionPrefix? ('individual' emptyMultiplicityMember)? definitionExtensionKeyword*
    ;

// 空多重性成员（用于 individual 前缀）
emptyMultiplicityMember
    :  // 空规则，表示可选的多重性占位符
    ;

// 特征方向
featureDirection
    : 'in'
    | 'out'
    | 'inout'
    ;

// 引用前缀
refPrefix
    : featureDirection? 'derived'? ('abstract' | 'variation')? 'constant'?
    ;

// 基础使用前缀
basicUsagePrefix
    : refPrefix 'ref'?
    ;

// 端点使用前缀
endUsagePrefix
    : 'end' ownedCrossFeatureMember?
    ;

// 拥有的交叉特征成员（简化实现，暂时为空，后续可扩展）
ownedCrossFeatureMember
    :  // 暂时为空，后续可扩展为 basicUsagePrefix usageDeclaration
    ;

// 使用扩展关键字（元数据前缀，暂时简化）
usageExtensionKeyword
    : '#' qualifiedName  // 元数据前缀，简化实现
    ;

// 未扩展的使用前缀
unextendedUsagePrefix
    : endUsagePrefix
    | basicUsagePrefix
    ;

// 使用前缀
usagePrefix
    : unextendedUsagePrefix usageExtensionKeyword*
    ;

// 出现使用前缀
occurrenceUsagePrefix
    : (endUsagePrefix | basicUsagePrefix 'individual'? portionKind?) usageExtensionKeyword*
    ;

// 部分类型
portionKind
    : 'snapshot'
    | 'timeslice'
    ;

// 标识符（支持 <id> name 形式）
// 注意：'initial' 关键字也可以作为标识符使用（在 state initial; 等上下文中）
identification
    : '<' (identifier | String) '>' identifier?  // 支持 <'1'> 或 <identifier>
    | 'initial'  // 允许 initial 作为标识符（用于状态名称等）
    | identifier
    ;

// 名称（别名，用于兼容）
name
    : identifier
    ;

// 定义声明
definitionDeclaration
    : identification? subclassificationPart?
    ;

// 子类化部分
subclassificationPart
    : (SUBSETS_OP | 'specializes') ownedSubclassification (',' ownedSubclassification)*
    ;

// 拥有的子类化
ownedSubclassification
    : qualifiedName
    ;

// 定义体
definitionBody
    : ';'
    | '{' definitionBodyItem* '}'
    ;

// 定义体项
definitionBodyItem
    : actionInputOutput  // in item / out item 语法（必须在 usageMember 之前，避免与 itemUsage 冲突）
    | attribute          // attribute 必须在 definitionMember 之前，确保 attribute redefines 能正确匹配
    | simplifiedAttribute  // 简化的 attribute 语法（identifier : Type;）
    | definitionMember   // 定义成员（包含 attributeDefinition）- 放在 attribute 之后
    | usageMember
    | importStatement
    | relationship
    | statement
    | docStatement  // doc 语句
    | commentStatement  // comment 语句
    | requirementMember  // requirement 特殊成员（subject, actor, stakeholder）- 只在 requirementBody 中有效
    | requirementConstraintMember  // requirement 约束成员（assume, require）- 只在 requirementBody 中有效
    | metadataAnnotation  // 元数据注解（@StatusInfo { ... }）
    | exhibitStatement  // exhibit state 语句
    | refStatement  // ref 语句（在定义体中，如 part def body）
    ;

// 元数据注解
metadataAnnotation
    : '@' identifier '{' (definitionBodyItem | metadataAssignment)* '}' ';'?
    ;

// 元数据赋值（用于 metadata body 中的简单赋值，如 status = StatusKind::closed;）
metadataAssignment
    : identifier '=' expression ';'
    ;

// 简化的 attribute 语法（允许省略 'attribute' 关键字）
simplifiedAttribute
    : identifier ':' qualifiedName ';'
    ;

// 定义成员（带可见性前缀）
definitionMember
    : visibilityIndicator? definition
    ;

// 使用成员（带可见性前缀）
usageMember
    : visibilityIndicator? usage
    ;

// 可见性指示符
visibilityIndicator
    : 'public'
    | 'private'
    | 'protected'
    ;

// 使用声明
usageDeclaration
    : featureDeclaration
    ;

// 特征声明
featureDeclaration
    : identification? featureSpecializationPart?
    | featureSpecializationPart
    ;

// 特征特殊化部分（简化实现，后续可扩展）
// 包含类型化、子集化、重定义等表达式
featureSpecializationPart
    : typingExpression? (subsettingExpression | redefinitionExpression)?
    | subsettingExpression? typingExpression? redefinitionExpression?
    | redefinitionExpression? (typingExpression | subsettingExpression)?
    | attributeExpressions?
    ;

// 使用完成
usageCompletion
    : valuePart? usageBody
    ;

// 使用体
// 注意：对于 stateUsage，使用 stateBody 而不是 definitionBody
// 这样可以支持状态特定的成员（entry/do/exit, transition 等）
// 注意：stateBody 必须在 definitionBody 之前，以确保优先匹配
usageBody
    : stateBody  // 支持状态体（用于 stateUsage）- 必须在 definitionBody 之前
    | definitionBody
    ;

// 值部分
valuePart
    : featureValue
    ;

// 特征值
featureValue
    : ('=' | ':=') expression
    | 'default' ('=' | ':=')? expression
    ;

// 表达式（简化实现，基础字面量和特征引用）
expression
    : logicalOrExpression
    ;

// 逻辑或表达式
logicalOrExpression
    : logicalAndExpression ('or' logicalAndExpression)*
    ;

// 逻辑与表达式
logicalAndExpression
    : comparisonExpression ('and' comparisonExpression)*
    ;

// 比较表达式
comparisonExpression
    : additiveExpression (comparisonOp additiveExpression)?
    ;

// 比较运算符
comparisonOp
    : '>'
    | '>='
    | '<'
    | '<='
    | '=='
    | '!='
    ;

// 加法表达式
additiveExpression
    : multiplicativeExpression (('+' | '-') multiplicativeExpression)*
    ;

// 乘法表达式
multiplicativeExpression
    : unaryExpression (('*' | '/') unaryExpression)*
    ;

// 一元表达式
unaryExpression
    : ('-' | '+' | 'not')? primaryExpression
    ;

// 主表达式
primaryExpression
    : (actionInvocation | literalExpression | featureReferenceExpression | '(' expression ')' | 'null') ('[' unitExpression ']')?
    ;

// 单位表达式（用于单位，如 [kg], [km / L]）
unitExpression
    : unitTerm (('/' | '*') unitTerm)*
    ;

// 单位项（基本单位或带指数的单位）
unitTerm
    : IDENTIFIER ('^' Integer)?
    ;

// 字面量表达式
literalExpression
    : Integer
    | Real
    | Boolean
    | String
    ;

// 特征引用表达式
featureReferenceExpression
    : qualifiedName
    | memberAccessExpression  // 支持点号访问
    ;

// 实数（简化实现）
Real
    : Integer '.' Integer?
    | Integer ('.' Integer)? ('e' | 'E') ('+' | '-')? Integer
    ;

// 布尔值
Boolean
    : 'true'
    | 'false'
    ;

// 字符串
String
    : '"' (~'"' | '\\"')* '"'
    | '\'' (~'\'' | '\\\'')* '\''
    ;

// 定义类型（统一规则）
definition
    : partDefinition
    | calculationDefinition
    | actionDefinition
    | stateDefinition
    | requirementDefinition
    | constraintDefinition
    | portDefinition
    | connectorDefinition
    | interfaceDefinition
    | datatypeDefinition
    | classDefinition
    | enumDefinition
    | attributeDefinition
    | connectionDefinition
    | allocationDefinition
    | itemDefinition
    | flowDefinition
    | viewpointDefinition
    | viewDefinition
    | concernDefinition
    ;

// 部件定义（支持前缀和子类化）
partDefinition
    : occurrenceDefinitionPrefix 'part' 'def' definitionDeclaration definitionBody
    ;

// 动作定义（支持前缀和子类化）
actionDefinition
    : occurrenceDefinitionPrefix 'action' 'def' definitionDeclaration definitionBody
    ;

// Calculation definition is distinct from ActionDefinition so semantic
// projections do not have to infer calculations from names or source text.
calculationDefinition
    : occurrenceDefinitionPrefix 'calc' 'def' definitionDeclaration calculationBody
    ;

calculationBody
    : ';'
    | '{' calculationBodyItem* '}'
    ;

calculationBodyItem
    : returnParameter
    | definitionBodyItem
    ;

returnParameter
    : 'return' usageDeclaration? valuePart? ';'
    ;

// 状态定义（支持前缀和子类化）
stateDefinition
    : occurrenceDefinitionPrefix 'state' 'def' definitionDeclaration stateDefinitionBody
    ;

// 需求定义（支持前缀和子类化）
requirementDefinition
    : occurrenceDefinitionPrefix 'requirement' 'def' definitionDeclaration definitionBody
    ;

// 约束定义（支持前缀和子类化）
constraintDefinition
    : occurrenceDefinitionPrefix 'constraint' 'def' definitionDeclaration definitionBody
    ;

// 端口定义（支持前缀和子类化）
portDefinition
    : definitionPrefix 'port' 'def' definitionDeclaration definitionBody
    ;

// 连接器定义（支持前缀和子类化）
connectorDefinition
    : definitionPrefix 'connector' 'def' definitionDeclaration definitionBody
    ;

// 接口定义（支持前缀和子类化）
interfaceDefinition
    : occurrenceDefinitionPrefix 'interface' 'def' definitionDeclaration definitionBody
    ;

// 数据类型定义（支持前缀和子类化）
datatypeDefinition
    : definitionPrefix 'datatype' 'def' definitionDeclaration definitionBody
    ;

// 类定义（支持前缀和子类化）
classDefinition
    : definitionPrefix 'class' 'def' definitionDeclaration definitionBody
    ;

// 枚举定义（支持前缀和子类化）
enumDefinition
    : definitionExtensionKeyword* 'enum' 'def' definitionDeclaration enumBody
    ;

// 属性定义（支持前缀和子类化）
attributeDefinition
    : definitionPrefix 'attribute' 'def' definitionDeclaration definitionBody
    ;

// 连接定义（支持前缀和子类化）
connectionDefinition
    : occurrenceDefinitionPrefix 'connection' 'def' definitionDeclaration definitionBody
    ;

// 分配定义（支持前缀和子类化）
allocationDefinition
    : occurrenceDefinitionPrefix 'allocation' 'def' definitionDeclaration definitionBody
    ;

// 项定义（支持前缀和子类化）
itemDefinition
    : occurrenceDefinitionPrefix 'item' 'def' definitionDeclaration definitionBody
    ;

// 视点定义（支持前缀和子类化）
viewpointDefinition
    : occurrenceDefinitionPrefix 'viewpoint' 'def' definitionDeclaration viewpointBody
    ;

// 视图定义（支持前缀和子类化）
viewDefinition
    : occurrenceDefinitionPrefix 'view' 'def' definitionDeclaration viewDefinitionBody
    ;

// 关注点定义（支持前缀和子类化）
concernDefinition
    : occurrenceDefinitionPrefix 'concern' 'def' definitionDeclaration concernBody
    ;

// 部件使用（支持前缀和完整使用结构）
partUsage
    : occurrenceUsagePrefix 'part' usageDeclaration? usageCompletion
    ;

// 动作使用（支持前缀和完整使用结构）
actionUsage
    : occurrenceUsagePrefix 'action' usageDeclaration? usageCompletion
    ;

analysisUsage
    : occurrenceUsagePrefix 'analysis' usageDeclaration? analysisBody
    ;

analysisBody
    : ';'
    | '{' analysisBodyItem* '}'
    ;

analysisBodyItem
    : subjectAssignment
    | definitionBodyItem
    ;

subjectAssignment
    : 'subject' '=' expression ';'
    ;

renderingUsage
    : occurrenceUsagePrefix 'rendering' usageDeclaration? definitionBody
    ;

// 状态使用（支持前缀和完整使用结构）
stateUsage
    : occurrenceUsagePrefix 'state' usageDeclaration? usageCompletion
    ;

// 需求使用（支持前缀和完整使用结构）
requirementUsage
    : occurrenceUsagePrefix 'requirement' usageDeclaration? usageCompletion
    ;

// 约束使用（支持前缀和完整使用结构）
constraintUsage
    : occurrenceUsagePrefix 'constraint' usageDeclaration? usageCompletion
    ;

// 端口使用（支持前缀和完整使用结构）
portUsage
    : occurrenceUsagePrefix 'port' usageDeclaration? usageCompletion
    ;

// 连接器使用（支持前缀和完整使用结构）
connectorUsage
    : usagePrefix 'connector' usageDeclaration? usageCompletion
    ;

// 流使用（支持前缀和完整使用结构）
flowUsage
    : occurrenceUsagePrefix 'flow' usageDeclaration? usageCompletion
    ;

// 连接使用（支持前缀和完整使用结构）
connectionUsage
    : occurrenceUsagePrefix 'connection' usageDeclaration? usageCompletion
    ;

// 分配使用（支持前缀和完整使用结构）
allocationUsage
    : occurrenceUsagePrefix 'allocation' usageDeclaration? usageCompletion
    ;

// 项使用（支持前缀和完整使用结构）
itemUsage
    : occurrenceUsagePrefix 'item' usageDeclaration? usageCompletion
    ;

// 视图使用（支持前缀和完整使用结构）
viewUsage
    : occurrenceUsagePrefix 'view' usageDeclaration? valuePart? viewBody
    ;

// 部件体：可以包含定义、使用、属性（兼容旧语法，新语法使用 definitionBody）
partBody
    : (definition | usage | attribute)*
    ;

// 动作体：用于状态动作中的动作体
// 参考 SysMLv2参考.g4: ruleActionBody
actionBody
    : '{' actionBodyItem* '}'
    ;

// actionBodyItem：动作体项
// 参考 SysMLv2参考.g4: ruleActionBodyItem
// 简化版本：使用 definitionBodyItem（它已经包含了大部分需要的项）
actionBodyItem
    : definitionBodyItem
    ;

// 状态体：可以包含定义、使用、属性、转换（兼容旧语法，新语法使用 definitionBody）
// 注意：此规则保留用于向后兼容，新的状态体使用 stateBodyItem
stateBodyOld
    : (definition | usage | attribute | transition)*
    ;

// exhibit state 语句
exhibitStatement
    : 'exhibit' 'state' usageDeclaration? stateMachineKind? stateBody
    ;

// 状态机类型
stateMachineKind
    : 'parallel'  // 并行状态机
    ;

// 状态体（用于 exhibit state 和 state definition）
stateBody
    : ';'
    | '{' stateBodyItem* '}'
    ;

// 状态定义体（扩展的 definitionBody，支持状态特定成员）
stateDefinitionBody
    : ';'
    | '{' stateBodyItem* '}'
    ;

// 状态体项
// 注意：顺序很重要！必须先匹配状态特定的成员（stateMember, transitionMember, stateActionMember），
// 然后再匹配通用的 definitionBodyItem，以避免冲突
stateBodyItem
    : stateMember               // 状态成员（嵌套状态）- 必须在 definitionBodyItem 之前
    | transitionMember          // 转换成员 - 必须在 definitionBodyItem 之前
    | stateActionMember         // 状态动作成员（entry/do/exit）- 必须在 definitionBodyItem 之前
    | refStatement              // ref 语句（在状态体中）- 必须在 definitionBodyItem 之前
    | constraintStatement       // constraint 语句 - 必须在 definitionBodyItem 之前
    | definitionBodyItem        // 基础定义体项（attribute, part, action等）- 放在最后
    ;

// constraint 语句（用于状态体中的约束）
// 参考 SysMLv2参考.g4: ruleCalculationBody 可以是 ';' 或 '{' ... '}'
// 所以 constraint {expression} 不需要分号，因为 '{' ... '}' 本身就是完整的形式
constraintStatement
    : 'constraint' identifier? '{' expression '}'
    | 'constraint' ';'  // 简化形式：constraint;
    ;

// 状态成员
stateMember
    : stateUsage
    | stateDefinition
    ;

// 转换成员
transitionMember
    : transition
    ;

// ref 语句（在状态体中）
refStatement
    : 'ref' usageDeclaration? usageCompletion
    ;

// 状态动作成员（占位符，将在阶段二实现）
stateActionMember
    : entryActionMember
    | doActionMember
    | exitActionMember
    ;

// entry 动作成员
entryActionMember
    : visibilityIndicator? 'entry' stateActionUsage
    ;

// do 动作成员
doActionMember
    : visibilityIndicator? 'do' stateActionUsage
    ;

// exit 动作成员
exitActionMember
    : visibilityIndicator? 'exit' stateActionUsage
    ;

// 状态动作使用
// 参考 SysMLv2参考.g4: ruleStateActionUsage
// 1. 空动作：';'
// 2. 执行的动作：rulePerformedActionUsage ruleActionBody（必须有 actionBody，没有分号）
// 注意：'initial' 关键字也可以作为动作名称使用（如 entry action initial;）
stateActionUsage
    : 'action' ('initial' | identifier) ';'     // entry action initial; 或 entry action someAction;
    | ownedReferenceSubsetting (';' | actionBody)  // entry performSelfTest; 或 do senseTemperature { ... }
    ;

// 需求体：可以包含定义、使用、属性、需求特殊成员（兼容旧语法，新语法使用 definitionBody）
requirementBody
    : definitionBody
    ;

// 视点体（继承自 requirementBody）
viewpointBody
    : ';'
    | '{' viewpointBodyItem* '}'
    ;

// 视点体项
viewpointBodyItem
    : definitionBodyItem        // 基础定义体项
    | requirementMember        // 需求成员（subject, actor, stakeholder）
    | requirementConstraintMember  // 需求约束成员
    | frameConcernMember       // 框架关注点成员
    ;

// 框架关注点成员
frameConcernMember
    : visibilityIndicator? 'frame' 'concern' usageDeclaration? ';'
    ;

// 视图定义体
viewDefinitionBody
    : ';'
    | '{' viewDefinitionBodyItem* '}'
    ;

// 视图定义体项
viewDefinitionBodyItem
    : definitionBodyItem        // 基础定义体项
    | elementFilterMember       // 元素过滤成员
    | viewRenderingMember       // 视图渲染成员
    ;

// 元素过滤成员（用于 view def）
elementFilterMember
    : visibilityIndicator? 'filter' filterExpression ';'
    ;

// 过滤表达式（简化实现，支持元数据访问和逻辑组合）
filterExpression
    : metadataAccessExpression
    | filterExpression 'and' filterExpression
    | filterExpression 'or' filterExpression
    | '(' filterExpression ')'
    ;

// 元数据访问表达式（简化实现）
metadataAccessExpression
    : '@' qualifiedName
    | metadataAccessExpression '.' qualifiedName
    ;

// 视图渲染成员
viewRenderingMember
    : visibilityIndicator? 'render' viewRenderingUsage ';'
    ;

// 视图渲染使用
viewRenderingUsage
    : ownedReferenceSubsetting featureSpecializationPart?
    | 'action' usageDeclaration? valuePart? usageBody
    ;

// 视图体（用于 view 使用）
viewBody
    : ';'
    | '{' viewBodyItem* '}'
    ;

// 视图体项
// 注意：顺序很重要！必须先匹配 view 特定的语句（satisfyStatement, exposeStatement），
// 然后再匹配通用的 definitionBodyItem，以避免冲突
viewBodyItem
    : satisfyStatement          // satisfy 语句 - 必须在 definitionBodyItem 之前
    | exposeStatement           // expose 语句 - 必须在 definitionBodyItem 之前
    | elementFilterMember       // 元素过滤成员
    | viewRenderingMember       // 视图渲染成员
    | definitionBodyItem        // 基础定义体项 - 放在最后
    ;

// expose 语句
exposeStatement
    : membershipExpose
    | namespaceExpose
    | (membershipExpose | namespaceExpose) relationshipBody  // 支持带关系体的 expose（可选）
    ;

// 成员暴露
membershipExpose
    : 'expose' qualifiedName ('::' '**')? ';'
    ;

// 命名空间暴露
namespaceExpose
    : 'expose' qualifiedName '::' '*' ('::' '**')? ';'
    ;

// 关注点体
concernBody
    : ';'
    | '{' concernBodyItem* '}'
    ;

// 关注点体项
concernBodyItem
    : definitionBodyItem
    | requirementMember         // subject, actor, stakeholder
    | docStatement
    ;

// 需求特殊成员（subject, actor, stakeholder 等）
requirementMember
    : ('subject' | 'actor' | 'stakeholder') usageDeclaration? usageCompletion
    ;

// 需求约束成员（assume 和 require 在 requirement body 中）
requirementConstraintMember
    : ('assume' | 'require') ('constraint' '{' expression '}' | expression) (';')?
    ;

// 端口体：可以包含定义、使用、属性（兼容旧语法，新语法使用 definitionBody）
portBody
    : (definition | usage | attribute)*
    ;

// 连接器体：可以包含定义、使用、属性（兼容旧语法，新语法使用 definitionBody）
connectorBody
    : (definition | usage | attribute)*
    ;

// 流体：可以包含定义、使用、属性（兼容旧语法，新语法使用 definitionBody）
flowBody
    : (definition | usage | attribute)*
    ;

// 接口体：可以包含定义、使用、属性（兼容旧语法，新语法使用 definitionBody）
interfaceBody
    : (definition | usage | attribute)*
    ;

// 数据类型体：可以包含属性（兼容旧语法，新语法使用 definitionBody）
datatypeBody
    : (attribute)*
    ;

// 类体：可以包含定义、使用、属性（兼容旧语法，新语法使用 definitionBody）
classBody
    : (definition | usage | attribute)*
    ;

// 枚举体：可以包含枚举值和注释（支持分号结束）
enumBody
    : ';'
    | '{' (enumValue | annotatingMember)* '}'
    ;

// 注释成员（简化实现）
annotatingMember
    : docStatement
    | commentStatement
    ;

// doc 语句
docStatement
    : 'doc' identification? blockComment ';'?  // 分号可选
    ;

// comment 语句
commentStatement
    : 'comment' identification? blockComment ';'
    ;

// 使用类型（统一规则）
// 注意：usage 规则现在支持完整的结构（usageDeclaration? usageCompletion）
usage
    : partUsage
    | actionUsage
    | analysisUsage
    | renderingUsage
    | stateUsage
    | requirementUsage
    | constraintUsage
    | portUsage
    | connectorUsage
    | flowUsage
    | connectionUsage
    | allocationUsage
    | itemUsage
    | viewUsage
    ;

// 语句类型（统一规则）
statement
    : bindStatement
    | connectStatement
    | allocateStatement
    | endStatement
    | flowStatement
    | performStatement
    | assertStatement
    | requireStatement
    | assumeStatement
    | redefinesStatement  // redefines 作为独立语句
    | satisfyStatement    // satisfy 语句（用于 view 和 requirement）
    | constraintStatement // named calculation/analysis constraint
    ;

// 绑定语句
bindStatement
    : 'bind' connectorEndMember '=' connectorEndMember ';'
    ;

// 连接语句
connectStatement
    : 'connect' connectorPart ';'
    | 'connect' connectorPart '{' definitionBodyItem* '}'  // 直接使用 definitionBodyItem，避免嵌套的 {}
    ;

// 分配语句
allocateStatement
    : 'allocate' allocationPart ';'
    | 'allocate' allocationPart '{' definitionBodyItem* '}' ';'  // 带体的 allocate 语句，需要分号
    ;

// 分配部分（与连接器部分类似，但用于 allocate）
allocationPart
    : binaryAllocationPart
    | naryAllocationPart
    ;

// 二元分配部分
binaryAllocationPart
    : connectorEndMember 'to' connectorEndMember
    ;

// n元分配部分
naryAllocationPart
    : '(' connectorEndMember (',' connectorEndMember)+ ')' 'to' connectorEndMember
    ;

// 端点语句
endStatement
    : 'end' ('#' identifier)? (name referencesKeyword? ownedReferenceSubsetting | referencesKeyword ownedReferenceSubsetting) ';'
    | 'end' ('#' identifier)? name ':' ('~')? qualifiedName ';'  // 向后兼容旧语法
    ;

// 流定义
flowDefinition
    : occurrenceDefinitionPrefix 'flow' 'def' definitionDeclaration definitionBody
    ;

// 流语句
flowStatement
    : 'flow' flowDeclaration ';'
    ;

// 流声明
flowDeclaration
    : usageDeclaration? valuePart? ('of' payloadFeatureMember)? 
      ('from' flowEndMember 'to' flowEndMember)?
    | flowEndMember 'to' flowEndMember
    ;

// 流端点成员（简化实现，使用 connectorEndMember）
flowEndMember
    : connectorEndMember
    ;

// 载荷特征成员（简化实现）
payloadFeatureMember
    : usageDeclaration? valuePart?
    ;

// 执行语句
performStatement
    : 'perform' (ownedReferenceSubsetting featureSpecializationPart? | 
                 'action' usageDeclaration?) valuePart? usageBody
    ;

// 断言语句
assertStatement
    : 'assert' expression ';'
    ;

// 要求语句
requireStatement
    : 'require' expression ';'
    | 'require' 'constraint' '{' expression '}' ';'  // require constraint {expression}
    ;

// satisfy 语句（用于 view 和 requirement）
satisfyStatement
    : 'satisfy' 'requirement'? ownedReferenceSubsetting featureSpecializationPart? ';'
    | 'satisfy' ownedReferenceSubsetting 'by' ownedReferenceSubsetting '{' requirementBodyItem* '}'
    ;

// 需求体项（用于 satisfy 语句体）
requirementBodyItem
    : definitionBodyItem
    | requirementMember
    | requirementConstraintMember
    ;

// 假设语句
assumeStatement
    : 'assume' expression ';'
    | 'assume' 'constraint' '{' expression '}' ';'  // assume constraint {expression}
    ;

// 重定义语句（redefines 作为独立语句）
redefinesStatement
    : 'redefines' qualifiedName ('=' | ':=') expression ';'  // redefines X = expression
    | 'redefines' qualifiedName 'default' expression ';'  // redefines X default expression
    ;

// 动作输入输出（in item / out item 语法）
actionInputOutput
    : ('in' | 'out') 'item'? usageDeclaration? valuePart? usageBody?
    | ('in' | 'out') 'item' valuePart usageBody?  // 支持 in item = expression（省略 usageDeclaration）
    ;

// 连接器部分
connectorPart
    : binaryConnectorPart
    | naryConnectorPart
    ;

// 二元连接器部分
binaryConnectorPart
    : connectorEndMember 'to' connectorEndMember
    ;

// n元连接器部分
naryConnectorPart
    : '(' connectorEndMember (',' connectorEndMember)+ ')'
    ;

// 连接器端点成员
connectorEndMember
    : ownedMultiplicity? (name referencesKeyword? ownedReferenceSubsetting | ownedReferenceSubsetting)  // 支持 name ::> reference 或直接 reference
    ;

// 拥有的引用子集化（简化实现，支持 qualifiedName、memberAccessExpression 或 identifier）
ownedReferenceSubsetting
    : qualifiedName
    | memberAccessExpression  // 支持点号访问
    | identifier  // 简化：支持简单的标识符
    ;

// 引用关键字
referencesKeyword
    : '::>'
    | 'references'
    ;

// 拥有的多重性（简化实现）
ownedMultiplicity
    : '[' Integer? '..' Integer? ']'
    | '[' Integer ']'  // 简化：单个数字表示 [n..n]
    | '[' '*' ']'
    ;

// 转换（状态机）
// 支持两种形式：
// 1. 简单形式：transition initial then state; 或 transition state1 then state2;
// 2. 复杂形式：transition name ... (将在阶段三实现)
// 注意：转换名称可以是 identifier 或 qualifiedName（如 State1::SubState1_To_State2::SubState3）
transition
    : 'transition' (identifier | qualifiedName)? transitionBody
    ;

// 转换体
transitionBody
    : simpleTransitionBody ';'           // transition initial then off; 或 transition X then Y;
    | complexTransitionBody              // transition name ... (阶段三实现)
    ;

// 简单转换体（阶段一支持）
// 注意：'initial' 可以作为状态名称，也可以作为关键字表示初始状态转换
// 在转换中，如果第一个 stateReference 是 'initial'，则表示初始状态转换
// 否则，表示从源状态到目标状态的转换
simpleTransitionBody
    : 'initial' 'then' stateReference  // transition initial then off;
    | stateReference 'then' stateReference  // transition off then starting; 或 transition State1::SubState then State2;
    ;

// 复杂转换体（阶段三实现）
complexTransitionBody
    : 'first' stateReference
      transitionTriggerPart?
      transitionGuardPart?
      transitionEffectPart?
      'then' stateReference ';'
    ;

// 转换触发部分
transitionTriggerPart
    : acceptClause
    ;

// accept 子句
acceptClause
    : 'accept' (acceptItem | acceptExpression)
    ;

// accept 项
acceptItem
    : usageDeclaration? ('via' ownedReferenceSubsetting)?
    ;

// accept 表达式
acceptExpression
    : 'when' expression
    | 'at' expression
    | expression  // 简单事件引用
    ;

// 转换守卫部分
transitionGuardPart
    : 'if' expression
    ;

// 转换效果部分
transitionEffectPart
    : 'do' transitionAction
    ;

// 转换动作
transitionAction
    : sendStatement
    | actionReference
    | actionInvocation
    ;

// send 语句
// 支持三种形式：
// 1. send new Action() to target
// 2. send Action() to target (无 new)
// 3. send to target (无 actionInvocation)
sendStatement
    : 'send' sendActionPart? 'to' ownedReferenceSubsetting
    ;

// send 动作部分
sendActionPart
    : 'new' actionInvocation  // send new Action() to target
    | actionInvocation        // send Action() to target
    ;

// 动作引用
actionReference
    : ownedReferenceSubsetting
    ;

// 动作调用（简化实现，使用 identifier 和参数列表）
// 支持两种参数形式：
// 1. 位置参数：Action(param1, param2)
// 2. 命名参数：Action(param1=value1, param2=value2)
actionInvocation
    : identifier '(' (actionArgument (',' actionArgument)*)? ')'
    ;

// 动作参数（支持位置参数和命名参数）
actionArgument
    : expression                          // 位置参数：param
    | identifier '=' expression          // 命名参数：param=value
    ;

// 状态引用
// 注意：'initial' 既可以作为关键字（在 transition initial then state 中），
// 也可以作为状态名称（在 state initial; 或 transition state then initial 中）
stateReference
    : 'initial'  // 允许 initial 作为状态名称
    | identifier
    | qualifiedName
    ;

// 触发表达式
triggerExpression
    : qualifiedName ('|' qualifiedName)*
    ;

// 守卫表达式（暂时简化，后续支持完整表达式）
guardExpression
    : identifier
    | qualifiedName
    ;

// 效果表达式
effectExpression
    : qualifiedName (',' qualifiedName)*
    ;

// 枚举值
enumValue
    : identifier ('=' Integer)? ';'
    ;

// 属性（Attribute）- 可以是属性使用或属性定义
// 支持类型化（:）、子集化（:>）、重定义（:>>, redefines）
// 支持特征前缀（abstract, const, derived, in/out/inout, ref）
// 注意：attribute 使用必须在 attributeDefinition 之前，以确保 'attribute redefines' 能正确匹配
attribute
    : 'attribute' featurePrefix* attributeContent ';'
    | attributeDefinition  // attribute def ... 形式
    ;

// 特征前缀
featurePrefix
    : 'abstract'
    | 'const' | 'constant'
    | 'derived'
    | 'in' | 'out' | 'inout'
    | 'ref'
    ;

// 属性内容：标识符 + 表达式，或仅表达式（重定义时）
attributeContent
    : 'redefines' qualifiedName ('=' | ':=') expression  // attribute redefines X = expression（必须在前面，优先匹配）
    | 'redefines' qualifiedName 'default' expression  // attribute redefines X default expression（必须在前面，优先匹配）
    | identifier attributeExpressions? valuePart?  // 支持 identifier :>Type = value
    | redefinitionExpression (typingExpression | subsettingExpression | valuePart)?  // 支持 redefines X = expression
    | attributeExpressions valuePart?  // 支持只有表达式 + valuePart
    ;

// 属性表达式：可以包含子集化、重定义、类型化
attributeExpressions
    : (subsettingExpression | redefinitionExpression)? typingExpression?
    | typingExpression? (subsettingExpression | redefinitionExpression)?
    ;

// 子集化表达式（:> 或 subsets 关键字）
subsettingExpression
    : SUBSETS_OP qualifiedName
    | 'subsets' qualifiedName
    ;

// 重定义表达式（:>> 或 redefines 关键字）
redefinitionExpression
    : REDEFINES_OP qualifiedName ownedMultiplicity?
    | 'redefines' qualifiedName ownedMultiplicity?
    ;

// 类型化表达式（:）
typingExpression
    : ':' qualifiedName
    ;

// 子类化表达式（:> 或 subclass/specializes 关键字）
// 注意：此规则保留用于向后兼容，新语法使用 subclassificationPart
subclassificationExpression
    : SUBSETS_OP qualifiedName (',' qualifiedName)*
    | ('subclass' | 'specializes') qualifiedName (',' qualifiedName)*
    ;

// 属性体：可以包含定义、使用、属性
attributeBody
    : (definition | usage | attribute)*
    ;

// 导入语句（支持可见性、通配符、all 关键字）
importStatement
    : (membershipImport | namespaceImport) relationshipBody
    ;

// 成员导入
membershipImport
    : importPrefix importedMembership
    ;

// 导入的成员
importedMembership
    : qualifiedName ('::' '**')?
    ;

// 命名空间导入
namespaceImport
    : importPrefix (importedNamespace | filterPackage)
    ;

// 导入的命名空间
importedNamespace
    : qualifiedName '::' '*' ('::' '**')?
    ;

// 过滤包（简化实现，暂时不支持）
filterPackage
    : qualifiedName '::' '*'  // 简化实现
    ;

// 导入前缀
importPrefix
    : visibilityIndicator? 'import' 'all'?
    ;

// 关系体（用于导入语句）
relationshipBody
    : ';'
    | '{' relationshipBodyItem* '}'
    ;

// 关系体项（简化实现）
relationshipBodyItem
    : annotatingMember
    ;

// 关系类型（统一规则）
relationship
    : subclassification
    | subsetting
    | redefinition
    | conjugation
    | dependency
    ;

// 子类化关系（独立语句形式）
subclassification
    : typeExpression ('subclass' | 'specializes') qualifiedName (',' qualifiedName)* ';'
    ;

// 子集化关系（独立语句形式）
subsetting
    : featureExpression 'subsets' qualifiedName ';'
    ;

// 重定义关系（独立语句形式）
redefinition
    : featureExpression 'redefines' qualifiedName ';'
    ;

// 共轭关系
conjugation
    : typeExpression 'conjugates' qualifiedName ';'
    ;

// 依赖关系
dependency
    : elementExpression 'dependency' qualifiedName ';'
    | 'dependency' 'from' qualifiedName 'to' qualifiedName ';'  // dependency from X to Y
    ;

// 类型表达式（用于关系）
typeExpression
    : qualifiedName
    ;

// 特征表达式（用于关系）
featureExpression
    : qualifiedName
    ;

// 元素表达式（用于关系）
elementExpression
    : qualifiedName
    ;

// 限定名称（命名空间路径）
qualifiedName
    : identifier ('::' identifier)*
    | memberAccessExpression  // 支持点号访问（如 generateTorque.engineTorque）
    ;

// 成员访问表达式（支持点号访问，如 generateTorque.engineTorque）
memberAccessExpression
    : identifier ('.' identifier)+
    | identifier ('::' identifier)* '.' identifier  // 支持 qualifiedName 形式的点号访问
    ;

// 标识符
identifier
    : IDENTIFIER
    ;

// ============================================
// 词法规则
// ============================================

// 标识符：字母、数字、下划线，不能以数字开头
IDENTIFIER
    : [a-zA-Z_][a-zA-Z0-9_]*
    ;

// 整数
Integer
    : [0-9]+
    ;

// 关系操作符（必须在 COLON 之前定义，以确保正确匹配）
SUBSETS_OP
    : ':>'
    ;

REDEFINES_OP
    : ':>>'
    ;

// 冒号（用于类型化，必须在 SUBSETS_OP 和 REDEFINES_OP 之后）
COLON
    : ':'
    ;

// 空白字符：跳过
WS
    : [ \t\r\n]+ -> skip
    ;

// 单行注释：跳过
LINE_COMMENT
    : '//' ~[\r\n]* -> skip
    ;

// 块注释令牌（用于语法规则，必须在 BLOCK_COMMENT 之前，以便在 doc/comment 语句中使用）
// 注意：这个规则用于 doc 和 comment 语句，不会被跳过
BLOCK_COMMENT_TOKEN
    : '/*' .*? '*/'
    ;

// 块注释：跳过（用于词法分析，默认行为）
// 注意：这个规则必须在 BLOCK_COMMENT_TOKEN 之后，以便 BLOCK_COMMENT_TOKEN 优先匹配
BLOCK_COMMENT
    : '/*' .*? '*/' -> skip
    ;

// 块注释（用于语法规则）
blockComment
    : BLOCK_COMMENT_TOKEN
    ;
