from __future__ import annotations

from dataclasses import dataclass, field
import math
from typing import Any, Callable

from . import SCHEMA_VERSION
from .model import Element, expression_ast, flatten, make_snapshot, stable_id


DIMENSIONS = {
    "Real": {}, "Integer": {}, "Boolean": {},
    "ISQ::mass": {"M": 1}, "ISQBase::mass": {"M": 1},
    "ISQ::duration": {"T": 1}, "Time": {"T": 1},
    "ISQ::electricCurrent": {"I": 1}, "ISQ::electricCharge": {"I": 1, "T": 1},
    "ISQ::electricPotential": {"V": 1}, "ISQ::energy": {"V": 1, "I": 1, "T": 1},
    "ISQ::power": {"V": 1, "I": 1}, "ISQ::angularMeasure": {"A": 1},
}
UNITS = {
    "": (1.0, {}), "g": (0.001, {"M": 1}), "kg": (1.0, {"M": 1}),
    "ms": (0.001, {"T": 1}), "s": (1.0, {"T": 1}), "h": (3600.0, {"T": 1}),
    "A": (1.0, {"I": 1}), "Ah": (3600.0, {"I": 1, "T": 1}),
    "V": (1.0, {"V": 1}), "W": (1.0, {"V": 1, "I": 1}),
    "Wh": (3600.0, {"V": 1, "I": 1, "T": 1}), "J": (1.0, {"V": 1, "I": 1, "T": 1}),
    "deg": (1.0, {"A": 1}), "rad": (1.0, {"A": 1}),
}


@dataclass
class Quantity:
    valid: bool = True
    si: float = 0.0
    dims: dict[str, int] = field(default_factory=dict)
    unit: str = ""
    boolean: bool = False
    bool_value: bool = False
    status: str = "computed"
    message: str = ""
    source_ids: set[str] = field(default_factory=set)


def failure(status: str, message: str) -> Quantity:
    return Quantity(valid=False, status=status, message=message)


def combined(a: dict[str, int], b: dict[str, int], sign: int) -> dict[str, int]:
    result = dict(a)
    for key, value in b.items():
        result[key] = result.get(key, 0) + sign * value
        if result[key] == 0: del result[key]
    return result


def dimension_text(value: dict[str, int]) -> str:
    if not value: return "1"
    return " ".join(f"{key}^{power}" if power != 1 else key for key, power in sorted(value.items()))


