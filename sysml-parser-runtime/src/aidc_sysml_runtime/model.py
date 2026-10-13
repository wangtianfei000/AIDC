from __future__ import annotations

from dataclasses import dataclass, field
from hashlib import sha256
import re
from typing import Any

from . import PARSER_BASELINE, SCHEMA_VERSION


@dataclass
class Token:
    text: str
    start: int
    end: int


@dataclass
class Element:
    kind: str
    name: str
    start: int
    end: int
    type_name: str = ""
    expression: str = ""
    owner: Element | None = None
    children: list[Element] = field(default_factory=list)
    properties: dict[str, Any] = field(default_factory=dict)
    element_id: str = ""

    @property
    def qualified_name(self) -> str:
        return f"{self.owner.qualified_name}::{self.name}" if self.owner else self.name

    def add(self, child: Element) -> Element:
        child.owner = self
        self.children.append(child)
        return child


KEYWORDS = {
    "package", "requirement", "part", "port", "attribute", "calc", "analysis",
    "constraint", "require", "assert", "assume", "rendering", "view", "connect",
    "doc", "subject", "render", "expose", "in", "out", "return",
}


def tokenize(source: str) -> tuple[list[Token], list[dict[str, Any]]]:
    tokens: list[Token] = []
    diagnostics: list[dict[str, Any]] = []
    i = 0
    while i < len(source):
        c = source[i]
        if c.isspace():
            i += 1; continue
        if source.startswith("//", i):
            end = source.find("\n", i + 2)
            i = len(source) if end < 0 else end
            continue
        if source.startswith("/*", i):
            end = source.find("*/", i + 2)
            if end < 0:
                diagnostics.append({"code": "AIDC_UNTERMINATED_COMMENT", "offset": i, "message": "Unterminated block comment"})
                break
            tokens.append(Token("__DOC__" + source[i + 2:end].strip(), i, end + 2)) if tokens and tokens[-1].text == "doc" else None
            i = end + 2; continue
        start = i
        if c in "\"'":
            quote = c; i += 1
            while i < len(source) and source[i] != quote:
                i += 2 if source[i] == "\\" else 1
            if i >= len(source):
                diagnostics.append({"code": "AIDC_UNTERMINATED_STRING", "offset": start, "message": "Unterminated quoted value"})
            else: i += 1
            tokens.append(Token(source[start:i], start, i)); continue
        if c.isalpha() or c == "_":
            i += 1
            while i < len(source) and (source[i].isalnum() or source[i] == "_"): i += 1
            tokens.append(Token(source[start:i], start, i)); continue
        if c.isdigit() or (c == "." and i + 1 < len(source) and source[i + 1].isdigit()):
            i += 1
            while i < len(source) and (source[i].isdigit() or source[i] in ".eE+-"):
                if source[i] in "+-" and source[i - 1] not in "eE": break
                i += 1
            tokens.append(Token(source[start:i], start, i)); continue
        op = next((value for value in (":>>", ":>", "::", "<=", ">=", "==", "!=", "->") if source.startswith(value, i)), None)
        if op:
            i += len(op); tokens.append(Token(op, start, i)); continue
        i += 1; tokens.append(Token(c, start, i))
        if c == "@" and i < len(source) and source[i] == "@":
            diagnostics.append({"code": "AIDC_SYNTAX", "offset": start, "message": "Unexpected character '@'"})
    return tokens, diagnostics


