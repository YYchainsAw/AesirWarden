"""Verify Main result flows in a cooked Windows Development package."""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[2]


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package", type=Path, default=ROOT / "Saved/Builds/WindowsBT",
                        help="Staged Windows directory containing AesirWarden/Binaries/Win64.")
    parser.add_argument("--only", choices=("Victory", "Defeat"))
    args = parser.parse_args()
    package = args.package.resolve()
    executable = package / "AesirWarden/Binaries/Win64/AesirWarden.exe"
    paks = package / "AesirWarden/Content/Paks"
    if not executable.is_file():
        parser.error(f"Packaged game executable not found: {executable}")
    if not paks.is_dir() or not any(paks.iterdir()):
        parser.error(f"Packaged Content/Paks directory is missing or empty: {paks}")

    output = ROOT / "Saved/Verification/WindowsRuntime" / datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    output.mkdir(parents=True, exist_ok=False)
    env = os.environ.copy()
    env.update(AESIR_NPC_BASE_URL="http://127.0.0.1:9", AESIR_NPC_SAVE_ID="disabled-baseline")
    env.pop("AESIR_NPC_BEARER_TOKEN", None)
    report = {
        "package": str(package),
        "executable": str(executable),
        "exe_sha256": sha256(executable),
        "scope": {
            "runtime": "cooked Windows Development game",
            "death_trigger": "GAS test health adjustment",
            "boss_entry": "native encounter entry",
            "full_manual_combat": False,
        },
        "cases": {},
    }
    for case in ((args.only,) if args.only else ("Victory", "Defeat")):
        log = output / f"{case}.log"
        command = [str(executable), "/Game/Main", "-unattended", "-nop4", "-nosound", "-NoSplash",
                   "-RenderOffscreen", "-windowed", "-ForceRes", "-ResX=1920", "-ResY=1080",
                   f"-AbsLog={log}", f"-ExecCmds=DisableAllScreenMessages,Aesir.Match.Smoke {case}"]
        exit_code = None
        error = None
        try:
            with (output / f"{case}.console.log").open("wb") as console:
                exit_code = subprocess.run(command, cwd=package, env=env, stdout=console,
                                           stderr=subprocess.STDOUT, timeout=300,
                                           creationflags=subprocess.CREATE_NO_WINDOW).returncode
        except subprocess.TimeoutExpired:
            error = "Game process exceeded the 300-second timeout."
        except OSError as exc:
            error = str(exc)
        text = log.read_text(encoding="utf-8-sig", errors="replace") if log.is_file() else ""
        result = {
            "command": command,
            "exit_code": exit_code,
            "error": error,
            "log": str(log),
            "log_sha256": sha256(log) if log.is_file() else None,
            "markers": [line for line in text.splitlines() if "MATCH_MAIN_SMOKE_" in line],
            "passed": (exit_code == 0 and f"MATCH_MAIN_SMOKE_PASS: case={case}" in text
                       and "MATCH_MAIN_SMOKE_FAIL" not in text),
        }
        report["cases"][case] = result
        print(f"{case}: {'PASS' if result['passed'] else 'FAIL'} {log}", flush=True)
    report["passed"] = all(case["passed"] for case in report["cases"].values())
    result_file = output / "summary.json"
    result_file.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Report: {result_file}", flush=True)
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
