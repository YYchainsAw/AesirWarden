"""Assign the Main encounter objective and save only its external actor package."""

from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import shutil

import unreal


ACTOR = "/Game/Main.Main:PersistentLevel.BP_AesirBoss_C_UAID_B025AA7DC8C06C0F03_1654060213"
PACKAGE = "/Game/__ExternalActors__/Main/A/2N/ORCIBKVER8BZVS28A66K14"
TAG = "Aesir.MatchBoss"


def setup_main_match_boss():
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.load_level("/Game/Main"):
        raise RuntimeError("Cannot load Main.")

    descriptors = unreal.WorldPartitionBlueprintLibrary.get_actor_descs()
    boss_descriptors = [d for d in descriptors
                        if "AesirEnemyCharacter" in str(d.get_editor_property("native_class"))]
    unreal.WorldPartitionBlueprintLibrary.load_actors(
        [d.get_editor_property("guid") for d in boss_descriptors])
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    boss = next((actor for actor in actors if actor.get_path_name() == ACTOR), None)
    conflicts = [a.get_path_name() for a in actors if a != boss and a.actor_has_tag(TAG)]
    if boss is None or conflicts:
        raise RuntimeError(f"Expected the verified Main objective and no conflicting tags. Conflicts: {conflicts}")

    project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
    source = project / "Content" / (PACKAGE.removeprefix("/Game/") + ".uasset")
    timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    output = project / "Saved/Verification/Match/MainObjective" / timestamp
    output.mkdir(parents=True, exist_ok=False)
    before = hashlib.sha256(source.read_bytes()).hexdigest()
    backup = output / source.name
    shutil.copy2(source, backup)
    if hashlib.sha256(backup.read_bytes()).hexdigest() != before:
        raise RuntimeError("Main objective backup checksum mismatch.")

    if not unreal.AesirMatchResultEditorLibrary.save_match_boss_tag(boss, PACKAGE):
        raise RuntimeError(f"Unable to save the objective actor. Backup: {backup}")
    result = {
        "actor": ACTOR, "actor_label": boss.get_actor_label(), "tag": TAG,
        "package": PACKAGE, "backup": str(backup), "before_sha256": before,
        "after_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
    }
    report = output / "setup-result.json"
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    unreal.log(f"MATCH_MAIN_OBJECTIVE_PASS {report}")


if __name__ == "__main__":
    setup_main_match_boss()
