"""
Shared helpers for QuidProWiz asset scripts that run inside Blender.

Asset scripts live in Tools/Blender/assets/ and are run headless through
Tools/Blender/BuildAsset.ps1, e.g.

    .\\Tools\\Blender\\BuildAsset.ps1 -Asset SM_PipelineTest

Conventions (keep these so assets import into Unreal correctly):
  * Units: model in metres (Blender default). 1 m in Blender = 100 Unreal units.
  * Facing: the asset's front faces Blender +X (arrives as Unreal +X, an actor's forward),
    up is +Z. Blender -Y becomes Unreal +Y. Pivot at the world origin.
  * Names: Unreal prefixes (SM_ static mesh, SK_ skeletal mesh, M_ material slot).
  * Each script builds the asset from scratch, so every asset is reproducible.

Outputs go to SourceArt/Generated/<AssetName>/: the .fbx Unreal imports, a .blend
for hand-tweaking, and a _preview.png render for quick review.
"""

import math
import os
import sys

import bpy
from mathutils import Vector


# --- Arguments ---------------------------------------------------------------------------------

def script_args():
    """Arguments passed after '--' on the Blender command line, as a dict of --key value pairs."""
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    args = {}
    key = None
    for token in argv:
        if token.startswith("--"):
            key = token[2:]
            args[key] = True
        elif key:
            args[key] = token
            key = None
    return args


def output_dir(asset_name):
    args = script_args()
    root = args.get("out") or os.path.join(os.getcwd(), "SourceArt", "Generated")
    path = os.path.join(root, asset_name)
    os.makedirs(path, exist_ok=True)
    return path


# --- Scene --------------------------------------------------------------------------------------

def reset_scene():
    """Start from an empty scene with metric units."""
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    return scene


def material(name, color, roughness=0.6, metallic=0.0):
    """Creates (or reuses) a simple principled material; its name becomes the Unreal slot name."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs["Base Color"].default_value = (*color, 1.0)
        bsdf.inputs["Roughness"].default_value = roughness
        bsdf.inputs["Metallic"].default_value = metallic
    mat.diffuse_color = (*color, 1.0)  # used by the Workbench preview render
    return mat


def assign_material(obj, mat):
    obj.data.materials.clear()
    obj.data.materials.append(mat)


def join(objects, name):
    """Joins objects into one mesh object called name."""
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.object.join()
    result = bpy.context.view_layer.objects.active
    result.name = name
    result.data.name = name
    return result


def finalize(obj, smooth_angle_degrees=40.0):
    """Applies transforms, sets smoothing and generates UVs so the mesh imports cleanly."""
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    bpy.ops.object.shade_auto_smooth(angle=math.radians(smooth_angle_degrees))

    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(66.0), island_margin=0.02)
    bpy.ops.object.mode_set(mode="OBJECT")
    return obj


# --- Output -------------------------------------------------------------------------------------

def export_fbx(obj, path):
    """Exports one mesh object as an Unreal-ready FBX."""
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(
        filepath=path,
        use_selection=True,
        object_types={"MESH"},
        apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_NONE",
        axis_forward="-Z",
        axis_up="Y",
        mesh_smooth_type="FACE",
        use_tspace=True,
        add_leaf_bones=False,
        bake_anim=False,
    )
    return path


def render_preview(path, resolution=512):
    """Renders a quick three-quarter view of everything in the scene to a PNG."""
    scene = bpy.context.scene

    meshes = [o for o in scene.objects if o.type == "MESH"]
    corners = [o.matrix_world @ Vector(c) for o in meshes for c in o.bound_box]
    center = sum(corners, Vector()) / len(corners)
    radius = max((c - center).length for c in corners)

    camera_data = bpy.data.cameras.new("PreviewCamera")
    camera = bpy.data.objects.new("PreviewCamera", camera_data)
    scene.collection.objects.link(camera)
    direction = Vector((1.0, -1.4, 0.8)).normalized()
    camera.location = center + direction * radius * 3.2
    camera.rotation_euler = (center - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera_data.lens = 50
    scene.camera = camera

    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "MATERIAL"
    scene.display.shading.show_cavity = True
    scene.render.resolution_x = resolution
    scene.render.resolution_y = resolution
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)

    bpy.data.objects.remove(camera, do_unlink=True)
    return path


def save_blend(path):
    bpy.ops.wm.save_as_mainfile(filepath=path, check_existing=False)
    return path


def publish(obj, asset_name):
    """Writes the FBX, preview and .blend for an asset and prints a summary line."""
    out = output_dir(asset_name)
    fbx = export_fbx(obj, os.path.join(out, f"{asset_name}.fbx"))
    preview = render_preview(os.path.join(out, f"{asset_name}_preview.png"))
    blend = save_blend(os.path.join(out, f"{asset_name}.blend"))

    dims = obj.dimensions
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    print(f"QPW_ASSET name={asset_name} tris={tris} "
          f"size_cm=({dims.x * 100:.1f}, {dims.y * 100:.1f}, {dims.z * 100:.1f}) "
          f"materials={[m.name for m in obj.data.materials]}")
    print(f"QPW_OUTPUT fbx={fbx}")
    print(f"QPW_OUTPUT preview={preview}")
    print(f"QPW_OUTPUT blend={blend}")
