"""Create/verify the separate editable settings menu; preserve existing user layouts.

Run after a full AesirWardenEditor build, with the interactive editor closed:
UnrealEditor-Cmd.exe AesirWarden.uproject /Engine/Maps/Entry
-EnablePlugins=PythonScriptPlugin -ExecutePythonScript=".../setup_settings_menu.py"
-unattended -NullRHI -nosound

Only WBP_SettingsMenu is created or metadata-repaired. No map, main-menu Blueprint,
player Blueprint, third-party source file or texture is imported or changed.
"""

from datetime import datetime
import hashlib
import json
from pathlib import Path
import shutil

import unreal


PROJECT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
ASSET = "/Game/Aesir/UI/Menus/WBP_SettingsMenu"
NATIVE = "/Script/AesirWarden.AesirSettingsMenuWidget"
REPORT = PROJECT / "Saved/UI/SettingsMenu"
DEPENDENCIES = (
    "/Game/Aesir/UI/Fonts/F_MenuSerif",
    "/Game/Aesir/UI/Art/Kenney/FantasyUIBorders/Double/Border/T_KUI_panel_border_002",
    "/Game/Aesir/UI/Art/Kenney/FantasyUIBorders/Double/DividerFade/T_KUI_divider_fade_003",
)


def setup_settings_menu():
    """Create once, then validate controls without replacing the authored WidgetTree."""
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    library = getattr(unreal, "AesirSettingsEditorLibrary", None)
    native_class = unreal.load_class(None, NATIVE)
    if library is None or native_class is None:
        raise RuntimeError("Fully build AesirWardenEditor and load its updated module first.")
    for dependency in DEPENDENCIES:
        if not assets.does_asset_exist(dependency):
            raise RuntimeError(f"Required existing settings-menu decoration/font is missing: {dependency}")

    existed = assets.does_asset_exist(ASSET)
    backup = None
    source = PROJECT / "Content/Aesir/UI/Menus/WBP_SettingsMenu.uasset"
    if existed:
        if not source.is_file():
            raise RuntimeError("Save WBP_SettingsMenu before rerunning setup so the disk backup includes your edits.")
        backup = REPORT / "BeforeSetup" / datetime.now().strftime("%Y%m%d-%H%M%S-%f")
        backup.mkdir(parents=True)
        parts = []
        for suffix in (".uasset", ".uexp", ".ubulk"):
            part = source.with_suffix(suffix)
            if part.is_file():
                shutil.copy2(part, backup / part.name)
                parts.append({"source": str(part), "backup": part.name,
                              "sha256": hashlib.sha256(part.read_bytes()).hexdigest()})
        (backup / "manifest.json").write_text(json.dumps({"packages": parts}, ensure_ascii=False, indent=2), encoding="utf-8")

    if not library.create_settings_menu_asset():
        raise RuntimeError("Settings menu creation/validation failed; inspect LogAesirWarden/LogBlueprint. Existing layout was preserved.")
    widget_class = assets.load_blueprint_class(ASSET)
    if not widget_class or not unreal.MathLibrary.class_is_child_of(widget_class, native_class):
        raise RuntimeError("Saved WBP_SettingsMenu has an unexpected parent.")
    defaults = unreal.get_default_object(widget_class)
    if not defaults.get_editor_property("is_focusable"):
        raise RuntimeError("WBP_SettingsMenu must be focusable for keyboard navigation and Escape.")
    report = {
        "asset": ASSET,
        "native_parent": NATIVE,
        "created": not existed,
        "existing_designer_layout_preserved": existed,
        "backup": str(backup) if backup else None,
        "main_menu_modified": False,
        "map_modified": False,
        "textures_imported": 0,
        "design_size": [1920, 1080],
        "surface": {"size": [1280, 760], "anchor": [0.5, 0.5], "alignment": [0.5, 0.5]},
        "appearance": "Transparent panel/button centres, Kenney gold borders, F_MenuSerif; local dropdown popup has a dark background.",
        "categories": ["Display", "Quality", "Audio", "Controls"],
        "blueprint_errors": 0,
    }
    REPORT.mkdir(parents=True, exist_ok=True)
    (REPORT / "asset_setup.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.log(f"SETTINGS_MENU_ASSET_PASS: {ASSET}; report: {REPORT / 'asset_setup.json'}")
    return report


if __name__ == "__main__":
    setup_settings_menu()
