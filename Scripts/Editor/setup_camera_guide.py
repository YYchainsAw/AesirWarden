"""Create/verify BP_CameraGuideTrigger without placing actors or saving any map.

After a full Editor build, run in the UE Python console or with:
UnrealEditor-Cmd.exe AesirWarden.uproject -run=pythonscript
-EnablePlugins=PythonScriptPlugin -script=".../setup_camera_guide.py"
-unattended -NullRHI -nosound -nop4
Existing Blueprint defaults and graphs are preserved.
"""

import json
from pathlib import Path

import unreal


ASSET = "/Game/Aesir/Framework/Camera/BP_CameraGuideTrigger"


def create_camera_guide_asset():
    if not unreal.AesirStartMenuEditorLibrary.create_camera_guide_asset():
        raise RuntimeError("Could not create/compile the camera guide Blueprint; inspect the editor log.")
    cls = unreal.EditorAssetLibrary.load_blueprint_class(ASSET)
    native = unreal.load_class(None, "/Script/AesirWarden.AesirCameraGuideTrigger")
    if not cls or not native or not unreal.MathLibrary.class_is_child_of(cls, native):
        raise RuntimeError("The camera guide Blueprint has an unexpected native parent.")
    defaults = unreal.get_default_object(cls)
    report = {
        "asset": ASSET,
        "native_parent": native.get_path_name(),
        "target_selection": "Set Target Actor on the placed instance, or choose Fixed Direction and rotate DirectionArrow.",
        "map_modified": False,
        "existing_blueprint_preserved": True,
        "trigger_box": str(defaults.get_editor_property("trigger_box")),
        "turn_duration": defaults.get_editor_property("turn_duration"),
        "hold_duration": defaults.get_editor_property("hold_duration"),
    }
    project_saved = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))
    output = project_saved / "CameraGuide" / "asset_setup.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.log(f"CAMERA_GUIDE_ASSET_PASS: {ASSET}; report: {output}")
    return report


if __name__ == "__main__":
    create_camera_guide_asset()
