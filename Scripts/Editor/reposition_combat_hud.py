"""Move only the two existing HUD containers, preserving all authored children.

Run with the interactive editor closed using UnrealEditor-Cmd -ExecutePythonScript.
No map, graph, font, style, widget size or data binding is replaced.
"""

from datetime import datetime
from pathlib import Path
import shutil

import unreal


HUD = "/Game/Aesir/UI/HealthBars/WBP_CombatHUD"


def reposition_combat_hud():
    project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
    blueprint = unreal.load_asset(HUD)
    if blueprint is None:
        raise RuntimeError(f"Missing existing HUD: {HUD}")
    backup = project / "Saved/HUD/BeforeReposition" / datetime.now().strftime("%Y%m%d-%H%M%S-%f")
    backup.mkdir(parents=True)
    source = project / "Content/Aesir/UI/HealthBars/WBP_CombatHUD.uasset"
    for extension in (".uasset", ".uexp", ".ubulk"):
        part = source.with_suffix(extension)
        if part.is_file():
            shutil.copy2(part, backup / part.name)
    # WidgetTree is not exposed for Python editing in this UE build. The native
    # helper edits only the two existing slots and validates Blueprint compilation.
    if not unreal.AesirHUDEditorLibrary.reposition_combat_hud():
        raise RuntimeError(f"HUD placement failed; original package backup: {backup}")
    unreal.log(f"COMBAT_HUD_REPOSITION_PASS: PlayerVitals bottom left; BossPanel top center; backup={backup}")


if __name__ == "__main__":
    reposition_combat_hud()
