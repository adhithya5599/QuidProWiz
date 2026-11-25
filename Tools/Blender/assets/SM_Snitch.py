"""
SM_Snitch: placeholder Snitch mesh. A small gold ball with a raised band around its middle
so its spin and direction read in motion. Deliberately plain; replace with final art later.

  * 25 cm diameter (gameplay-readable; the catch sphere is set separately in DA_Snitch)
  * band runs around the asset's forward axis (Blender +X -> Unreal +X)
"""

import math
import os
import sys

import bpy

# Blender ignores PYTHONPATH by default; make the shared helpers in Tools/Blender importable.
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import qpw_blender as qpw  # noqa: E402

ASSET_NAME = "SM_Snitch"
RADIUS = 0.125


def build():
    qpw.reset_scene()

    gold = qpw.material("M_SnitchGold", (0.95, 0.70, 0.15), roughness=0.25, metallic=1.0)
    band_gold = qpw.material("M_SnitchBand", (0.75, 0.50, 0.08), roughness=0.35, metallic=1.0)

    bpy.ops.mesh.primitive_uv_sphere_add(segments=32, ring_count=16, radius=RADIUS, location=(0.0, 0.0, 0.0))
    ball = bpy.context.active_object
    qpw.assign_material(ball, gold)

    # Band around the forward (+X) axis: a torus lying in the YZ plane.
    bpy.ops.mesh.primitive_torus_add(major_radius=RADIUS, minor_radius=0.012, major_segments=48, minor_segments=8,
                                     location=(0.0, 0.0, 0.0), rotation=(0.0, math.radians(90), 0.0))
    band = bpy.context.active_object
    qpw.assign_material(band, band_gold)

    snitch = qpw.join([ball, band], ASSET_NAME)
    qpw.finalize(snitch, smooth_angle_degrees=60.0)
    qpw.publish(snitch, ASSET_NAME)


build()
