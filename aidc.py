import sys

from cli.aidc import main

if __name__ == "__main__":
    if hasattr(sys.stdout, "reconfigure"): sys.stdout.reconfigure(encoding="utf-8")
    if hasattr(sys.stderr, "reconfigure"): sys.stderr.reconfigure(encoding="utf-8")
    if len(sys.argv) > 1 and sys.argv[1] == "__gateway":
        from aidc_core.gateway import main as gateway_main
        sys.argv.pop(1)
        raise SystemExit(gateway_main())
    if len(sys.argv) > 1 and sys.argv[1] == "__menu":
        from cli.launcher import main as launcher_main
        raise SystemExit(launcher_main())
    raise SystemExit(main())