class Parser:
    def __init__(self, source: str):
        self.source = source
        self.tokens, self.diagnostics = tokenize(source)
        self.i = 0
        self.roots: list[Element] = []

    def peek(self, offset: int = 0) -> str:
        index = self.i + offset
        return self.tokens[index].text if index < len(self.tokens) else ""

    def take(self, value: str) -> bool:
        if self.peek() == value: self.i += 1; return True
        return False

    def next(self) -> Token:
        if self.i >= len(self.tokens): return Token("", len(self.source), len(self.source))
        value = self.tokens[self.i]; self.i += 1; return value

    def name(self) -> Token:
        value = self.next()
        if len(value.text) >= 2 and value.text[0] == value.text[-1] == "'":
            value.text = value.text[1:-1].replace("''", "'")
        return value

    def attach(self, owner: Element | None, element: Element) -> Element:
        if owner: owner.add(element)
        else: self.roots.append(element)
        return element

    def collect(self, terminators: set[str]) -> tuple[str, int]:
        if not self.peek(): return "", len(self.source)
        begin = self.tokens[self.i].start; end = begin; round_depth = square_depth = 0
        while self.peek():
            value = self.peek()
            if round_depth == square_depth == 0 and value in terminators: break
            token = self.next(); end = token.end
            if value == "(": round_depth += 1
            elif value == ")": round_depth -= 1
            elif value == "[": square_depth += 1
            elif value == "]": square_depth -= 1
        return self.source[begin:end].strip(), end

    def parse_type(self) -> str:
        if not (self.take(":") or self.take(":>") or self.take(":>>")): return ""
        parts: list[str] = []
        while self.peek() and self.peek() not in {"=", ";", "{"}:
            part = self.next().text
            if part == "::" and parts: parts[-1] += "::"
            elif parts and parts[-1].endswith("::"): parts[-1] += part
            else: parts.append(part)
        return " ".join(parts).strip()

    def parse_body(self, element: Element) -> None:
        if self.take("{"): self.scope(element, True)
        else: self.take(";")

    def skip(self) -> None:
        depth = 0
        while self.peek():
            if self.take("{"): depth += 1
            elif self.peek() == "}":
                if depth == 0: return
                self.i += 1; depth -= 1
            elif self.take(";") and depth == 0: return
            else: self.i += 1

    def scan_unknown(self, owner: Element | None) -> None:
        """Keep walking nested declarations that the read-only subset does not model."""
        round_depth = square_depth = 0
        while self.peek():
            value = self.peek()
            if value == "(" : round_depth += 1
            elif value == ")": round_depth = max(0, round_depth - 1)
            elif value == "[": square_depth += 1
            elif value == "]": square_depth = max(0, square_depth - 1)
            elif value == "{" and round_depth == square_depth == 0:
                self.i += 1; self.scope(owner, True); return
            elif value == ";" and round_depth == square_depth == 0:
                self.i += 1; return
            elif value == "}" and round_depth == square_depth == 0: return
            self.i += 1

    def declaration(self, owner: Element | None, kind: str, typed: bool = False) -> None:
        name = self.name(); type_name = self.parse_type() if typed else ""
        element = self.attach(owner, Element(kind, name.text, name.start, name.end, type_name=type_name))
        self.parse_body(element)

    def feature(self, owner: Element | None, role: str = "local") -> None:
        name = self.name(); type_name = self.parse_type(); expression = ""; end = name.end
        if self.take("="): expression, end = self.collect({";", "}"})
        item = Element("AttributeUsage", name.text, name.start, end, type_name, expression, properties={"role": role})
        self.attach(owner, item); self.take(";")

    def constraint(self, owner: Element | None, keyword: str, start: int) -> None:
        if self.peek() == "constraint": self.next()
        name = keyword if self.peek() == "{" else self.name().text
        if not self.take("{"): self.skip(); return
        expression, end = self.collect({"}"}); self.take("}"); self.take(";")
        kind = {"require": "RequireStatement", "assert": "AssertStatement", "assume": "AssumeStatement"}.get(keyword, "ConstraintStatement")
        self.attach(owner, Element(kind, name, start, end, expression=expression))

    def scope(self, owner: Element | None, closing: bool = False) -> None:
        while self.peek():
            if self.peek() == "}":
                if closing: self.i += 1
                return
            if self.take(";"): continue
            keyword_token = self.next(); keyword = keyword_token.text
            if keyword == "package": self.declaration(owner, "Package")
            elif keyword == "requirement":
                if self.take("def"): self.declaration(owner, "RequirementDefinition")
                else:
                    if self.take("<"):
                        while self.peek() and not self.take(">"): self.next()
                    self.declaration(owner, "RequirementUsage", typed=True)
            elif keyword == "part": self.declaration(owner, "PartDefinition" if self.take("def") else "PartUsage", typed=self.tokens[self.i - 1].text != "def")
            elif keyword == "port": self.declaration(owner, "PortDefinition" if self.take("def") else "PortUsage", typed=self.tokens[self.i - 1].text != "def")
            elif keyword == "attribute": self.feature(owner, "result" if owner and owner.kind == "AnalysisCase" else "local")
            elif keyword == "calc" and self.take("def"): self.declaration(owner, "CalculationDefinition")
            elif keyword in {"in", "out"}:
                name = self.name(); type_name = self.parse_type(); expression = ""; end = name.end
                if self.take("="): expression, end = self.collect({";", "}"})
                self.attach(owner, Element("CalculationParameter", name.text, name.start, end, type_name, expression, properties={"role": "input" if keyword == "in" else "output"})); self.take(";")
            elif keyword == "return":
                name = self.name(); type_name = self.parse_type(); expression = ""; end = name.end
                if self.take("="): expression, end = self.collect({";", "}"})
                self.attach(owner, Element("CalculationParameter", name.text, name.start, end, type_name, expression, properties={"role": "output"})); self.take(";")
            elif keyword == "analysis": self.declaration(owner, "AnalysisCase")
            elif keyword == "subject":
                self.take("="); expression, _ = self.collect({";", "}"}); self.take(";")
                if owner: owner.properties["subjectReference"] = expression.strip()
            elif keyword in {"constraint", "require", "assert", "assume"}: self.constraint(owner, keyword, keyword_token.start)
            elif keyword == "connect":
                left, _ = self.collect({"to", ";"})
                if self.take("to"):
                    right, end = self.collect({";", "}"}); self.take(";")
                    self.attach(owner, Element("Connection", "connect", keyword_token.start, end, properties={"ends": [re.sub(r"\s+", "", left), re.sub(r"\s+", "", right)]}))
                else: self.skip()
            elif keyword == "rendering": self.declaration(owner, "RenderingUsage", typed=True)
            elif keyword == "view": self.declaration(owner, "ViewDefinition" if self.take("def") else "ViewUsage", typed=True)
            elif keyword == "render":
                ref, end = self.collect({";", "}"}); self.take(";")
                self.attach(owner, Element("ViewRenderingMember", ref.strip(), keyword_token.start, end, properties={"reference": ref.strip()}))
            elif keyword == "expose":
                ref, end = self.collect({";", "}"}); self.take(";")
                self.attach(owner, Element("ExposeStatement", "expose", keyword_token.start, end, properties={"reference": re.sub(r"\s+", "", ref)}))
            elif keyword in {"public", "private"} and self.take("import"):
                ref, end = self.collect({";"}); self.take(";")
                self.attach(owner, Element("NamespaceImport", "import", keyword_token.start, end, properties={"namespace": ref.strip()}))
            elif keyword == "doc":
                content = self.next().text.removeprefix("__DOC__") if self.peek().startswith("__DOC__") else ""
                self.attach(owner, Element("DocStatement", "doc", keyword_token.start, keyword_token.end, properties={"content": content}))
            else: self.scan_unknown(owner)
        if closing:
            self.diagnostics.append({"code": "AIDC_MISSING_BRACE", "offset": len(self.source), "message": "Missing closing brace"})

    def parse(self) -> tuple[list[Element], list[dict[str, Any]]]:
        self.scope(None)
        return self.roots, self.diagnostics


