"""
SM_Gate: a ring for the gym's flight course.

  * opening faces the asset's forward axis (Blender +X -> Unreal +X), so a gate's +X is the
    direction you fly through it
  * 4.8 m outer diameter, 4.4 m opening (matches AGymFlightCourse::GateRadius = 220 cm)
  * one material slot; the course swaps it to show the next / passed gates
  * pivot at the ring's centre
"""

import math
import os
import sys

import bpy

# Blender ignores PYTHONPATH by default; make the shared helpers in Tools/Blender importable.
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import qpw_blender as qpw  # noqa: E402

ASSET_NAME = "SM_Gate"
MAJOR_RADIUS = 2.4
MINOR_RADIUS = 0.18


def build():
    qpw.reset_scene()

    glow = qpw.material("M_GateRing", (0.25, 0.55, 1.0), roughness=0.4, metallic=0.2)

    # Torus standing in the YZ plane so its hole looks down +X.
    bpy.ops.mesh.primitive_torus_add(major_radius=MAJOR_RADIUS, minor_radius=MINOR_RADIUS,
                                     major_segments=64, minor_segments=12,
                                     location=(0.0, 0.0, 0.0), rotation=(0.0, math.radians(90), 0.0))
    ring = bpy.context.active_object
    ring.name = ASSET_NAME
    qpw.assign_material(ring, glow)

    qpw.finalize(ring, smooth_angle_degrees=60.0)
    qpw.publish(ring, ASSET_NAME)


build()
