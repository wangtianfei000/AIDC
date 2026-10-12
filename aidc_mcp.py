import sys

from mcp.aidc_mcp import main

if __name__ == "__main__":
    if hasattr(sys.stdout, "reconfigure"): sys.stdout.reconfigure(encoding="utf-8")
    if hasattr(sys.stderr, "reconfigure"): sys.stderr.reconfigure(encoding="utf-8")
    raise SystemExit(main())