def flatten(roots: list[Element]) -> list[Element]:
    result: list[Element] = []
    def visit(item: Element) -> None:
        result.append(item)
        for child in item.children: visit(child)
    for root in roots: visit(root)
    return result


def source_span(uri: str, text: str, start: int, end: int) -> dict[str, Any]:
    def position(offset: int) -> dict[str, int]:
        line = text.count("\n", 0, offset) + 1
        previous = text.rfind("\n", 0, offset)
        return {"line": line, "column": offset - previous, "offset": offset}
    return {"uri": uri, "start": position(start), "end": position(end)}


def stable_id(prefix: str, *parts: str) -> str:
    return prefix + sha256("|".join(parts).encode()).hexdigest()[:24]


CAPABILITIES = {
    "package": "supported", "part.definition": "supported", "part.usage": "supported",
    "port.definition": "supported", "port.usage": "supported", "attribute": "supported",
    "requirement": "supported", "connection": "supported", "constraint": "supported",
    "calculation.definition": "supported", "calculation.analysis": "supported",
    "view.table-rendering": "supported", "state-machine": "partial", "action": "partial",
}


def make_snapshot(uri: str, text: str) -> tuple[dict[str, Any], list[Element]]:
    roots, raw_diagnostics = Parser(text).parse()
    records = flatten(roots)
    # The read-only baseline excludes requirements nested in a parts-tree
    # configuration; those are configuration selections rather than requirement usages.
    records = [item for item in records if not (item.kind == "RequirementUsage" and "::PartsTree::" in item.qualified_name)]
    occurrences: dict[str, int] = {}
    for item in records:
        key = f"{uri}|{item.kind}|{item.qualified_name}|{item.start}"
        occurrence = occurrences.get(key, 0); occurrences[key] = occurrence + 1
        item.element_id = stable_id("e-", key, str(occurrence))
    by_qn = {item.qualified_name: item for item in records}
    by_name: dict[str, list[Element]] = {}
    for item in records: by_name.setdefault(item.name, []).append(item)

    def resolve(name: str, owner: Element | None = None) -> Element | None:
        name = name.replace(".", "::")
        if name in by_qn: return by_qn[name]
        current = owner
        while current:
            candidate = f"{current.qualified_name}::{name}"
            if candidate in by_qn: return by_qn[candidate]
            current = current.owner
        matches = by_name.get(name.split("::")[-1], [])
        return matches[0] if len(matches) == 1 else None

    elements: list[dict[str, Any]] = []
    relationships: list[dict[str, Any]] = []
    diagnostics: list[dict[str, Any]] = []
    for raw in raw_diagnostics:
        diagnostics.append({"code": raw["code"], "severity": "error", "message": raw["message"], "source": source_span(uri, text, raw["offset"], min(raw["offset"] + 1, len(text)))})

    relationship_kinds = {"Connection", "NamespaceImport"}
    for item in records:
        source = source_span(uri, text, item.start, item.end)
        owner_id = item.owner.element_id if item.owner else None
        properties = dict(item.properties)
        if item.type_name: properties.update({"typeName": item.type_name, "typeResolution": "local" if resolve(item.type_name) else "official-library"})
        if item.expression: properties.update({"valueExpression": item.expression, "defaultValue": item.expression})
        if item.kind in relationship_kinds:
            source_ids: list[str] = []; target_ids: list[str] = []
            if item.kind == "Connection":
                left, right = item.properties["ends"]
                left_item, right_item = resolve(left, item.owner), resolve(right, item.owner)
                if left_item: source_ids.append(left_item.element_id)
                if right_item: target_ids.append(right_item.element_id)
            relationships.append({"id": item.element_id, "kind": item.kind, "sourceIds": source_ids, "targetIds": target_ids, "ownerId": owner_id, "source": source, "properties": properties})
        else:
            elements.append({"id": item.element_id, "kind": item.kind, "name": item.name or None, "qualifiedName": item.qualified_name or None, "ownerId": owner_id, "source": source, "properties": properties})
        if item.type_name:
            target = resolve(item.type_name)
            relationships.append({"id": stable_id("r-", item.element_id, "typing"), "kind": "FeatureTyping", "sourceIds": [item.element_id], "targetIds": [target.element_id] if target else [], "ownerId": owner_id, "source": source, "properties": {"typeName": item.type_name}})
        constraint_context = item.owner
        while constraint_context and constraint_context.kind not in {"AnalysisCase", "RequirementDefinition", "RequirementUsage"}:
            constraint_context = constraint_context.owner
        if item.kind in {"ConstraintStatement", "RequireStatement", "AssertStatement", "AssumeStatement"} and item.owner and (constraint_context or item.name == "selectionConstraint"):
            relationships.append({"id": stable_id("r-", item.element_id, "constraint"), "kind": item.kind.replace("Statement", "Constraint"), "sourceIds": [item.owner.element_id], "targetIds": [item.element_id], "ownerId": item.owner.element_id, "source": source, "properties": {"expression": item.expression, "statementKind": item.kind}})

    calculation_model = project_calculations(records, resolve, uri, text)
    view_model = project_views(records, resolve)
    snapshot = {
        "schemaVersion": SCHEMA_VERSION,
        "document": {"uri": uri, "contentHash": sha256(text.encode()).hexdigest(), "parser": {"name": "AIDC Standard Library Parser", "baseline": PARSER_BASELINE, "capabilityVersion": "1.0"}},
        "elements": elements, "relationships": relationships,
        "calculationModel": calculation_model, "viewModel": view_model,
        "diagnostics": diagnostics, "capabilities": CAPABILITIES,
    }
    return snapshot, records


