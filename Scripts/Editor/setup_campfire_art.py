"""Attach the imported Quixel campfire and Epic Niagara fire to the menu BP.

Run explicitly in an idle editor, or as a Python commandlet. Existing components,
the menu Designer, actor placement and player animation settings are preserved.
Only newly added art components receive the initial tuning on subsequent runs.
Third-party assets remain in their original, non-public Content directories.
"""

import json
import shutil
from pathlib import Path

import unreal


BLUEPRINT = '/Game/Aesir/Framework/Flow/BP_AesirCampfireStart'
MESH = '/Game/Fab/Megascans/3D/Campfire_uenpfeifa/High/uenpfeifa_tier_1/StaticMeshes/uenpfeifa_tier_1'
FIRE = '/Game/NiagaraExamples/FX_Misc/NS_Fire'
EMISSION_MESH = '/Game/CustomAssets/Campfire/SM_CampfireEmissionDisk'
FLAME_MASTER = '/Game/CustomAssets/Campfire/M_MenuCampfireFlames'
FLAME_MATERIAL = '/Game/CustomAssets/Campfire/MI_MenuCampfireFlames'
PLACEHOLDERS = ('Firewood', 'StoneRing', 'FlameCard', 'FlameCardCross')
FIRE_FLOATS = {
    'User.Flame Scale': 0.9,
    'User.Lifetime Scale': 0.85,
    'User.Spawn Rate': 220.0,
    'User.Smoke Spawn Scale': 0.035,
    # Keep the menu actor's existing controllable, flickering point light.
    # The spelling is the original Niagara sample's exposed parameter name.
    'User.Base Light Intentsity': 0.0,
}


def _components(blueprint):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    library = unreal.SubobjectDataBlueprintFunctionLibrary
    result = {}
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        data = library.get_data(handle)
        result[str(library.get_variable_name(data))] = (
            handle, library.get_object_for_blueprint(data, blueprint))
    return result


def _add_component(blueprint, name, component_class, parent='CampfireRoot'):
    components = _components(blueprint)
    if name in components:
        component = components[name][1]
        if not isinstance(component, component_class):
            raise RuntimeError(f'{name} already exists with an unexpected type; preserving it.')
        if 'AesirMenuCampfireArt' not in [str(tag) for tag in component.get_editor_property('component_tags')]:
            raise RuntimeError(f'{name} already exists without this installer\'s ownership tag; preserving it.')
        return component, False
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handle, reason = subsystem.add_new_subobject(unreal.AddNewSubobjectParams(
        parent_handle=components[parent][0],
        new_class=component_class,
        blueprint_context=blueprint))
    if not unreal.SubobjectDataBlueprintFunctionLibrary.is_handle_valid(handle):
        raise RuntimeError(f'Could not add {name}: {reason}')
    if not subsystem.rename_subobject(handle, name):
        raise RuntimeError(f'Could not name the new component {name}.')
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    return unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, blueprint), True


def _flame_material():
    """Use local exposure controls instead of the sample Gallery's global MPC."""
    library = unreal.MaterialEditingLibrary
    material = unreal.load_asset(FLAME_MASTER) if unreal.EditorAssetLibrary.does_asset_exist(FLAME_MASTER) else None
    if material is None:
        material = unreal.EditorAssetLibrary.duplicate_asset('/Game/NiagaraExamples/Materials/MasterMaterials/M_Flames', FLAME_MASTER)
        calls = [node for node in library.get_material_expressions(material)
                 if isinstance(node, unreal.MaterialExpressionMaterialFunctionCall)
                 and node.get_editor_property('material_function')
                 and node.get_editor_property('material_function').get_name() == 'MF_ExposureCompensation']
        if len(calls) != 1:
            raise RuntimeError('The sample flame material no longer has one exposure compensation function.')
        exposure = calls[0]
        input_names = library.get_material_expression_input_names(exposure)
        for parameter, default, input_prefix, y in (
                ('Menu Fire Exposure Mix', 1.0, 'Auto Exposure Compensation Mix', -320),
                ('Menu Fire Exposure EV', 0.0, 'Exposure Compensation', -480)):
            scalar = library.create_material_expression(material, unreal.MaterialExpressionScalarParameter, 1000, y)
            scalar.set_editor_property('parameter_name', parameter)
            scalar.set_editor_property('default_value', default)
            input_name = next(str(name) for name in input_names if str(name).startswith(input_prefix))
            if not library.connect_material_expressions(scalar, '', exposure, input_name):
                raise RuntimeError(f'Could not connect {parameter} in the menu flame material.')
        errors = library.recompile_material(material)
        if errors:
            raise RuntimeError(f'Menu flame material compilation failed: {errors}')
        if not unreal.EditorAssetLibrary.save_loaded_asset(material, False):
            raise RuntimeError('Could not save the local menu flame material.')

    instance = unreal.load_asset(FLAME_MATERIAL) if unreal.EditorAssetLibrary.does_asset_exist(FLAME_MATERIAL) else None
    if instance is None:
        instance = unreal.EditorAssetLibrary.duplicate_asset('/Game/NiagaraExamples/Materials/MI_Flames', FLAME_MATERIAL)
        library.set_material_instance_parent(instance, material)
        library.update_material_instance(instance)
        if not unreal.EditorAssetLibrary.save_loaded_asset(instance, False):
            raise RuntimeError('Could not save the local menu flame material instance.')
    return instance


