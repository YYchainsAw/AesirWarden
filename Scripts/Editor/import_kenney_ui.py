"""Import the CC0 Kenney Fantasy UI Borders PNGs as UMG-ready textures.

Run in UE's Python console, or with UnrealEditor-Cmd -run=pythonscript
-EnablePlugins=PythonScriptPlugin -script=".../import_kenney_ui.py" -NullRHI.
Pass -KenneyUIVerifyOnly to check saved assets in a separate process.
Existing textures are validated and preserved; no maps or Widgets are edited.
"""

import json
import re
import struct
from pathlib import Path

import unreal


DESTINATION = "/Game/Aesir/UI/Art/Kenney/FantasyUIBorders"
PROJECT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
SOURCE = PROJECT / "SourceArt/UI/Kenney/FantasyUIBorders/Assets/PNG"
REPORT = PROJECT / "Saved/KenneyUIImport"
OWNER_TAG = "Aesir.KenneyUI.Source"
SETTINGS = {
    "compression_settings": unreal.TextureCompressionSettings.TC_EDITOR_ICON,
    "lod_group": unreal.TextureGroup.TEXTUREGROUP_UI,
    "mip_gen_settings": unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS,
    "srgb": True,
    "never_stream": True,
    "filter": unreal.TextureFilter.TF_BILINEAR,
    "compression_no_alpha": False,
}


def _plan():
    files = sorted(SOURCE.rglob("*.png"))
    if len(files) != 280:
        raise RuntimeError(f"Expected the original 280 PNGs in {SOURCE}, found {len(files)}.")
    result = []
    paths = set()
    for source in files:
        relative = source.relative_to(SOURCE)
        variant, category, filename = relative.parts
        folder = category.title().replace(" ", "")
        name = "T_KUI_" + re.sub(r"[^A-Za-z0-9_]", "_", Path(filename).stem)
        destination = f"{DESTINATION}/{variant}/{folder}"
        asset_path = f"{destination}/{name}"
        if asset_path in paths:
            raise RuntimeError(f"Duplicate import destination: {asset_path}")
        paths.add(asset_path)
        with source.open("rb") as stream:
            header = stream.read(26)
        if header[:8] != b"\x89PNG\r\n\x1a\n":
            raise RuntimeError(f"Invalid PNG: {source}")
        width, height = struct.unpack(">II", header[16:24])
        result.append({"source": source, "relative": relative.as_posix(),
                       "destination": destination, "name": name,
                       "asset": asset_path, "width": width, "height": height})
    return result


def import_kenney_ui(verify_only=False):
    library = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    plan = _plan()
    tasks = []
    new_paths = set()
    for item in plan:
        if library.does_asset_exist(item["asset"]):
            texture = unreal.load_asset(item["asset"])
            if not isinstance(texture, unreal.Texture2D):
                raise RuntimeError(f"Preserving existing non-texture asset: {item['asset']}")
            if library.get_metadata_tag(texture, OWNER_TAG) != item["relative"]:
                raise RuntimeError(f"Preserving an asset not owned by this importer: {item['asset']}")
            continue
        if verify_only:
            raise RuntimeError(f"Missing saved UI texture: {item['asset']}")
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(item["source"]))
        task.set_editor_property("destination_path", item["destination"])
        task.set_editor_property("destination_name", item["name"])
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", False)
        task.set_editor_property("save", False)
        task.set_editor_property("factory", unreal.TextureFactory())
        tasks.append(task)
        new_paths.add(item["asset"])
    if tasks:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    manifest = []
    for item in plan:
        texture = unreal.load_asset(item["asset"])
        if not isinstance(texture, unreal.Texture2D):
            raise RuntimeError(f"Texture import failed: {item['asset']}")
        if item["asset"] in new_paths:
            for key, value in SETTINGS.items():
                texture.set_editor_property(key, value)
            library.set_metadata_tag(texture, OWNER_TAG, item["relative"])
            if not library.save_loaded_asset(texture, only_if_is_dirty=False):
                raise RuntimeError(f"Could not save UI texture: {item['asset']}")
        for key, expected in SETTINGS.items():
            if texture.get_editor_property(key) != expected:
                raise RuntimeError(f"UI setting {key} differs on {item['asset']}; existing edits are preserved.")
        size = (texture.blueprint_get_size_x(), texture.blueprint_get_size_y())
        if size != (item["width"], item["height"]):
            raise RuntimeError(f"Imported texture size differs: {item['asset']} {size}")
        manifest.append({"asset": item["asset"], "source": item["relative"],
                         "width": size[0], "height": size[1]})
    REPORT.mkdir(parents=True, exist_ok=True)
    report = {"destination": DESTINATION, "textures": len(manifest),
              "created": len(new_paths), "verify_only": verify_only,
              "settings": {key: str(value) for key, value in SETTINGS.items()},
              "assets": manifest}
    output = REPORT / ("verified.json" if verify_only else "imported.json")
    output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.log(f"KENNEY_UI_IMPORT_PASS: {len(manifest)} textures verified; "
               f"{len(new_paths)} created in {DESTINATION}; report: {output}")
    return report


if __name__ == "__main__":
    import_kenney_ui(verify_only="-KenneyUIVerifyOnly" in unreal.SystemLibrary.get_command_line())