def project_calculations(records: list[Element], resolve, uri: str, text: str) -> dict[str, Any]:
    parameters: list[dict[str, Any]] = []
    definitions: list[dict[str, Any]] = []
    analyses: list[dict[str, Any]] = []
    constraints: list[dict[str, Any]] = []
    usages: list[dict[str, Any]] = []
    for item in records:
        if item.kind in {"AttributeUsage", "CalculationParameter"}:
            role = item.properties.get("role", "local")
            parameter = {"id": item.element_id, "elementId": item.element_id, "name": item.name, "role": role, "typeName": item.type_name or None, "expression": item.expression or None, "expressionAst": expression_ast(item.expression) if item.expression else None, "unit": None, "ownerId": item.owner.element_id if item.owner else None, "source": source_span(uri, text, item.start, item.end)}
            parameters.append(parameter)
            ast = parameter["expressionAst"]
            if role == "result" and ast and ast.get("kind") == "call":
                definition = resolve(ast["callee"])
                usages.append({"id": stable_id("cu-", item.element_id, ast["callee"]), "resultParameterId": item.element_id, "definitionName": ast["callee"], "definitionId": definition.element_id if definition else None, "analysisId": item.owner.element_id if item.owner else None, "arguments": ast["arguments"], "bindings": [], "actualParameterIds": [], "source": parameter["source"]})
        if item.kind == "ConstraintStatement" and item.owner and item.owner.kind == "AnalysisCase":
            constraints.append({"id": item.element_id, "elementId": item.element_id, "name": item.name, "analysisId": item.owner.element_id, "expression": item.expression, "expressionAst": expression_ast(item.expression), "source": source_span(uri, text, item.start, item.end)})
    by_owner: dict[str, list[dict[str, Any]]] = {}
    for parameter in parameters: by_owner.setdefault(parameter["ownerId"] or "", []).append(parameter)
    for item in records:
        if item.kind == "CalculationDefinition":
            owned = by_owner.get(item.element_id, [])
            definitions.append({"id": item.element_id, "elementId": item.element_id, "name": item.name, "inputParameterIds": [x["id"] for x in owned if x["role"] == "input"], "outputParameterIds": [x["id"] for x in owned if x["role"] == "output"], "localParameterIds": [x["id"] for x in owned if x["role"] == "local"], "source": source_span(uri, text, item.start, item.end)})
        elif item.kind == "AnalysisCase":
            analyses.append({"id": item.element_id, "elementId": item.element_id, "name": item.name, "subjectReference": item.properties.get("subjectReference", ""), "subjectId": (resolve(item.properties.get("subjectReference", ""), item).element_id if resolve(item.properties.get("subjectReference", ""), item) else None), "resultParameterIds": [x["id"] for x in by_owner.get(item.element_id, [])], "constraintIds": [x["id"] for x in constraints if x["analysisId"] == item.element_id], "calculationUsageIds": [x["id"] for x in usages if x["analysisId"] == item.element_id], "source": source_span(uri, text, item.start, item.end)})
    parameter_by_id = {p["id"]: p for p in parameters}
    definition_by_id = {d["id"]: d for d in definitions}
    for usage in usages:
        definition = definition_by_id.get(usage["definitionId"])
        if not definition: continue
        for formal_id, argument in zip(definition["inputParameterIds"], usage["arguments"]):
            formal = parameter_by_id[formal_id]
            ref = argument.get("path", "") if argument.get("kind") == "reference" else ""
            actual = next((p for p in parameters if p["name"] == ref.split(".")[-1]), None)
            usage["bindings"].append({"formalParameterId": formal_id, "formalParameterName": formal["name"], "actualParameterId": actual["id"] if actual else None, "actualReference": ref})
            if actual: usage["actualParameterIds"].append(actual["id"])
    return {"version": "0.1", "definitions": definitions, "usages": usages, "analyses": analyses, "parameters": parameters, "constraints": constraints}


