"""Upgrade the existing result Widget Blueprint after a full Editor build.

Run in Unreal Python with the interactive editor closed. Only WBP_CombatResult
is saved. Original package parts and the unchanged GameMode are backed up first.
No map or project service configuration is changed.
"""

from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import shutil

import unreal


PROJECT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
WIDGET = "/Game/Aesir/UI/Results/WBP_CombatResult"
GAME_MODE = "/Game/Aesir/Framework/BP_AesirGameMode"
NATIVE = "/Script/AesirWarden.AesirCombatResultWidget"


def package_file(asset):
    return PROJECT / "Content" / (asset.removeprefix("/Game/") + ".uasset")


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def setup_match_results():
    library = getattr(unreal, "AesirMatchResultEditorLibrary", None)
    native = unreal.load_class(None, NATIVE)
    if library is None or native is None:
        raise RuntimeError("Build AesirWardenEditor with the match-result classes before setup.")
    timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    output = PROJECT / "Saved/Verification/Match/ResultWidgetSetup" / timestamp
    backup = output / "Before"
    backup.mkdir(parents=True, exist_ok=False)
    parts = []
    for asset in (WIDGET, GAME_MODE):
        source = package_file(asset)
        if not source.is_file():
            raise RuntimeError(f"Required existing package is missing: {source}")
        for suffix in (".uasset", ".uexp", ".ubulk"):
            part = source.with_suffix(suffix)
            if part.is_file():
                destination = backup / part.name
                shutil.copy2(part, destination)
                fingerprint = sha256(part)
                if sha256(destination) != fingerprint:
                    raise RuntimeError("Result-page package backup failed its checksum.")
                parts.append({"source": str(part), "backup": str(destination), "sha256": fingerprint})
    (output / "backup-manifest.json").write_text(json.dumps(parts, indent=2), encoding="utf-8")
    original_mode_hash = sha256(package_file(GAME_MODE))
    report = {"passed": False, "widget": WIDGET, "native_parent": NATIVE,
              "backup_manifest": str(output / "backup-manifest.json"),
              "game_mode_asset_saved": False, "map_saved": False}
    try:
        if not library.upgrade_match_result_widget():
            raise RuntimeError("Native result-page upgrade/Blueprint compilation failed; retain the package backup.")
        widget_class = unreal.load_class(None, WIDGET + ".WBP_CombatResult_C")
        if widget_class is None or not unreal.MathLibrary.class_is_child_of(widget_class, native):
            raise RuntimeError("The upgraded result page does not use the native result-widget parent.")
        if sha256(package_file(GAME_MODE)) != original_mode_hash:
            raise RuntimeError("The GameMode package changed on disk unexpectedly.")
        report.update(passed=True, widget_sha256=sha256(package_file(WIDGET)),
                      game_mode_sha256=original_mode_hash,
                      controls=["TXT_Result", "BTN_Restart", "BTN_ReturnToMenu", "BTN_Quit"],
                      legacy_restart_routing="Replaced at runtime by native same-level match actions")
    finally:
        (output / "setup-result.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    unreal.log(f"MATCH_RESULT_SETUP_PASS {output / 'setup-result.json'}")
    return report


if __name__ == "__main__":
    setup_match_results()
