"""Create the editable campfire menu assets without modifying a level.

Run after building AesirWardenEditor with the new native classes available:

    UnrealEditor-Cmd.exe AesirWarden.uproject -run=pythonscript \
        -EnablePlugins=PythonScriptPlugin -script=".../setup_start_menu.py" \
        -unattended -NullRHI -nosound

Or run this file from Unreal Editor's Python console. The native editor library
creates the Designer WidgetTree because WidgetTree construction is not exposed
as a supported Python editing API. Re-running preserves existing menu assets.

Only create_start_menu_assets() runs by default. To place the scene actor, import
this file as a module and call place_start_menu(location, rotation) explicitly.
Placement leaves the level unsaved for review in the editor.
"""

import unreal
import json
import math
import shutil
from pathlib import Path


WIDGET_PATH = "/Game/Aesir/UI/Menus/WBP_CampfireMainMenu"
ACTOR_PATH = "/Game/Aesir/Framework/Flow/BP_AesirCampfireStart"
LOG_PREFIX = "[Aesir Start Menu]"


def _required_class(path):
    cls = unreal.load_class(None, path)
    if cls is None:
        raise RuntimeError(
            f"{LOG_PREFIX} Missing class {path}. Build AesirWardenEditor and "
            "load the updated editor module before running this script."
        )
    return cls


def _verify_assets():
    """Validate native parent classes and the actor's menu widget assignment."""
    native_widget = _required_class("/Script/AesirWarden.AesirMainMenuWidget")
    native_actor = _required_class("/Script/AesirWarden.AesirCampfireStart")
    widget_class = _required_class(f"{WIDGET_PATH}.{WIDGET_PATH.rsplit('/', 1)[-1]}_C")
    actor_class = _required_class(f"{ACTOR_PATH}.{ACTOR_PATH.rsplit('/', 1)[-1]}_C")
    actor_defaults = unreal.get_default_object(actor_class)

    if not unreal.MathLibrary.class_is_child_of(widget_class, native_widget):
        raise RuntimeError(f"{LOG_PREFIX} {WIDGET_PATH} has an unexpected parent.")
    if not unreal.MathLibrary.class_is_child_of(actor_class, native_actor):
        raise RuntimeError(f"{LOG_PREFIX} {ACTOR_PATH} has an unexpected parent.")
    if actor_defaults.get_editor_property("main_menu_widget_class") != widget_class:
        raise RuntimeError(
            f"{LOG_PREFIX} {ACTOR_PATH}.MainMenuWidgetClass must reference "
            f"{WIDGET_PATH}. Existing user edits have been preserved."
        )
    unreal.log(f"{LOG_PREFIX} Verified {WIDGET_PATH} and {ACTOR_PATH}.")
    return actor_class


def create_start_menu_assets():
    """Create/save the two menu assets; do not load, place, or save a map."""
    editor_library = getattr(unreal, "AesirStartMenuEditorLibrary", None)
    if editor_library is None:
        raise RuntimeError(
            f"{LOG_PREFIX} AesirStartMenuEditorLibrary is unavailable. "
            "Build AesirWardenEditor and reopen the editor with its updated "
            "module. The existing open editor may still have the old DLL loaded."
        )
    if not editor_library.create_start_menu_assets():
        raise RuntimeError(
            f"{LOG_PREFIX} Native asset creation failed. See the preceding "
            "LogAesir/LogBlueprint messages; the current map was not modified."
        )
    actor_class = _verify_assets()
    unreal.log(
        f"{LOG_PREFIX} Open WBP_CampfireMainMenu in the UMG Designer to adjust "
        "the layout. Place BP_AesirCampfireStart at the intended campfire spot. "
        "No level was modified or saved."
    )
    return actor_class


def place_start_menu(location, rotation=None):
    """Explicitly place one persistent menu actor in the current editor level.

    Example from the Python console after importing this file as a module:
        setup_start_menu.place_start_menu(unreal.Vector(1200, 400, 100))

    Choose the campfire position in the viewport first. Coordinates are Unreal
    units, not an inferred PlayerStart. Undo is supported; saving is manual.
    Existing flow actors must be reviewed before adding another owner.
    """
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    if editor.get_game_world() is not None:
        raise RuntimeError(f"{LOG_PREFIX} Stop Play/Simulate before placing actors.")
    if editor.get_editor_world() is None:
        raise RuntimeError(f"{LOG_PREFIX} Open the intended level before placement.")

    actor_class = _verify_assets()
    flow_class = _required_class("/Script/AesirWarden.AesirLevelFlowDirector")
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    existing = [
        actor
        for actor in actor_subsystem.get_all_level_actors()
        if unreal.MathLibrary.class_is_child_of(actor.get_class(), flow_class)
    ]
    if existing:
        labels = ", ".join(actor.get_actor_label() for actor in existing)
        raise RuntimeError(
            f"{LOG_PREFIX} A level-flow actor already exists: {labels}. "
            "Review the existing actor before placing another menu owner."
        )

    with unreal.ScopedEditorTransaction("Place Aesir Campfire Start Menu"):
        actor = actor_subsystem.spawn_actor_from_class(
            actor_class, location, rotation or unreal.Rotator()
        )
        if actor is None:
            raise RuntimeError(f"{LOG_PREFIX} Could not spawn {ACTOR_PATH}.")
        actor.set_actor_label("Aesir Campfire Start")
        actor.set_editor_property("is_spatially_loaded", False)

    unreal.log(
        f"{LOG_PREFIX} Placed a persistent campfire menu actor. Adjust its "
        "camera, fire and light components, then review and save the level."
    )
    return actor


