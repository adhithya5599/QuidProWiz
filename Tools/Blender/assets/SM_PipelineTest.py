"""
SM_PipelineTest: a throwaway orientation marker that proves the Blender -> Unreal pipeline.

  * grey base plate, 1 m square
  * red arrow pointing along the asset's front (Blender +X, which arrives as Unreal +X)
  * blue post pointing up (+Z)

Safe to delete once real assets exist.
"""

import math
import os
import sys

import bpy

# Blender ignores PYTHONPATH by default; make the shared helpers in Tools/Blender importable.
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import qpw_blender as qpw  # noqa: E402

ASSET_NAME = "SM_PipelineTest"


def build():
    qpw.reset_scene()

    grey = qpw.material("M_Base", (0.35, 0.35, 0.38))
    red = qpw.material("M_Forward", (0.80, 0.08, 0.06))
    blue = qpw.material("M_Up", (0.08, 0.25, 0.85))

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 0.025))
    base = bpy.context.active_object
    base.scale = (1.0, 1.0, 0.05)
    qpw.assign_material(base, grey)

    bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.04, depth=0.6,
                                        location=(0.3, 0.0, 0.1), rotation=(0.0, math.radians(90), 0.0))
    shaft = bpy.context.active_object
    qpw.assign_material(shaft, red)

    bpy.ops.mesh.primitive_cone_add(vertices=16, radius1=0.1, depth=0.2,
                                    location=(0.7, 0.0, 0.1), rotation=(0.0, math.radians(90), 0.0))
    tip = bpy.context.active_object
    qpw.assign_material(tip, red)

    bpy.ops.mesh.primitive_cylinder_add(vertices=12, radius=0.03, depth=0.5, location=(0.0, 0.0, 0.3))
    post = bpy.context.active_object
    qpw.assign_material(post, blue)

    marker = qpw.join([base, shaft, tip, post], ASSET_NAME)
    qpw.finalize(marker)
    qpw.publish(marker, ASSET_NAME)


build()
