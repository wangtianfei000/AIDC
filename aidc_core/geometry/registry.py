from __future__ import annotations

from pathlib import Path

from aidc_core.paths import product_root

from .freecad_occ import FreeCadOccBackend


class GeometryBackendRegistry:
    """Resolve the `geometry.backend` product setting to an implementation."""

    setting = "geometry.backend"
    default_backend = "freecad-occ"

    def __init__(self, root: Path | None = None):
        self.root = (root or product_root()).resolve()
        self._types = {"freecad-occ": FreeCadOccBackend}

    def get(self, backend_id: str | None = None):
        selected = backend_id or self.default_backend
        try:
            return self._types[selected](self.root)
        except KeyError as error:
            raise ValueError(f"unknown geometry backend: {selected}") from error

    def list(self) -> list[dict]:
        values = []
        for backend_id in self._types:
            backend = self.get(backend_id)
            values.append({**backend.descriptor.to_dict(), "selectedByDefault": backend_id == self.default_backend, "probe": backend.probe()})
        return values

