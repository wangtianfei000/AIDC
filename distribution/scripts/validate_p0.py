from __future__ import annotations

import json
import sys
from pathlib import Path, PurePosixPath

from jsonschema import Draft202012Validator, FormatChecker, SchemaError


ROOT = Path(__file__).resolve().parents[2]
DIST = ROOT / "distribution"
SCHEMAS = DIST / "schemas"


def load(path: Path) -> object:
    with path.open("r", encoding="utf-8") as stream:
        return json.load(stream)


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def validate(instance_path: Path, schema_path: Path, errors: list[str]) -> None:
    schema = load(schema_path)
    instance = load(instance_path)
    if schema_path.name == "cli-result.schema.json":
        schema["properties"]["artifacts"]["items"] = load(SCHEMAS / "artifact-descriptor.schema.json")
    validator = Draft202012Validator(schema, format_checker=FormatChecker())
    for error in sorted(validator.iter_errors(instance), key=lambda item: list(item.absolute_path)):
        location = "/".join(str(part) for part in error.absolute_path) or "$"
        errors.append(f"{instance_path.relative_to(ROOT)}:{location}: {error.message}")


def safe_relative(path: str) -> bool:
    normalized = path.replace("\\", "/")
    candidate = PurePosixPath(normalized)
    return not candidate.is_absolute() and ".." not in candidate.parts and ":" not in normalized


def main() -> int:
    errors: list[str] = []

    for schema_path in sorted(SCHEMAS.glob("*.schema.json")):
        schema = load(schema_path)
        try:
            Draft202012Validator.check_schema(schema)
        except SchemaError as error:
            errors.append(f"Invalid schema {schema_path}: {error.message}")

    validate(DIST / "dependencies.lock.json", SCHEMAS / "dependencies-lock.schema.json", errors)
    validate(DIST / "contracts" / "mcp-tools.json", SCHEMAS / "mcp-tool-catalog.schema.json", errors)
    validate(DIST / "contracts" / "example-cli-result.json", SCHEMAS / "cli-result.schema.json", errors)
    validate(DIST / "contracts" / "cli-contract.json", SCHEMAS / "cli-contract.schema.json", errors)
    validate(DIST / "runtime-packs" / "catalog.json", SCHEMAS / "runtime-catalog.schema.json", errors)
    validate(
        DIST / "contracts" / "project-artifact-types.json",
        SCHEMAS / "project-artifact-catalog.schema.json",
        errors,
    )

    manifests = []
    for manifest_path in sorted((DIST / "manifests").glob("*.component.json")):
        validate(manifest_path, SCHEMAS / "component-manifest.schema.json", errors)
        manifests.append((manifest_path, load(manifest_path)))

    require(bool(manifests), "No component manifests found", errors)
    component_ids = [manifest["id"] for _, manifest in manifests]
    require(len(component_ids) == len(set(component_ids)), "Duplicate component IDs", errors)

    dependency_lock = load(DIST / "dependencies.lock.json")
    dependency_ids = [item["id"] for item in dependency_lock["dependencies"]]
    require(len(dependency_ids) == len(set(dependency_ids)), "Duplicate dependency IDs", errors)
    known_ids = set(component_ids) | set(dependency_ids)

    for path, manifest in manifests:
        for entry in manifest["entryPoints"]:
            require(
                safe_relative(entry["relativePath"]),
                f"Unsafe entry point in {path.relative_to(ROOT)}: {entry['relativePath']}",
                errors,
            )
        for dependency in manifest["dependencies"]:
            require(
                dependency["id"] in known_ids,
                f"Unknown dependency {dependency['id']} in {path.relative_to(ROOT)}",
                errors,
            )

    for dependency in dependency_lock["dependencies"]:
        if dependency["distributionStatus"] == "blocked":
            require(bool(dependency.get("blockers")), f"Blocked dependency {dependency['id']} has no blockers", errors)
        if dependency["source"]["sha256"] is None:
            require(
                dependency["distributionStatus"] == "blocked",
                f"Dependency {dependency['id']} has no SHA-256 but is not blocked",
                errors,
            )
        for entry_path in dependency.get("install", {}).get("entryPoints", {}).values():
            require(safe_relative(entry_path), f"Unsafe dependency entry point: {dependency['id']}:{entry_path}", errors)

    cli = load(DIST / "contracts" / "cli-contract.json")
    command_ids = [command["id"] for command in cli["commands"]]
    require(len(command_ids) == len(set(command_ids)), "Duplicate CLI command IDs", errors)
    exit_codes = [item["code"] for item in cli["exitCodes"]]
    require(len(exit_codes) == len(set(exit_codes)), "Duplicate CLI exit codes", errors)

    mcp = load(DIST / "contracts" / "mcp-tools.json")
    tool_names = [tool["name"] for tool in mcp["tools"]]
    require(len(tool_names) == len(set(tool_names)), "Duplicate MCP tool names", errors)
    for tool in mcp["tools"]:
        require(tool["capability"] in command_ids, f"MCP capability has no CLI command: {tool['capability']}", errors)
        for dependency_id in tool["requiredDependencies"]:
            require(dependency_id in dependency_ids, f"MCP tool {tool['name']} uses unknown dependency {dependency_id}", errors)

    artifacts = load(DIST / "contracts" / "project-artifact-types.json")
    project_ids = [item["id"] for item in artifacts["projectTypes"]]
    artifact_ids = [item["id"] for item in artifacts["artifactTypes"]]
    require(len(project_ids) == len(set(project_ids)), "Duplicate project type IDs", errors)
    require(len(artifact_ids) == len(set(artifact_ids)), "Duplicate artifact type IDs", errors)

    gates = load(DIST / "audits" / "supply-chain-gates.json")
    blocked_gates = [gate for gate in gates["gates"] if gate["status"] == "blocked"]
    require(gates["releaseGate"] == "blocked", "P0 release gate must remain blocked while critical items are open", errors)
    require(bool(blocked_gates), "Supply-chain baseline must enumerate blocked gates", errors)

    sbom = load(DIST / "sbom.cdx.json")
    require(sbom.get("bomFormat") == "CycloneDX", "SBOM must use CycloneDX", errors)
    require(bool(sbom.get("components")), "SBOM has no components", errors)

    if errors:
        print("P0 validation failed:", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1

    print(
        "P0 validation passed: "
        f"{len(manifests)} components, {len(dependency_ids)} dependencies, "
        f"{len(command_ids)} CLI commands, {len(tool_names)} MCP tools, "
        f"{len(blocked_gates)} blocked supply-chain gates recorded."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
