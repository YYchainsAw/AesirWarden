"""Verify match rules and both Main result pages using the local Editor build."""

import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path, default=Path("D:/Program Files/Epic/UE_5.8"))
    parser.add_argument("--only", choices=("rules", "Victory", "Defeat"))
    args = parser.parse_args()
    binaries = args.engine / "Engine/Binaries/Win64"
    output = ROOT / "Saved/Verification/Match" / datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    output.mkdir(parents=True, exist_ok=False)
    env = os.environ.copy()
    env.update(AESIR_NPC_BASE_URL="http://127.0.0.1:9", AESIR_NPC_SAVE_ID="disabled-baseline")
    env.pop("AESIR_NPC_BEARER_TOKEN", None)
    report = {}
    marker = "MATCH_MAIN_SMOKE_PASS:"
    for case in ((args.only,) if args.only else ("rules", "Victory", "Defeat")):
        is_rules = case == "rules"
        executable = binaries / ("UnrealEditor-Cmd.exe" if is_rules else "UnrealEditor.exe")
        log = output / f"{case}.log"
        command = [str(executable), str(ROOT / "AesirWarden.uproject"),
                   "/Engine/Maps/Entry" if is_rules else "/Game/Main",
                   "-unattended", "-nop4", "-nosound", "-NoLiveCoding", "-NoSplash", f"-AbsLog={log}"]
        if is_rules:
            command += ["-NullRHI", "-ExecCmds=Automation RunTests Aesir.Match",
                        "-TestExit=Automation Test Queue Empty", f"-ReportExportPath={output / 'Automation'}"]
        else:
            command += ["-game", "-RenderOffscreen", "-windowed", "-ForceRes", "-ResX=1920", "-ResY=1080",
                        f"-ExecCmds=DisableAllScreenMessages,Aesir.Match.Smoke {case}"]
        try:
            with (output / f"{case}.console.log").open("wb") as console:
                exit_code = subprocess.run(command, cwd=ROOT, env=env, stdout=console,
                                           stderr=subprocess.STDOUT, timeout=300,
                                           creationflags=subprocess.CREATE_NO_WINDOW).returncode
        except subprocess.TimeoutExpired:
            exit_code = None
        text = log.read_text(encoding="utf-8-sig", errors="replace") if log.exists() else ""
        result = {"command": command, "exit_code": exit_code, "log": str(log), "passed": False}
        if is_rules:
            index = output / "Automation/index.json"
            if index.is_file():
                automation = json.loads(index.read_text(encoding="utf-8-sig"))
                result["tests"] = [{k: test.get(k) for k in ("fullTestPath", "state", "warnings", "errors")}
                                   for test in automation.get("tests", [])]
                result["passed"] = (exit_code == 0 and bool(result["tests"])
                                    and all(t["state"] == "Success" for t in result["tests"]))
        else:
            result["markers"] = [line for line in text.splitlines() if "MATCH_MAIN_SMOKE_" in line]
            result["passed"] = exit_code == 0 and f"{marker} case={case}" in text and "MATCH_MAIN_SMOKE_FAIL" not in text
        report[case] = result
        print(f"{case}: {'PASS' if result['passed'] else 'FAIL'} {log}", flush=True)
    result_file = output / "summary.json"
    result_file.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return 0 if all(case["passed"] for case in report.values()) else 1


if __name__ == "__main__":
    raise SystemExit(main())