def _flame_material_override(component):
    for variable, value in component.get_editor_property('template_parameter_overrides').items():
        if str(variable.get_editor_property('name')) == 'User.Flame Material':
            return value.get_editor_property('object')
    return None


def install_campfire_art():
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    if editor.get_game_world() is not None:
        raise RuntimeError('Stop Play/Simulate before installing the campfire art.')
    mesh = unreal.load_asset(MESH)
    fire = unreal.load_asset(FIRE)
    if not unreal.EditorAssetLibrary.does_asset_exist(BLUEPRINT) or not isinstance(mesh, unreal.StaticMesh) or not isinstance(fire, unreal.NiagaraSystem):
        raise RuntimeError('The menu BP, Quixel Campfire and Niagara Examples NS_Fire must all be imported first.')

    output = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) / 'StartMenu' / 'CampfireArt'
    backup = output / 'BeforeInstall'
    backup.mkdir(parents=True, exist_ok=True)
    source = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir())) / 'Aesir/Framework/Flow/BP_AesirCampfireStart.uasset'
    if not (backup / source.name).exists():
        shutil.copy2(source, backup / source.name)

    # NS_Fire samples an attached mesh surface. Without an explicit mesh parent,
    # it finds the actor's hidden placeholder Cylinder, which has no CPU data.
    # Keep a private copy of Epic's disk so the source sample remains untouched.
    emission_mesh = unreal.load_asset(EMISSION_MESH) if unreal.EditorAssetLibrary.does_asset_exist(EMISSION_MESH) else None
    if emission_mesh is None:
        emission_mesh = unreal.EditorAssetLibrary.duplicate_asset('/Game/NiagaraExamples/StaticMesh/SM_Disk', EMISSION_MESH)
        if not emission_mesh:
            raise RuntimeError('Could not create the campfire emission disk.')
        emission_mesh.set_editor_property('allow_cpu_access', True)
        if not unreal.EditorAssetLibrary.save_loaded_asset(emission_mesh, False):
            raise RuntimeError('Could not save the CPU-readable campfire emission disk.')
    if not isinstance(emission_mesh, unreal.StaticMesh) or not emission_mesh.get_editor_property('allow_cpu_access'):
        raise RuntimeError('The existing emission mesh is not CPU-readable; preserving it for review.')
    flame_material = _flame_material()

    # Build the ignored local dependencies before loading the tracked BP. A
    # checkout on another machine must resolve those references during load.
    blueprint = unreal.load_asset(BLUEPRINT)
    if not blueprint:
        raise RuntimeError('The existing campfire Blueprint could not be loaded.')
    # A transaction supports Undo after success; Python exceptions do not undo
    # earlier edits. Check all existing components before adding any of them.
    existing = _components(blueprint)
    for name, expected_class, expected_asset, asset_property in (
            ('CampfireScannedBase', unreal.StaticMeshComponent, mesh, 'static_mesh'),
            ('CampfireNiagara', unreal.NiagaraComponent, fire, 'asset'),
            ('CampfireEmissionSurface', unreal.StaticMeshComponent, emission_mesh, 'static_mesh')):
        if name not in existing:
            continue
        component = existing[name][1]
        if (not isinstance(component, expected_class)
                or 'AesirMenuCampfireArt' not in [str(tag) for tag in component.get_editor_property('component_tags')]):
            raise RuntimeError(f'{name} conflicts with an existing component; preserving it.')
        actual = component.get_asset() if asset_property == 'asset' else component.get_editor_property(asset_property)
        if actual != expected_asset and not (name == 'CampfireEmissionSurface' and actual is None):
            raise RuntimeError(f'{name} uses a different asset; preserving the existing art.')

    with unreal.ScopedEditorTransaction('Install Quixel and Niagara menu campfire'):
        blueprint.modify()
        base, base_created = _add_component(blueprint, 'CampfireScannedBase', unreal.StaticMeshComponent)
        if base_created:
            base.set_static_mesh(mesh)
            # The scan is 141 x 124 cm, with its bottom about 1 cm below its pivot.
            base.set_editor_property('relative_location', unreal.Vector(0.0, 0.0, 1.0))
            base.set_editor_property('relative_scale3d', unreal.Vector(1.0, 1.0, 1.0))
            base.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
            base.set_editor_property('receives_decals', False)
            base.set_editor_property('component_tags', ['AesirMenuCampfireArt'])
        elif base.get_editor_property('static_mesh') != mesh:
            raise RuntimeError('CampfireScannedBase uses a different mesh; preserving the existing art.')

        surface, surface_created = _add_component(blueprint, 'CampfireEmissionSurface', unreal.StaticMeshComponent)
        if surface_created:
            surface.set_static_mesh(emission_mesh)
            surface.set_editor_property('relative_location', unreal.Vector(0.0, 0.0, 12.0))
            surface.set_editor_property('relative_scale3d', unreal.Vector(0.65, 0.65, 1.0))
            surface.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
            surface.set_visibility(False, False)
            surface.set_hidden_in_game(True, False)
            surface.set_cast_shadow(False)
            surface.set_editor_property('component_tags', ['AesirMenuCampfireArt'])
        elif surface.get_editor_property('static_mesh') is None:
            # An editor that loaded the BP before dependency restoration may
            # still hold a null reference even after the asset is recreated.
            surface.set_static_mesh(emission_mesh)

        effect, effect_created = _add_component(blueprint, 'CampfireNiagara', unreal.NiagaraComponent, 'CampfireEmissionSurface')
        if effect_created:
            effect.set_asset(fire)
            effect.set_editor_property('relative_location', unreal.Vector(0.0, 0.0, 0.0))
            effect.set_editor_property('auto_activate', True)
            effect.set_editor_property('component_tags', ['AesirMenuCampfireArt'])
            for name, value in FIRE_FLOATS.items():
                effect.set_variable_float(name, value)
            effect.set_variable_linear_color('User.Smoke Color', unreal.LinearColor(0.08, 0.075, 0.07, 0.16))
        elif effect.get_asset() != fire:
            raise RuntimeError('CampfireNiagara uses a different system; preserving the existing art.')

        tags = [str(t) for t in effect.get_editor_property('component_tags')]
        if 'AesirMenuCampfireSamplingV2' not in tags:
            components = _components(blueprint)
            subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
            if not subsystem.attach_subobject(components['CampfireEmissionSurface'][0], components['CampfireNiagara'][0]):
                raise RuntimeError('Could not attach Niagara to its explicit sampling surface.')
            effect.set_editor_property('relative_location', unreal.Vector(0.0, 0.0, 0.0))
            effect.set_visibility(True, False)
            effect.set_hidden_in_game(False, False)
            effect.set_editor_property('component_tags', tags + ['AesirMenuCampfireSamplingV2'])

        tags = [str(t) for t in effect.get_editor_property('component_tags')]
        if 'AesirMenuCampfireExposureV3' not in tags or _flame_material_override(effect) is None:
            effect.set_variable_material('User.Flame Material', flame_material)
            if 'AesirMenuCampfireExposureV3' not in tags:
                effect.set_editor_property('component_tags', tags + ['AesirMenuCampfireExposureV3'])

        if not unreal.BlueprintEditorLibrary.compile_blueprint(blueprint):
            raise RuntimeError('Campfire Blueprint compilation failed; inspect the Output Log before saving.')
        # Native inherited default subobjects live on the generated class CDO.
        # Set these after SCS compilation; do not clear their mesh or rename them.
        components = _components(blueprint)
        for name in PLACEHOLDERS:
            component = components[name][1]
            component.modify()
            component.set_visibility(False, False)
            component.set_hidden_in_game(True, False)
        if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False):
            raise RuntimeError('Could not save BP_AesirCampfireStart.')

    report = {
        'blueprint': BLUEPRINT,
        'mesh': MESH,
        'niagara': FIRE,
        'emission_mesh': EMISSION_MESH,
        'flame_material': FLAME_MATERIAL,
        'new_mesh_component': base_created,
        'new_niagara_component': effect_created,
        'hidden_placeholders': list(PLACEHOLDERS),
        'initial_niagara_floats': FIRE_FLOATS,
        'backup': str(backup / source.name),
        'map_saved': False,
    }
    (output / 'installation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    unreal.log('CAMPFIRE_ART_INSTALLED ' + json.dumps(report))
    return report


if __name__ == '__main__':
    install_campfire_art()
