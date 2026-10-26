"""Upgrade the existing editable combat HUD in place and import six local icons.

Run after a full AesirWardenEditor build, with the interactive editor closed:

    UnrealEditor-Cmd.exe AesirWarden.uproject /Engine/Maps/Entry \
        -EnablePlugins=PythonScriptPlugin -ExecutePythonScript=".../setup_combat_hud.py" \
        -unattended -NullRHI -nosound

The first run migrates WBP_CombatHUD and replaces its old Designer tree while
retaining event/function graphs and PB_PlayerHealth's variable GUID. Subsequent
runs preserve the Designer layout. To explicitly reinstall the default layout,
import this module in the editor's Python console and call:

    setup_combat_hud.setup_combat_hud(rebuild_layout=True)

Every invocation backs up the original HUD and Player Blueprint packages before
any import or migration. It never edits a map, creates a second HUD, or saves the
Player Blueprint. Native migration compiles both Blueprints before saving the HUD.
"""

from datetime import datetime
import hashlib
import json
from pathlib import Path
import shutil

import unreal


PROJECT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
HUD = "/Game/Aesir/UI/HealthBars/WBP_CombatHUD"
PLAYER = "/Game/Aesir/Characters/Player/BP_AesirPlayerCharacter"
NATIVE_HUD = "/Script/AesirWarden.AesirCombatHUDWidget"
ICON_SOURCE = PROJECT / "SourceArt/UI/Emberhold/emberhold-free-v1.0/emberhold-free/icons/png/white/128px"
ICON_DESTINATION = "/Game/Aesir/UI/Art/Emberhold"
ICONS = ("heart", "shield", "fire", "potion", "sword", "lightning")
LOG_PREFIX = "[Aesir Combat HUD]"


def _package_file(asset_path):
    return PROJECT / "Content" / (asset_path.removeprefix("/Game/") + ".uasset")


def _sha256(filename):
    return hashlib.sha256(filename.read_bytes()).hexdigest()


def _player_references_hud(assets):
    # Package dependencies include generated-class references used by CreateWidget.
    # Loading only the Blueprint object to confirm memory references can omit these.
    referencers = assets.find_package_referencers_for_asset(HUD, False)
    return any(str(path).split(".", 1)[0] == PLAYER for path in referencers)


