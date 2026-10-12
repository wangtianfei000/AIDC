"""Publish the verified detailed-assembly preview beside the P4 model."""

from pathlib import Path
import shutil


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
SOURCE = ROOT / "Jingang/workspace/hd1910_updated_assembly.png"
OUTPUT = HERE / "jingang-10axis-layout-preview.png"


if __name__ == "__main__":
    if not SOURCE.exists():
        raise FileNotFoundError(SOURCE)
    shutil.copyfile(SOURCE, OUTPUT)
    print(OUTPUT)