def install_in_main_at_player_start():
    """Explicit one-time Main integration, using the existing single PlayerStart.

    Keep Main's actors and player start intact. Back up its map package, add the
    new persistent campfire nearby, and save this level. A manifest records the
    placement so the local, non-public map can be restored or adjusted later.
    """
    create_start_menu_assets()
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.load_level('/Game/Main'):
        raise RuntimeError(f'{LOG_PREFIX} Could not load /Game/Main.')

    descs = unreal.WorldPartitionBlueprintLibrary.get_actor_descs()
    needed = [d.get_editor_property('guid') for d in descs
              if any(word in str(d.get_editor_property('native_class'))
                     for word in ('PlayerStart', 'AesirLevelFlowDirector', 'AesirCampfireStart'))]
    unreal.WorldPartitionBlueprintLibrary.load_actors(needed)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    existing = [a for a in actors.get_all_level_actors()
                if isinstance(a, unreal.AesirLevelFlowDirector)]
    if existing:
        if len(existing) == 1 and isinstance(existing[0], unreal.AesirCampfireStart):
            unreal.log(f'{LOG_PREFIX} Main already has its campfire; preserving placement.')
            return existing[0]
        raise RuntimeError(f'{LOG_PREFIX} Review existing flow actors before installing.')

    starts = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.PlayerStart)]
    if len(starts) != 1:
        raise RuntimeError(f'{LOG_PREFIX} Expected one PlayerStart, found {len(starts)}.')
    start = starts[0]
    origin = start.get_actor_location()
    yaw = start.get_actor_rotation().yaw
    rotation = unreal.Rotator(0.0, yaw, 0.0)
    angle = math.radians(yaw)
    offset = unreal.Vector(110.0 * math.cos(angle) - 130.0 * math.sin(angle),
                           110.0 * math.sin(angle) + 130.0 * math.cos(angle), 0.0)
    location = origin + offset
    # Existing Character capsule stands about 90cm above the floor. A local
    # collision trace refines this when the relevant partition cells are loaded.
    location.z = origin.z - 92.0
    region = unreal.Box(min=origin - unreal.Vector(1200, 1200, 1500),
                        max=origin + unreal.Vector(1200, 1200, 1500))
    nearby = unreal.WorldPartitionBlueprintLibrary.get_intersecting_actor_descs(region)
    unreal.WorldPartitionBlueprintLibrary.load_actors([d.get_editor_property('guid') for d in nearby])
    hit = unreal.SystemLibrary.line_trace_single(
        unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),
        unreal.Vector(location.x, location.y, origin.z + 40.0),
        unreal.Vector(location.x, location.y, origin.z - 400.0),
        unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, starts,
        unreal.DrawDebugTrace.NONE, True)
    if hit:
        hit_data = unreal.GameplayStatics.break_hit_result(hit)
        if hit_data[0]:
            location = hit_data[5]  # ImpactPoint

    backup = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) / 'StartMenu' / 'BeforeInstall'
    backup.mkdir(parents=True, exist_ok=True)
    source_map = Path(unreal.Paths.project_content_dir()) / 'Main.umap'
    if not (backup / 'Main.umap').exists():
        shutil.copy2(source_map, backup / 'Main.umap')

    actor = place_start_menu(location, rotation)
    hud = unreal.load_class(None, '/Game/Aesir/UI/HealthBars/WBP_CombatHUD.WBP_CombatHUD_C')
    if hud:
        actor.set_editor_property('gameplay_hud_widget_classes', [hud])
    if not levels.save_current_level():
        raise RuntimeError(f'{LOG_PREFIX} Failed to save Main; inspect the editor log.')
    manifest = {'map': '/Game/Main', 'actor': actor.get_path_name(),
                'location': {'x': location.x, 'y': location.y, 'z': location.z},
                'yaw': yaw, 'player_start': start.get_path_name(),
                'backup_map': str(backup / 'Main.umap')}
    (backup.parent / 'MainMenuPlacement.json').write_text(
        json.dumps(manifest, indent=2), encoding='utf-8')
    unreal.log(f'{LOG_PREFIX} Installed campfire in Main: {manifest}')
    return actor


if __name__ == "__main__":
    create_start_menu_assets()
