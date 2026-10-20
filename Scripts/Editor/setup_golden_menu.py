"""Explicitly rebuild the editable campfire menu at the golden-section anchors.

Run with UnrealEditor-Cmd /Engine/Maps/Entry -EnablePlugins=PythonScriptPlugin
-ExecutePythonScript=".../setup_golden_menu.py" -NullRHI while the interactive
editor is closed. Font import needs an initialized Slate application, so do not
use the -run=pythonscript commandlet for this script.
This replaces the Designer tree, preserving a timestamped backup first. Normal
startup and setup_start_menu.py continue to preserve hand-edited Designer trees.
"""

from datetime import datetime
import json
from pathlib import Path
import shutil

import unreal


PROJECT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
FONT_SOURCE = PROJECT / "SourceArt/UI/Fonts/SourceHanSerif/SourceHanSerifSC-Regular.otf"
FONT_FACE = "/Game/Aesir/UI/Fonts/FF_SourceHanSerifSC_Regular"
FONT = "/Game/Aesir/UI/Fonts/F_MenuSerif"
MENU = "/Game/Aesir/UI/Menus/WBP_CampfireMainMenu"
LOGO = "/Game/Aesir/UI/Art/Logo/logo"


def setup_golden_menu():
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    if not FONT_SOURCE.is_file():
        raise RuntimeError(f"Missing licensed font source: {FONT_SOURCE}")
    if not assets.does_asset_exist(MENU):
        raise RuntimeError("Create the existing campfire menu before replacing its layout.")
    logo = unreal.load_asset(LOGO)
    if not isinstance(logo, unreal.Texture2D):
        raise RuntimeError(f"Import the transparent title logo first: {LOGO}")
    for suffix in ("Border/T_KUI_panel_border_002", "DividerFade/T_KUI_divider_fade_003"):
        if not assets.does_asset_exist("/Game/Aesir/UI/Art/Kenney/FantasyUIBorders/Double/" + suffix):
            raise RuntimeError(f"Required Kenney decoration is missing: {suffix}")

    backup = PROJECT / "Saved/StartMenu/BeforeGoldenLayout" / datetime.now().strftime("%Y%m%d-%H%M%S-%f")
    backup.mkdir(parents=True)
    for relative in ("Content/Aesir/UI/Menus/WBP_CampfireMainMenu.uasset",
                     "Content/Aesir/UI/Art/Logo/logo.uasset"):
        shutil.copy2(PROJECT / relative, backup / Path(relative).name)

    if not assets.does_asset_exist(FONT_FACE):
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(FONT_SOURCE))
        task.set_editor_property("destination_path", "/Game/Aesir/UI/Fonts")
        task.set_editor_property("destination_name", "FF_SourceHanSerifSC_Regular")
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", False)
        task.set_editor_property("save", True)
        task.set_editor_property("factory", unreal.FontFileImportFactory())
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    if not isinstance(unreal.load_asset(FONT_FACE), unreal.FontFace):
        raise RuntimeError(f"Expected a static runtime Font Face: {FONT_FACE}")

    for key, value in {
        "compression_settings": unreal.TextureCompressionSettings.TC_EDITOR_ICON,
        "lod_group": unreal.TextureGroup.TEXTUREGROUP_UI,
        "mip_gen_settings": unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS,
        "compression_no_alpha": False,
        "srgb": True,
        "never_stream": True,
    }.items():
        if logo.get_editor_property(key) != value:
            logo.set_editor_property(key, value)
    if not assets.save_loaded_asset(logo):
        raise RuntimeError("Could not save UI logo settings.")

    if not unreal.AesirStartMenuEditorLibrary.rebuild_campfire_menu_layout():
        raise RuntimeError("Could not compile and save the golden-section Widget Blueprint.")
    if not isinstance(unreal.load_asset(FONT), unreal.Font):
        raise RuntimeError("The menu composite Font was not saved.")

    report = {
        "menu": MENU, "font": FONT, "font_face": FONT_FACE, "logo": LOGO,
        "backup": str(backup), "anchor": {"x": 0.618034, "y": 0.381966},
        "button_font_size": 55, "button_labels": ["开始游戏", "设置", "制作人员", "退出游戏"],
        "background": "transparent; only logo, border textures and divider textures paint",
        "settings_and_credits": "editable placeholder pages with Back/Escape handling",
    }
    report_dir = PROJECT / "Saved/StartMenu/GoldenLayout"
    report_dir.mkdir(parents=True, exist_ok=True)
    (report_dir / "manifest.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.log(f"GOLDEN_MENU_SETUP_PASS: {MENU}; backup: {backup}")
    return report


if __name__ == "__main__":
    setup_golden_menu()