def _backup_packages():
    backup = PROJECT / "Saved/HUD/BeforeInstall" / datetime.now().strftime("%Y%m%d-%H%M%S-%f")
    backup.mkdir(parents=True)
    files = []
    for asset_path in (HUD, PLAYER):
        source = _package_file(asset_path)
        if not source.is_file():
            raise RuntimeError(f"{LOG_PREFIX} Missing on-disk package: {source}")
        for extension in (".uasset", ".uexp", ".ubulk"):
            part = source.with_suffix(extension)
            if part.is_file():
                shutil.copy2(part, backup / part.name)
                files.append({"source": str(part), "backup": part.name, "sha256": _sha256(part)})
    (backup / "manifest.json").write_text(
        json.dumps({"packages": files}, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    return backup


def _import_icons(assets):
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    imported = []
    settings = {
        # UI (RGBA) keeps the white glyph's soft transparent edge, with no block compression.
        "compression_settings": unreal.TextureCompressionSettings.TC_EDITOR_ICON,
        "lod_group": unreal.TextureGroup.TEXTUREGROUP_UI,
        "mip_gen_settings": unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS,
        "compression_no_alpha": False,
        "srgb": True,
        "never_stream": True,
    }
    for name in ICONS:
        asset_name = f"T_EH_{name}"
        asset_path = f"{ICON_DESTINATION}/{asset_name}"
        created = not assets.does_asset_exist(asset_path)
        if created:
            task = unreal.AssetImportTask()
            task.set_editor_property("filename", str(ICON_SOURCE / f"{name}.png"))
            task.set_editor_property("destination_path", ICON_DESTINATION)
            task.set_editor_property("destination_name", asset_name)
            task.set_editor_property("automated", True)
            task.set_editor_property("replace_existing", False)
            task.set_editor_property("save", False)
            tools.import_asset_tasks([task])
        texture = unreal.load_asset(asset_path)
        if not isinstance(texture, unreal.Texture2D):
            raise RuntimeError(f"{LOG_PREFIX} Expected a Texture2D at {asset_path}; existing assets were not replaced.")
        changed = created
        for key, value in settings.items():
            if texture.get_editor_property(key) != value:
                texture.set_editor_property(key, value)
                changed = True
        if changed and not assets.save_loaded_asset(texture, False):
            raise RuntimeError(f"{LOG_PREFIX} Could not save UI settings for {asset_path}.")
        imported.append(asset_path)
    return imported


def setup_combat_hud(rebuild_layout=False):
    """Back up, import the local licensed glyphs, migrate/compile/save one existing HUD."""
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    library = getattr(unreal, "AesirHUDEditorLibrary", None)
    if library is None or unreal.load_class(None, NATIVE_HUD) is None:
        raise RuntimeError(f"{LOG_PREFIX} Fully build AesirWardenEditor and load its updated module first.")
    for asset_path in (HUD, PLAYER, "/Game/Aesir/UI/Fonts/F_MenuSerif"):
        if not assets.does_asset_exist(asset_path):
            raise RuntimeError(f"{LOG_PREFIX} Required existing asset is missing: {asset_path}")
    for suffix in ("Border/T_KUI_panel_border_002", "DividerFade/T_KUI_divider_fade_003"):
        decoration = "/Game/Aesir/UI/Art/Kenney/FantasyUIBorders/Double/" + suffix
        if not assets.does_asset_exist(decoration):
            raise RuntimeError(f"{LOG_PREFIX} Required transparent Kenney decoration is missing: {decoration}")
    for name in ICONS:
        if not (ICON_SOURCE / f"{name}.png").is_file():
            raise RuntimeError(f"{LOG_PREFIX} Missing licensed local icon: {ICON_SOURCE / (name + '.png')}")
    if not _player_references_hud(assets):
        raise RuntimeError(f"{LOG_PREFIX} Player Blueprint does not reference the existing HUD; review CreateWidget before migrating.")

    backup = _backup_packages()
    original_player_hash = _sha256(_package_file(PLAYER))
    try:
        icons = _import_icons(assets)
        if not library.upgrade_combat_hud(bool(rebuild_layout)):
            raise RuntimeError("Native migration/Blueprint compilation failed. Inspect the preceding LogAesirWarden/LogBlueprint messages.")
        native_class = unreal.load_class(None, NATIVE_HUD)
        widget_class = unreal.load_class(None, f"{HUD}.WBP_CombatHUD_C")
        if widget_class is None or not unreal.MathLibrary.class_is_child_of(widget_class, native_class):
            raise RuntimeError("The saved HUD does not use AesirCombatHUDWidget as its native parent.")
        if not _player_references_hud(assets):
            raise RuntimeError("Player Blueprint no longer references the existing HUD package.")
        if _sha256(_package_file(PLAYER)) != original_player_hash:
            raise RuntimeError("The Player Blueprint changed on disk unexpectedly; compare it with the backup before continuing.")
    except Exception:
        unreal.log_error(f"{LOG_PREFIX} Setup stopped. Original packages are backed up at {backup}. Close the editor before restoring package files if needed.")
        raise

    report = {
        "hud": HUD,
        "native_parent": NATIVE_HUD,
        "backup": str(backup),
        "explicit_layout_rebuild": bool(rebuild_layout),
        "blueprints_compiled_by_native_library": [HUD, PLAYER],
        "player_blueprint_reference_verified": True,
        "player_blueprint_saved": False,
        "icons": icons,
        "texture_settings": "UI RGBA; no mipmaps; sRGB; never stream; alpha retained",
        "design_size": [1920, 1080],
        "required_controls": [
            "PB_PlayerHealth", "PB_PlayerHealthTrail", "PB_GuardPressure",
            "Txt_PlayerHealth", "Txt_GuardState", "BossPanel", "PB_BossHealth",
            "Txt_BossName", "HB_Abilities",
        ],
        "background": "transparent outer panels; local bar tracks use black alpha 0.35",
        "ability_slots": "hidden until implemented abilities are configured in Class Defaults",
    }
    report_directory = PROJECT / "Saved/HUD/Install"
    report_directory.mkdir(parents=True, exist_ok=True)
    (report_directory / "manifest.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.log(f"COMBAT_HUD_SETUP_PASS: {HUD}; backup: {backup}; Player Blueprint reference retained")
    return report


if __name__ == "__main__":
    setup_combat_hud()