def project_views(records: list[Element], resolve) -> dict[str, Any]:
    renderings: list[dict[str, Any]] = []; views: list[dict[str, Any]] = []; definitions: list[dict[str, Any]] = []
    def rendering_target(reference: str) -> Element | None:
        return next((record for record in records if record.kind == "RenderingUsage" and (record.name == reference or record.qualified_name == reference)), None)
    for item in records:
        if item.kind == "RenderingUsage":
            columns = []
            for child in item.children:
                if child.kind != "ViewUsage": continue
                label = next((x.expression.strip('"') for x in child.children if x.name == "label"), child.name)
                columns.append({"key": child.name, "label": label, "viewId": child.element_id})
            renderings.append({"id": item.element_id, "elementId": item.element_id, "name": item.name, "typeName": item.type_name, "renderingKind": "element-table" if "ElementTable" in item.type_name else item.type_name, "columns": columns})
        elif item.kind == "ViewDefinition":
            render = next((x.properties["reference"] for x in item.children if x.kind == "ViewRenderingMember"), "")
            target = rendering_target(render)
            definitions.append({"id": item.element_id, "elementId": item.element_id, "name": item.name, "typeName": item.type_name, "renderingId": target.element_id if target else None})
        elif item.kind == "ViewUsage":
            if item.owner and item.owner.kind == "RenderingUsage": continue
            render = next((x.properties["reference"] for x in item.children if x.kind == "ViewRenderingMember"), "")
            target = rendering_target(render)
            exposed = []
            for child in item.children:
                if child.kind == "ExposeStatement":
                    found = resolve(child.properties["reference"], item)
                    if found: exposed.append(found.element_id)
            views.append({"id": item.element_id, "elementId": item.element_id, "name": item.name, "typeName": item.type_name, "renderingId": target.element_id if target else None, "exposedElementIds": exposed})
    return {"version": "0.1", "definitions": definitions, "views": views, "renderings": renderings}