class Evaluator:
    def __init__(self, snapshot: dict[str, Any], records: list[Element]):
        self.snapshot = snapshot; self.records = records
        self.by_qn = {x.qualified_name: x for x in records}
        self.by_name: dict[str, list[Element]] = {}
        for item in records: self.by_name.setdefault(item.name, []).append(item)
        self.analysis_results: dict[str, Element] = {}
        self.values: dict[str, Quantity] = {}; self.visiting: set[str] = set()
        self.diagnostics: list[dict[str, Any]] = []; self.order: list[str] = []; self.stack: list[str] = []
        self.dependencies: set[tuple[str, str]] = set()

    def resolve_local(self, owner: Element | None, name: str) -> Element | None:
        if not owner: return None
        direct = next((x for x in owner.children if x.name == name), None)
        if direct: return direct
        if owner.type_name:
            type_def = self.resolve_simple(owner.type_name)
            if type_def:
                direct = next((x for x in type_def.children if x.name == name), None)
                if direct: return direct
        return None

    def resolve_simple(self, name: str) -> Element | None:
        normalized = name.replace(".", "::")
        if normalized in self.by_qn: return self.by_qn[normalized]
        matches = self.by_name.get(normalized.split("::")[-1], [])
        return matches[0] if matches else None

    def resolve_path(self, path: str, analysis: Element | None = None) -> Element | None:
        simple = path.split(".")[-1]
        if analysis:
            local = next((x for x in analysis.children if x.name == simple and x.kind == "AttributeUsage"), None)
            if local and "." not in path: return local
        parts = path.split(".")
        current = self.resolve_simple(parts[0])
        for part in parts[1:]:
            current = self.resolve_local(current, part)
            if not current: break
        return current

    def evaluate_element(self, element: Element, analysis: Element | None = None, locals_: dict[str, Quantity] | None = None, local_resolver: Callable[[str], Quantity] | None = None) -> Quantity:
        if element.element_id in self.values: return self.values[element.element_id]
        if element.element_id in self.visiting:
            value = failure("cycle", f"Circular dependency at {element.name}")
            self.values[element.element_id] = value; return value
        self.visiting.add(element.element_id); self.stack.append(element.element_id)
        value = self.evaluate_ast(expression_ast(element.expression), analysis, locals_ or {}, local_resolver)
        expected = DIMENSIONS.get(element.type_name, {})
        if value.valid and expected and expected != value.dims:
            value = failure("unit-error", f"{element.name} expects {dimension_text(expected)} but expression has {dimension_text(value.dims)}")
        if value.valid: value.source_ids.add(element.element_id)
        elif value.status != "cycle":
            self.diagnostics.append({"code": "AIDC_ANALYSIS_" + value.status.upper().replace("-", "_"), "severity": "error", "message": value.message, "parameterId": element.element_id})
        self.stack.pop(); self.visiting.remove(element.element_id); self.values[element.element_id] = value; self.order.append(element.element_id)
        return value

    def evaluate_ast(self, node: dict[str, Any], analysis: Element | None, locals_: dict[str, Quantity], local_resolver=None) -> Quantity:
        if not node: return failure("missing-input", "Expression is missing")
        kind = node.get("kind")
        if kind == "literal":
            if isinstance(node.get("value"), str): return failure("unsupported", "String arithmetic is unsupported")
            unit = node.get("unit", "")
            if unit not in UNITS: return failure("unit-error", f"Unsupported unit {unit}")
            factor, dims = UNITS[unit]
            return Quantity(si=float(node["value"]) * factor, dims=dict(dims), unit=unit)
        if kind == "reference":
            path = node["path"]; simple = path.split(".")[-1]
            if path in locals_: return locals_[path]
            if simple in locals_: return locals_[simple]
            if local_resolver:
                result = local_resolver(simple)
                if result.valid or result.status != "missing-input": return result
            target = self.resolve_path(path, analysis)
            if not target or not target.expression: return failure("missing-input", f"Reference {path} is unresolved")
            if self.stack: self.dependencies.add((self.stack[-1], target.element_id))
            return self.evaluate_element(target, analysis)
        if kind == "call": return self.invoke(node["callee"], node.get("arguments", []), analysis)
        if kind == "unary":
            value = self.evaluate_ast(node["operand"], analysis, locals_, local_resolver)
            if value.valid and node["operator"] == "-": value.si = -value.si
            return value
        if kind == "binary":
            left = self.evaluate_ast(node["left"], analysis, locals_, local_resolver)
            if not left.valid: return left
            right = self.evaluate_ast(node["right"], analysis, locals_, local_resolver)
            if not right.valid: return right
            op = node["operator"]; sources = left.source_ids | right.source_ids
            if op in {"+", "-"}:
                if left.dims != right.dims: return failure("unit-error", "Addition or subtraction requires matching dimensions")
                return Quantity(si=left.si + right.si if op == "+" else left.si - right.si, dims=dict(left.dims), unit=left.unit or right.unit, source_ids=sources)
            if op == "*":
                dims = combined(left.dims, right.dims, 1); unit = ""
                if {left.unit, right.unit} == {"V", "A"}: unit = "W"
                elif {left.unit, right.unit} == {"V", "Ah"}: unit = "Wh"
                elif not left.dims: unit = right.unit
                elif not right.dims: unit = left.unit
                return Quantity(si=left.si * right.si, dims=dims, unit=unit, source_ids=sources)
            if op == "/":
                if abs(right.si) < 1e-15: return failure("runtime-error", "Division by zero")
                dims = combined(left.dims, right.dims, -1); unit = "h" if left.unit == "Wh" and right.unit == "W" else ""
                return Quantity(si=left.si / right.si, dims=dims, unit=unit, source_ids=sources)
            if op in {"<", "<=", ">", ">=", "==", "!="}:
                if left.dims != right.dims: return failure("unit-error", "Comparison requires matching dimensions")
                verdict = {"<": left.si < right.si, "<=": left.si <= right.si, ">": left.si > right.si, ">=": left.si >= right.si, "==": left.si == right.si, "!=": left.si != right.si}[op]
                return Quantity(boolean=True, bool_value=verdict, source_ids=sources)
        return failure("unsupported", f"Unsupported expression kind {kind}")

    def invoke(self, name: str, arguments: list[dict[str, Any]], analysis: Element | None) -> Quantity:
        definition = next((x for x in self.by_name.get(name.split("::")[-1], []) if x.kind == "CalculationDefinition"), None)
        if not definition: return failure("unsupported", f"Calculation {name} is unresolved")
        inputs = [x for x in definition.children if x.properties.get("role") == "input"]
        outputs = [x for x in definition.children if x.properties.get("role") == "output"]
        locals_elements = [x for x in definition.children if x.kind == "AttributeUsage"]
        if len(inputs) != len(arguments): return failure("missing-input", "Calculation argument count does not match definition")
        locals_: dict[str, Quantity] = {}
        for formal, argument in zip(inputs, arguments):
            value = self.evaluate_ast(argument, analysis, {})
            if not value.valid: return value
            expected = DIMENSIONS.get(formal.type_name, {})
            if expected and value.dims != expected: return failure("unit-error", f"Argument {formal.name} has incompatible dimension")
            locals_[formal.name] = value
        local_by_name = {x.name: x for x in locals_elements}; visiting: set[str] = set()
        def resolve_local(name_: str) -> Quantity:
            if name_ in locals_: return locals_[name_]
            item = local_by_name.get(name_)
            if not item: return failure("missing-input", f"Reference {name_} is unresolved")
            if name_ in visiting: return failure("cycle", "Circular local calculation dependency")
            visiting.add(name_); value = self.evaluate_ast(expression_ast(item.expression), analysis, locals_, resolve_local); visiting.remove(name_)
            locals_[name_] = value; return value
        for local in locals_elements: resolve_local(local.name)
        if not outputs: return failure("unsupported", "Calculation definition has no return parameter")
        result = self.evaluate_ast(expression_ast(outputs[0].expression), analysis, locals_, resolve_local)
        expected = DIMENSIONS.get(outputs[0].type_name, {})
        if result.valid and expected and result.dims != expected: return failure("unit-error", "Calculation return has incompatible dimension")
        return result

    def display(self, value: Quantity, unit: str | None = None) -> float:
        selected = unit if unit is not None else value.unit
        factor = UNITS.get(selected or "", (1.0, {}))[0]
        return value.si / factor

    def result(self, element: Element, value: Quantity) -> dict[str, Any]:
        result: dict[str, Any] = {"parameterId": element.element_id, "name": element.name, "typeName": element.type_name, "status": "computed" if value.valid else value.status, "formula": element.expression, "formulaAst": expression_ast(element.expression), "dimension": dimension_text(value.dims), "source": self.element_source(element), "inputParameterIds": sorted(value.source_ids)}
        if value.valid and value.boolean: result["value"] = value.bool_value
        elif value.valid:
            result.update({"value": self.display(value), "unit": value.unit or None, "siValue": value.si})
        else: result["message"] = value.message
        return result

    def element_source(self, element: Element) -> Any:
        return next((x["source"] for x in self.snapshot["elements"] if x["id"] == element.element_id), None)

    def constraint(self, item: Element, analysis: Element) -> dict[str, Any]:
        ast = expression_ast(item.expression); verdict = self.evaluate_ast(ast, analysis, {})
        result: dict[str, Any] = {"constraintId": item.element_id, "name": item.name, "expression": item.expression, "expressionAst": ast, "status": ("pass" if verdict.bool_value else "fail") if verdict.valid and verdict.boolean else verdict.status, "source": self.element_source(item), "inputParameterIds": sorted(verdict.source_ids)}
        if ast.get("kind") == "binary":
            left = self.evaluate_ast(ast["left"], analysis, {}); right = self.evaluate_ast(ast["right"], analysis, {})
            if left.valid and right.valid and left.dims == right.dims:
                unit = left.unit or right.unit; op = ast["operator"]
                result.update({"actual": self.display(left, unit), "target": self.display(right, unit), "unit": unit or None, "comparison": op, "margin": self.display(Quantity(si=(right.si-left.si if op in {"<", "<="} else left.si-right.si), dims=left.dims, unit=unit), unit)})
        if not verdict.valid: result["message"] = verdict.message
        return result

    def run(self) -> dict[str, Any]:
        analyses = []
        for analysis in (x for x in self.records if x.kind == "AnalysisCase"):
            self.analysis_results = {x.name: x for x in analysis.children if x.kind == "AttributeUsage"}
            results = [self.result(item, self.evaluate_element(item, analysis)) for item in self.analysis_results.values()]
            constraints = [self.constraint(item, analysis) for item in analysis.children if item.kind == "ConstraintStatement"]
            subject_ref = analysis.properties.get("subjectReference", ""); subject = self.resolve_path(subject_ref, analysis)
            ids = {x.element_id for x in self.analysis_results.values()}
            analyses.append({"analysisId": analysis.element_id, "name": analysis.name, "subjectId": subject.element_id if subject else None, "subjectReference": subject_ref, "results": results, "constraints": constraints, "evaluationOrder": [x for x in self.order if x in ids], "dependencies": [{"sourceParameterId": a, "targetParameterId": b} for a, b in sorted(self.dependencies) if a in ids]})
        return {"schemaVersion": SCHEMA_VERSION, "document": self.snapshot["document"], "engine": {"name": "AIDC Unit-aware Calculation Engine", "version": "1.0.0"}, "analyses": analyses, "diagnostics": self.diagnostics, "capabilities": {"scalar-arithmetic": "supported", "unit-conversion": "supported", "dimension-check": "supported", "dependency-cycle": "supported", "constraint-margin": "supported", "advanced-functions": "unsupported"}}


def analyze(uri: str, text: str) -> dict[str, Any]:
    snapshot, records = make_snapshot(uri, text)
    result = Evaluator(snapshot, records).run()
    result["parseDiagnostics"] = snapshot["diagnostics"]
    return result
