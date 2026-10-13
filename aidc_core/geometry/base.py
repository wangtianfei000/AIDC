from __future__ import annotations

from abc import ABC, abstractmethod
from dataclasses import asdict, dataclass, field
from typing import Any


GEOMETRY_BACKEND_SCHEMA = "aidc.geometry.backend/0.1"
VALIDATION_SCHEMA = "aidc.geometry.validation/0.1"


@dataclass(frozen=True)
class BackendDescriptor:
    id: str
    name: str
    status: str
    process_model: str
    exact_format: str
    capabilities: tuple[str, ...]

    def to_dict(self) -> dict[str, Any]:
        return {
            "id": self.id,
            "name": self.name,
            "status": self.status,
            "processModel": self.process_model,
            "exactFormat": self.exact_format,
            "capabilities": list(self.capabilities),
        }


@dataclass
class CapabilityResult:
    id: str
    passed: bool
    message: str
    metrics: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


class GeometryBackend(ABC):
    """Process-neutral contract for an exact-geometry implementation.

    Backends may run in the current Python process or behind an isolated
    executable. Callers consume descriptors and validation reports rather
    than backend-native BRep objects.
    """

    descriptor: BackendDescriptor

    def __init__(self, product_root):
        self.product_root = product_root

    @abstractmethod
    def probe(self) -> dict[str, Any]:
        """Return availability, version and resolved runtime location."""

    @abstractmethod
    def validate(self) -> dict[str, Any]:
        """Exercise the backend's common capability contract."""

