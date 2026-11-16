"""Build and archive the Development Windows BT baseline; runtime acceptance is separate."""

import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import subprocess
import time


ROOT = Path(__file__).resolve().parents[2]


def utc_now():
    return datetime.now(timezone.utc).isoformat()


def git_output(root, *arguments):
    return subprocess.check_output(
        ["git", "-C", str(root), *arguments], encoding="utf-8", errors="replace"
    ).rstrip("\r\n")


def windows_batch_command(arguments):
    # RunUAT reparses arguments in batch files; reject cmd metacharacters.
    if any(any(c in argument for c in '"%&|<>^!\r\n') for argument in arguments):
        raise ValueError('Batch arguments cannot contain cmd metacharacters or newlines.')
    command = " ".join(f'"{argument}"' for argument in arguments)
    executable = Path(os.environ.get("SystemRoot", "C:/Windows")) / "System32/cmd.exe"
    return f'"{executable}" /d /v:off /s /c "{command}"'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path, default=Path("D:/Program Files/Epic/UE_5.8"))
    parser.add_argument("--project", type=Path, default=ROOT / "AesirWarden.uproject")
    parser.add_argument("--output", type=Path, help="Archive directory; defaults to Saved/Builds/WindowsBT")
    parser.add_argument("--map", default="/Game/Main", help="Unreal package path to cook")
    parser.add_argument("--iterate", action="store_true", help="Reuse compatible cooked data from a prior local build")
    parser.add_argument("--dry-run", action="store_true", help="Print the plan without running UAT or writing files")
    args = parser.parse_args()

    project = args.project.resolve()
    engine = args.engine.resolve()
    root = project.parent
    output = (args.output or root / "Saved/Builds/WindowsBT").resolve()
    uat = engine / "Engine/Build/BatchFiles/RunUAT.bat"
    version = engine / "Engine/Build/Build.version"
    for path in (project, uat, version):
        if not path.is_file():
            parser.error(f"Required file does not exist: {path}")
    if not args.map.startswith("/Game/"):
        parser.error("--map must be an Unreal /Game/ package path")
    if os.name != "nt" and not args.dry_run:
        parser.error("Running RunUAT.bat requires Windows")

    run_id = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    evidence = root / "Saved/Verification/Windows" / run_id
    log = evidence / "BuildCookRun.log"
    result_file = evidence / "summary.json"
    arguments = [
        str(uat), "BuildCookRun", f"-project={project}", "-platform=Win64",
        "-clientconfig=Development", f"-map={args.map}", "-build", "-cook", "-stage",
        "-pak", "-iostore", "-archive", f"-archivedirectory={output}", "-compressed",
        "-prereqs", "-nodebuginfo", "-noP4", "-unattended", "-utf8output",
    ]
    if args.iterate:
        arguments.append("-iterate")
    try:
        command = windows_batch_command(arguments)
        report = {
            "status": "planned", "started_utc": utc_now(), "finished_utc": None,
            "project": str(project), "map": args.map, "configuration": "Development",
            "platform": "Win64", "output": str(output), "log": str(log),
            "uat_argv": arguments, "command": command,
            "git_commit": git_output(root, "rev-parse", "HEAD"),
            "git_dirty": git_output(root, "status", "--porcelain=v1", "--untracked-files=all").splitlines(),
            "engine_version": json.loads(version.read_text(encoding="utf-8-sig")),
            "exit_code": None, "build_cook_run_succeeded": False,
            "cold_start_verified": False,
        }
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.error(str(error))

    if args.dry_run:
        print(json.dumps(report, ensure_ascii=False, indent=2))
        return 0

    evidence.mkdir(parents=True, exist_ok=False)
    report["status"] = "running"
    result_file.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"BuildCookRun log: {log}", flush=True)
    print(command, flush=True)
    started = time.monotonic()
    try:
        with log.open("wb") as console:
            completed = subprocess.run(
                command, cwd=root, stdout=console, stderr=subprocess.STDOUT,
                creationflags=subprocess.CREATE_NO_WINDOW,
            )
        report["exit_code"] = completed.returncode
        report["build_cook_run_succeeded"] = completed.returncode == 0
        report["status"] = "build_succeeded" if completed.returncode == 0 else "build_failed"
    except OSError as error:
        report.update(status="launch_failed", error=str(error))
    finally:
        report["finished_utc"] = utc_now()
        report["duration_seconds"] = round(time.monotonic() - started, 3)
        result_file.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        print(f"{report['status']}: {result_file}", flush=True)
    return report["exit_code"] if report["exit_code"] is not None else 1


if __name__ == "__main__":
    raise SystemExit(main())