def expression_tokens(value: str) -> list[str]:
    return re.findall(r'"(?:\\.|[^"\\])*"|<=|>=|==|!=|[A-Za-z_]\w*(?:(?:::|\.)[A-Za-z_]\w*)*|(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?|[()+\-*/,\[\]<>]', value)


class ExpressionParser:
    def __init__(self, value: str): self.tokens = expression_tokens(value); self.i = 0
    def peek(self): return self.tokens[self.i] if self.i < len(self.tokens) else ""
    def take(self): value = self.peek(); self.i += bool(value); return value
    def parse(self, minimum: int = 0) -> dict[str, Any]:
        token = self.take()
        if token in {"+", "-"}: left = {"kind": "unary", "operator": token, "operand": self.parse(5)}
        elif token == "(": left = self.parse(); self.take() if self.peek() == ")" else None
        elif token.startswith('"'): left = {"kind": "literal", "value": token[1:-1], "raw": token}
        elif re.fullmatch(r"(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?", token):
            left = {"kind": "literal", "value": float(token), "raw": token}
            if self.peek() == "[":
                self.take(); unit = self.take(); self.take() if self.peek() == "]" else None; left["unit"] = unit
        else:
            if self.peek() == "(":
                self.take(); args = []
                while self.peek() and self.peek() != ")":
                    args.append(self.parse()); self.take() if self.peek() == "," else None
                self.take() if self.peek() == ")" else None
                left = {"kind": "call", "callee": token, "arguments": args}
            else: left = {"kind": "reference", "path": token}
        precedence = {"==": 1, "!=": 1, "<": 1, "<=": 1, ">": 1, ">=": 1, "+": 2, "-": 2, "*": 3, "/": 3}
        while self.peek() in precedence and precedence[self.peek()] >= minimum:
            op = self.take(); right = self.parse(precedence[op] + 1)
            left = {"kind": "binary", "operator": op, "left": left, "right": right}
        return left


def expression_ast(value: str) -> dict[str, Any]:
    return ExpressionParser(value).parse() if value.strip() else {}
