"""Create the first Stage 12 Earthworks facility mesh and Blender preview.

Run with the project portable Blender:
  blender.exe --background --factory-startup --python assets/building/generate_earthworks_facility.py

This creates a source .blend, a Wavefront OBJ/MTL for ModelViewer import, and a
PNG review render. The mesh is original project content.
"""

from pathlib import Path
import math
import sys
import bpy
import bmesh
from mathutils import Vector


ROOT = Path(__file__).resolve().parents[2]
MEDIUM = "--medium" in sys.argv
LARGE = "--large" in sys.argv
if MEDIUM and LARGE:
    raise SystemExit("Choose one building size: --medium or --large")
VARIANT = "large" if LARGE else ("medium" if MEDIUM else "small")
OUT = ROOT / "assets" / "building" / "generated"
if MEDIUM or LARGE:
    OUT = OUT / VARIANT
OUT.mkdir(parents=True, exist_ok=True)
suffix = "_" + VARIANT if VARIANT != "small" else ""
BLEND = OUT / f"earthworks_facility{suffix}.blend"
OBJ = OUT / f"earthworks_facility{suffix}.obj"
preview_name = {
    "small": "STAGE12_FACILITY_MODEL_PREVIEW.png",
    "medium": "STAGE12_FACILITY_MEDIUM_PREVIEW.png",
    "large": "STAGE12_FACILITY_LARGE_PREVIEW.png",
}[VARIANT]
PREVIEW = ROOT / "docs" / "mockups" / preview_name
sys.path.insert(0, str(Path(__file__).resolve().parent))
from bake_atlas_patches import bake_patch


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    # Materials are created at module import time, before main() clears the
    # factory scene. Keep them alive while removing any prior scene geometry.
    for datablocks in (bpy.data.meshes, bpy.data.curves, bpy.data.cameras, bpy.data.lights):
        for datablock in list(datablocks):
            if datablock.users == 0:
                datablocks.remove(datablock)


def material(name, color, roughness=0.82, metallic=0.0):
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = (*color, 1.0)
    mat.use_nodes = True
    principled = mat.node_tree.nodes.get("Principled BSDF")
    principled.inputs["Base Color"].default_value = (*color, 1.0)
    principled.inputs["Roughness"].default_value = roughness
    principled.inputs["Metallic"].default_value = metallic
    return mat


# Muted industrial palette used by the facility preview and Workshop materials.
PLASTER = material("dispatch_concrete", (0.64, 0.63, 0.59))
CONCRETE = material("foundation_concrete", (0.59, 0.59, 0.56))
STEEL = material("canopy_structural_steel", (0.31, 0.33, 0.34), 0.72, 0.12)
ROOF = material("corrugated_galvanized_roof", (1.00, 1.00, 1.00), 0.88, 0.02)
OFFICE_ROOF = material("office_roof_graphite", (0.35, 0.37, 0.38), 0.76, 0.08)
SAFETY_RED = material("safety_red", (0.52, 0.12, 0.09), 0.72, 0.02)
GLASS = material("office_window_glass", (0.29, 0.47, 0.53), 0.32, 0.05)
FRAME = material("window_frames", (0.19, 0.22, 0.24), 0.72, 0.04)
TANK = material("decorative_tank_painted_steel", (1.00, 1.00, 1.00), 0.90, 0.01)
TANK_TOP = material("tank_cap_steel", (0.28, 0.31, 0.28), 0.90, 0.01)
CONTAINER_RED = material("container_oxide_red", (0.40, 0.19, 0.14), 0.86, 0.03)
CONTAINER_BLUE = material("container_blue", (0.20, 0.29, 0.34), 0.84, 0.04)


EXPORT_OBJECTS = []


def add_box(name, center, size, mat, bevel=0.0):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=center)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(mat)
    if bevel:
        bevel_mod = obj.modifiers.new("small_game_asset_edge_bevel", "BEVEL")
        bevel_mod.width = bevel
        bevel_mod.segments = 1
        bevel_mod.limit_method = "ANGLE"
        bevel_mod.angle_limit = math.radians(28)
        obj.modifiers.new("weighted_corner_normals", "WEIGHTED_NORMAL")
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.modifier_apply(modifier=bevel_mod.name)
        normal = obj.modifiers.get("weighted_corner_normals")
        if normal:
            bpy.ops.object.modifier_apply(modifier=normal.name)
    EXPORT_OBJECTS.append(obj)
    return obj


def add_beam(name, start, end, width, depth, mat, bevel=0.025):
    start = Vector(start)
    end = Vector(end)
    delta = end - start
    obj = add_box(name, (start + end) * 0.5, (width, depth, delta.length), mat, bevel)
    obj.rotation_mode = "QUATERNION"
    obj.rotation_quaternion = delta.to_track_quat("Z", "Y")
    return obj


def add_cylinder(name, center, radius, depth, mat, vertices=16, bevel=0.04):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=center)
    obj = bpy.context.object
    obj.name = name
    obj.data.materials.append(mat)
    if bevel:
        mod = obj.modifiers.new("soft_cylinder_edges", "BEVEL")
        mod.width = bevel
        mod.segments = 1
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.modifier_apply(modifier=mod.name)
    EXPORT_OBJECTS.append(obj)
    return obj


def apply_cylinder_atlas_uvs(obj, material_name, center_xy):
    """Unwrap a vertical cylinder around its circumference into an atlas patch."""
    texture, (left, top, right, bottom), width, height = TEXTURED_ATLAS_REGIONS[material_name]
    uv_layer = obj.data.uv_layers.active or obj.data.uv_layers.new(name="UVMap")
    z_values = [obj.matrix_world @ vertex.co for vertex in obj.data.vertices]
    z_min = min(point.z for point in z_values)
    z_max = max(point.z for point in z_values)
    if material_name in BAKED_ATLAS_TEXTURES:
        u_min, u_max, v_min, v_max = 0.0, 1.0, 0.0, 1.0
    else:
        u_min, u_max = left / width, right / width
        v_min, v_max = 1.0 - bottom / height, 1.0 - top / height

    for polygon in obj.data.polygons:
        loop_angles = []
        for loop_index in polygon.loop_indices:
            vertex = obj.data.vertices[obj.data.loops[loop_index].vertex_index]
            point = obj.matrix_world @ vertex.co
            angle = math.atan2(point.y - center_xy[1], point.x - center_xy[0])
            loop_angles.append(angle if angle >= 0.0 else angle + 2.0 * math.pi)
        # Keep a polygon crossing the 0/2π seam locally continuous.
        if loop_angles and max(loop_angles) - min(loop_angles) > math.pi:
            loop_angles = [angle + (2.0 * math.pi if angle < math.pi else 0.0) for angle in loop_angles]
        for loop_index, angle in zip(polygon.loop_indices, loop_angles):
            vertex = obj.data.vertices[obj.data.loops[loop_index].vertex_index]
            point = obj.matrix_world @ vertex.co
            u = u_min + (angle / (2.0 * math.pi)) * (u_max - u_min)
            v_t = 0.5 if z_max - z_min < 1e-6 else (point.z - z_min) / (z_max - z_min)
            uv_layer.data[loop_index].uv = (u, v_min + v_t * (v_max - v_min))


def add_window(name, x, z, width, height, y=-8.74):
    add_box(name + "_glass", (x, y, z), (width, 0.08, height), GLASS, 0.025)
    t = 0.11
    add_box(name + "_frame_left", (x - width / 2 - t / 2, y - 0.04, z), (t, 0.15, height + 2 * t), FRAME, 0.025)
    add_box(name + "_frame_right", (x + width / 2 + t / 2, y - 0.04, z), (t, 0.15, height + 2 * t), FRAME, 0.025)
    add_box(name + "_frame_top", (x, y - 0.04, z + height / 2 + t / 2), (width + 2 * t, 0.15, t), FRAME, 0.02)
    add_box(name + "_frame_bottom", (x, y - 0.04, z - height / 2 - t / 2), (width + 2 * t, 0.15, t), FRAME, 0.02)
    add_box(name + "_mullion_v", (x, y - 0.10, z), (0.07, 0.08, height), FRAME)
    add_box(name + "_mullion_h", (x, y - 0.10, z), (width, 0.08, 0.07), FRAME)


def make_building():
    # Native CONNECTIONS_ROAD_DEAD_SQUARE metadata supplies the finished
    # asphalt surface for the vehicle court/forecourt. Keep ground out of mesh.
    add_box("office_plinth", (-12.8, -0.8, 0.56), (13.6, 16.2, 0.42), CONCRETE, 0.08)

    # Compact dispatch office to the left of the canopy.
    add_box("dispatch_office_main_shell", (-12.8, -0.8, 3.05), (13.0, 15.6, 5.0), PLASTER, 0.09)
    add_box("dispatch_office_roof", (-12.8, -0.8, 5.72), (14.0, 16.6, 0.42), OFFICE_ROOF, 0.08)
    add_box("office_roof_edge_front", (-12.8, -9.05, 5.72), (14.0, 0.28, 0.65), FRAME, 0.05)
    add_box("office_roof_edge_left", (-19.82, -0.8, 5.72), (0.28, 16.4, 0.65), FRAME, 0.04)
    add_box("office_roof_edge_right", (-5.78, -0.8, 5.72), (0.28, 16.4, 0.65), FRAME, 0.04)
    add_box("office_base_red_trim", (-12.8, -0.8, 0.94), (13.1, 15.7, 0.32), SAFETY_RED, 0.035)

    # Front facade, windows and entrance door.
    add_window("office_window_1", -17.0, 3.35, 2.35, 1.95)
    add_window("office_window_2", -13.35, 3.35, 2.35, 1.95)
    # Put the entrance toward the garage/road approach end of the frontage.
    entry_x = -7.65
    add_box("office_door", (entry_x, -8.81, 2.36), (1.65, 0.15, 3.18), STEEL, 0.04)
    add_box("office_door_panel", (entry_x, -8.91, 2.42), (1.16, 0.05, 2.68), FRAME, 0.025)
    add_box("office_door_handle", (entry_x + 0.48, -8.98, 2.25), (0.07, 0.05, 0.27), FRAME, 0.015)
    add_box("office_entry_canopy", (entry_x, -9.63, 4.20), (2.65, 1.0, 0.18), STEEL, 0.04)
    # The lowest tread sits furthest from the doorway; the stair rises toward it.
    for idx, step in enumerate((0.62, 0.40, 0.18)):
        add_box(f"office_entry_step_{idx+1}", (entry_x, -9.10 - idx * 0.47, step / 2), (2.3, 0.50, step), CONCRETE, 0.05)
    # A restrained red dispatch mark with three angled stripes.
    add_box("dispatch_mark", (-13.8, -8.93, 4.92), (5.5, 0.08, 0.34), SAFETY_RED, 0.03)
    for idx, x in enumerate((-11.0, -10.55, -10.10)):
        add_box(f"dispatch_mark_slash_{idx+1}", (x, -9.01, 4.92), (0.12, 0.04, 0.34), CONCRETE, 0.01)

    # Small chimney and a modest exhaust stack.
    add_box("office_chimney_lower", (-16.9, 3.7, 6.16), (1.25, 1.25, 0.75), PLASTER, 0.05)
    add_box("office_chimney_cap", (-16.9, 3.7, 6.58), (1.55, 1.55, 0.12), CONCRETE, 0.04)

    # Decorative vertical tank at the office's outer left edge. No pump or hose.
    add_box("tank_support_pad", (-21.35, -1.0, 0.27), (3.6, 3.6, 0.50), CONCRETE, 0.08)
    add_cylinder("decorative_tank_body", (-21.35, -1.0, 3.35), 1.38, 5.75, TANK, 20, 0.10)
    # Use a plain painted steel cap instead of projecting the side atlas across
    # the upward-facing disk; omit the protruding vent that read as a white knob.
    add_cylinder("tank_top_dome", (-21.35, -1.0, 6.28), 1.31, 0.22, TANK_TOP, 20, 0.08)
    for idx, x in enumerate((-22.15, -20.55)):
        add_box(f"tank_support_{idx+1}", (x, -1.0, 0.79), (0.22, 2.5, 1.0), STEEL, 0.035)

    # Open steel canopy. The medium layout extends the two-bay design with a
    # second gable module, keeping four equal openings side-by-side.
    canopy_modules = [(-5.6, 7.0, 19.6)]
    if MEDIUM:
        canopy_modules.append((19.6, 32.2, 44.8))
    post_xs = sorted({x for module in canopy_modules for x in module})
    y_front, y_back = -9.2, 9.7
    eave_z, ridge_z = 6.5, 7.25
    for side_name, y in (("front", y_front), ("back", y_back)):
        for idx, x in enumerate(post_xs):
            add_box(f"canopy_{side_name}_foot_{idx+1}", (x, y, 0.28), (1.05, 1.05, 0.56), CONCRETE, 0.08)
            add_box(f"canopy_{side_name}_post_{idx+1}", (x, y, 3.55), (0.48, 0.48, 6.05), STEEL, 0.035)
            add_box(f"canopy_{side_name}_safety_band_{idx+1}", (x, y - 0.26, 0.86), (0.50, 0.035, 0.80), SAFETY_RED, 0.015)
            # Two pale diagonal warning slashes on the post base.
            add_box(f"canopy_{side_name}_stripe_a_{idx+1}", (x - 0.10, y - 0.285, 0.86), (0.07, 0.025, 0.72), CONCRETE, 0.01).rotation_euler[1] = math.radians(25)
            add_box(f"canopy_{side_name}_stripe_b_{idx+1}", (x + 0.12, y - 0.285, 0.86), (0.07, 0.025, 0.72), CONCRETE, 0.01).rotation_euler[1] = math.radians(25)

    # Longitudinal roof rails follow every structural column line.
    for idx, x in enumerate(post_xs):
        add_beam(f"canopy_longitudinal_rail_{idx+1}", (x, y_front, eave_z), (x, y_back, eave_z), 0.33, 0.40, STEEL, 0.04)
    # Each pair of bays forms one pitched-roof module with a center ridge.
    for module_index, (x_left, x_mid, x_right) in enumerate(canopy_modules, start=1):
        for name, y in (("front", y_front), ("back", y_back)):
            prefix = f"module_{module_index}_{name}"
            add_beam(f"{prefix}_truss_eave_left", (x_left, y, eave_z), (x_mid, y, ridge_z), 0.22, 0.28, STEEL)
            add_beam(f"{prefix}_truss_eave_right", (x_mid, y, ridge_z), (x_right, y, eave_z), 0.22, 0.28, STEEL)
            add_beam(f"{prefix}_truss_bottom", (x_left, y, eave_z - 0.18), (x_right, y, eave_z - 0.18), 0.20, 0.22, STEEL)
            add_beam(f"{prefix}_truss_web_left", (x_left + 3.0, y, eave_z - 0.15), (x_mid, y, ridge_z - 0.25), 0.16, 0.18, STEEL)
            add_beam(f"{prefix}_truss_web_right", (x_mid, y, ridge_z - 0.25), (x_right - 3.0, y, eave_z - 0.15), 0.16, 0.18, STEEL)

    # A muted pitched roof. Its vanilla atlas texture carries the corrugation;
    # separate modeled ribs caused a doubled, bright striped appearance.
    roof_front = y_front - 0.35
    roof_back = y_back + 0.35
    for module_index, (x_left, x_mid, x_right) in enumerate(canopy_modules, start=1):
        for side, xa, xb, za, zb in (
            ("left", x_left - 0.45, x_mid, eave_z, ridge_z),
            ("right", x_mid, x_right + 0.45, ridge_z, eave_z),
        ):
            verts = [(xa, roof_front, za), (xb, roof_front, zb), (xb, roof_back, zb), (xa, roof_back, za)]
            mesh = bpy.data.meshes.new(f"canopy_roof_{module_index}_{side}_mesh")
            mesh.from_pydata(verts, [], [(0, 1, 2, 3)])
            mesh.materials.append(ROOF)
            roof = bpy.data.objects.new(f"canopy_roof_{module_index}_{side}", mesh)
            bpy.context.collection.objects.link(roof)
            EXPORT_OBJECTS.append(roof)

    # Parking markings and the two decorative center pads are intentionally
    # omitted; the native ground surface is provided by building metadata.


def make_large_building():
    """Articulated U-shaped vehicle yard: 16 covered and 4 open-air stalls."""
    def roof_panel(name, vertices, mat):
        mesh = bpy.data.meshes.new(name + "_mesh")
        mesh.from_pydata(vertices, [], [tuple(range(len(vertices)))])
        mesh.materials.append(mat)
        obj = bpy.data.objects.new(name, mesh)
        bpy.context.collection.objects.link(obj)
        EXPORT_OBJECTS.append(obj)

    # Rear operations building closes the U. A stepped dispatch office and a
    # pair of loading bays make it read as a working depot, not a blank box.
    add_box("large_service_plinth", (0.0, 27.0, 0.30), (57.0, 15.0, 0.56), CONCRETE, 0.08)
    add_box("large_service_hall", (0.0, 27.5, 3.85), (56.0, 13.8, 6.7), PLASTER, 0.07)
    add_box("large_dispatch_office", (23.0, 18.4, 3.5), (9.5, 4.0, 6.0), PLASTER, 0.05)
    add_box("large_service_roof", (-1.0, 27.4, 7.45), (55.0, 14.6, 0.34), OFFICE_ROOF, 0.05)
    add_box("large_office_roof", (23.0, 18.4, 6.7), (9.7, 4.2, 0.30), OFFICE_ROOF, 0.04)
    add_box("large_service_roof_front_edge", (-1.0, 19.98, 7.42), (55.0, 0.28, 0.42), FRAME, 0.03)
    for idx, x in enumerate((-15.0, 15.0), start=1):
        add_box(f"large_loading_door_{idx}_surround", (x, 20.42, 3.0), (7.8, 0.28, 5.5), FRAME, 0.04)
        add_box(f"large_loading_door_{idx}_panel", (x, 20.23, 3.0), (7.15, 0.08, 4.95), STEEL, 0.03)
        for stripe in range(1, 5):
            add_box(f"large_loading_door_{idx}_rib_{stripe}", (x, 20.15, 0.85 + stripe * 0.88), (6.95, 0.06, 0.06), FRAME, 0.01)
    for x in (-27.0, -7.0, 10.5):
        add_box(f"large_service_pier_{str(x).replace('-', 'm')}", (x, 20.23, 3.8), (0.55, 0.3, 6.3), CONCRETE, 0.03)

    # Dispatch frontage, with sheltered entrance and broad steps to the yard.
    add_box("large_entry_door", (23.0, 16.28, 2.25), (1.5, 0.14, 3.5), FRAME, 0.04)
    add_box("large_entry_door_inner", (23.0, 16.17, 2.25), (1.18, 0.08, 3.12), STEEL, 0.03)
    for idx, x in enumerate((18.2, 23.0, 27.7), start=1):
        add_window(f"large_dispatch_window_{idx}", x, 3.15, 2.4, 1.8, y=16.7)
    add_box("large_dispatch_awning", (23.0, 15.85, 4.25), (4.8, 1.0, 0.18), OFFICE_ROOF, 0.03)
    add_box("large_service_safety_band", (0.0, 20.10, 0.82), (55.0, 0.12, 0.28), SAFETY_RED, 0.02)
    for idx, (width, depth, height) in enumerate(((2.0, 0.55, 0.48), (2.4, 0.6, 0.34), (2.8, 0.65, 0.2))):
        add_box(f"large_entry_step_{idx+1}", (23.0, 15.4 - idx * 0.58, height / 2), (width, depth, height), CONCRETE, 0.04)

    # Two prominent garage wings form the U around a broad, unobstructed court.
    # Each wing has eight covered bays. The four remaining single-depth stalls
    # are open-air at the U mouth, as specified in the accepted layout.
    y_front, y_back = -11.0, 21.0
    y_centers = (-7.0, -3.0, 1.0, 5.0, 9.0, 13.0, 17.0, 21.0)
    for side, sign in (("left", -1), ("right", 1)):
        x_inner, x_outer = sign * 28.5, sign * 45.5
        x_mid = (x_inner + x_outer) / 2.0
        x_min, x_max = sorted((x_inner, x_outer))
        # Low, articulated rear walls with clerestory lights leave the garages
        # visually open while giving each wing a separate service-building form.
        add_box(f"large_{side}_rear_wall_base", (x_outer, 5.0, 1.05), (0.36, 32.0, 2.1), PLASTER, 0.03)
        for bay, y in enumerate(y_centers, start=1):
            add_box(f"large_{side}_rear_panel_{bay}", (x_outer, y, 3.15), (0.30, 3.25, 1.55), PLASTER, 0.02)
            add_box(f"large_{side}_clerestory_{bay}", (x_outer - sign * 0.19, y, 4.65), (0.08, 2.45, 0.65), GLASS, 0.01)
            add_box(f"large_{side}_rear_pilaster_{bay}", (x_outer - sign * 0.22, y + 1.85, 2.9), (0.16, 0.18, 5.6), FRAME, 0.02)
        add_box(f"large_{side}_rear_safety_band", (x_outer - sign * 0.21, 5.0, 0.62), (0.12, 31.8, 0.23), SAFETY_RED, 0.01)

        # Repeated steel portal frames articulate the stalls from the courtyard.
        ridge_x = sign * 37.0
        for bay, y in enumerate(y_centers, start=1):
            post_x = x_inner
            add_box(f"large_{side}_portal_post_{bay}", (post_x, y + 1.85, 2.9), (0.34, 0.34, 5.6), STEEL, 0.025)
            add_box(f"large_{side}_portal_foot_{bay}", (post_x, y + 1.85, 0.24), (0.78, 0.78, 0.48), CONCRETE, 0.04)
            add_box(f"large_{side}_portal_girder_{bay}", (x_mid, y + 1.85, 5.72), (17.0, 0.32, 0.34), STEEL, 0.025)
            add_beam(f"large_{side}_roof_truss_outer_{bay}", (x_outer, y + 1.85, 5.85), (ridge_x, y + 1.85, 7.02), 0.16, 0.18, STEEL)
            add_beam(f"large_{side}_roof_truss_inner_{bay}", (ridge_x, y + 1.85, 7.02), (x_inner, y + 1.85, 5.85), 0.16, 0.18, STEEL)
        add_box(f"large_{side}_rear_return", (x_mid, y_back, 3.0), (17.0, 0.36, 5.8), PLASTER, 0.04)

        # Pitched metal roofs split at the ridge so the wings look like garages
        # and respond to the light instead of reading as single flat slabs.
        eave_z, ridge_z = 5.95, 7.15
        for roof_side in ("outer", "inner"):
            if sign < 0:
                xa, xb = ((x_min - 0.35, ridge_x) if roof_side == "outer" else (ridge_x, x_max + 0.35))
            else:
                xa, xb = ((ridge_x, x_max + 0.35) if roof_side == "outer" else (x_min - 0.35, ridge_x))
            za = eave_z if abs(xa) > abs(xb) else ridge_z
            zb = eave_z if abs(xb) > abs(xa) else ridge_z
            roof_panel(f"large_{side}_{roof_side}_pitched_roof", [
                (xa, y_front - 0.35, za), (xb, y_front - 0.35, zb),
                (xb, y_back + 0.35, zb), (xa, y_back + 0.35, za),
            ], ROOF)
        add_box(f"large_{side}_ridge_cap", (ridge_x, 5.0, ridge_z + 0.08), (0.22, 32.8, 0.18), FRAME, 0.03)
        add_box(f"large_{side}_front_gutter", (x_mid, y_front - 0.22, eave_z + 0.03), (17.6, 0.18, 0.2), FRAME, 0.02)

    # Small utility annex and decorative tank distinguish the service yard.
    add_box("large_pump_house", (-22.0, 29.0, 2.3), (7.5, 7.0, 4.2), PLASTER, 0.05)
    add_box("large_pump_house_roof", (-22.0, 29.0, 4.48), (7.8, 7.3, 0.28), OFFICE_ROOF, 0.03)
    add_box("large_pump_house_door", (-22.0, 25.39, 1.55), (1.2, 0.12, 2.6), STEEL, 0.03)
    add_cylinder("large_decorative_fuel_tank", (-27.0, 31.0, 3.2), 1.65, 6.2, TANK, vertices=20)
    add_cylinder("large_decorative_tank_cap", (-27.0, 31.0, 6.35), 1.62, 0.18, FRAME, vertices=20, bevel=0.03)

    # Shipping containers stay outside the parking slots, court and loading approaches.
    for i, (x, y, mat) in enumerate(((-50.0, 9.0, CONTAINER_RED), (-50.0, 1.0, CONTAINER_BLUE), (50.0, 9.0, CONTAINER_RED)), start=1):
        add_box(f"large_container_{i}_body", (x, y, 1.3), (6.0, 2.45, 2.6), mat, 0.04)
        for rib in range(1, 8):
            xx = x - 2.8 + rib * 0.8
            add_box(f"large_container_{i}_rib_{rib}", (xx, y - 1.25, 1.3), (0.07, 0.07, 2.5), FRAME, 0.01)
        add_box(f"large_container_{i}_roof", (x, y, 2.63), (6.1, 2.5, 0.08), STEEL, 0.02)


# Material atlas regions used by vanilla building assets. Coordinates are pixel
# rectangles measured from the upper-left of each DDS; each entry records its
# source atlas dimensions because the vanilla sheets use different sizes.
TEXTURED_ATLAS_REGIONS = {
    # Pale, lightly mottled concrete field on the vanilla construction-office atlas.
    "dispatch_concrete": ("buildings/construction_office.dds", (430, 520, 640, 1008), 1024.0, 1024.0),
    # The construction-office warehouse atlas has the tighter, more subdued
    # galvanized roof used by the vanilla office reference.
    "corrugated_galvanized_roof": ("buildings/construction_office_warehouse.dds", (970, 30, 2000, 438), 2048.0, 2048.0),
    "office_roof_graphite": ("buildings/firestation.dds", (8, 458, 628, 640), 2048.0, 2048.0),
    # Weathered steel from the horizontal fuel tank on the vanilla construction-
    # office warehouse atlas. This matches the CO reference more closely than
    # the clean, pale explosive-factory tank texture.
    "decorative_tank_painted_steel": ("buildings/construction_office_warehouse.dds", (1228, 1238, 1678, 1480), 2048.0, 2048.0),
}

BAKED_ATLAS_TEXTURES = {
    "corrugated_galvanized_roof": "earthworks_corrugated_roof.dds",
    "decorative_tank_painted_steel": "earthworks_tank_steel.dds",
}


def bake_facility_textures():
    """Bake darker vanilla atlas patches into local, opaque BC1 textures."""
    media = ROOT.parent / "media_soviet" / "buildings"
    bake_patch(
        media / "construction_office_warehouse.dds",
        OUT / BAKED_ATLAS_TEXTURES["corrugated_galvanized_roof"],
        (970, 30, 2000, 438),
        (1.0, 1.0, 1.0),
        contrast=1.0,
    )
    bake_patch(
        media / "construction_office_warehouse.dds",
        OUT / BAKED_ATLAS_TEXTURES["decorative_tank_painted_steel"],
        (1228, 1238, 1678, 1480),
        (1.0, 1.0, 1.0),
        contrast=1.0,
    )


def apply_atlas_uvs(obj, material_name):
    """Map a grouped mesh into the relevant vanilla DDS atlas region."""
    _texture, (left, top, right, bottom), width, height = TEXTURED_ATLAS_REGIONS[material_name]
    bounds = obj.bound_box
    coords = [obj.matrix_world @ Vector(corner) for corner in bounds]
    minima = [min(point[axis] for point in coords) for axis in range(3)]
    maxima = [max(point[axis] for point in coords) for axis in range(3)]
    uv_layer = obj.data.uv_layers.active or obj.data.uv_layers.new(name="UVMap")

    for polygon in obj.data.polygons:
        normal = polygon.normal
        if abs(normal.z) >= max(abs(normal.x), abs(normal.y)):
            axis_u, axis_v = 0, 1
        elif abs(normal.x) >= abs(normal.y):
            axis_u, axis_v = 1, 2
        else:
            axis_u, axis_v = 0, 2
        if material_name in BAKED_ATLAS_TEXTURES:
            ranges = ((0.0, 1.0), (0.0, 1.0))
        else:
            ranges = ((left / width, right / width),
                      (1.0 - bottom / height, 1.0 - top / height))
        axes = (axis_u, axis_v)
        for loop_index in polygon.loop_indices:
            point = obj.matrix_world @ obj.data.vertices[obj.data.loops[loop_index].vertex_index].co
            uv = []
            for dim, axis in enumerate(axes):
                low, high = minima[axis], maxima[axis]
                t = 0.5 if high - low < 1e-6 else (point[axis] - low) / (high - low)
                uv.append(ranges[dim][0] + t * (ranges[dim][1] - ranges[dim][0]))
            uv_layer.data[loop_index].uv = uv


def set_camera_and_render():
    if LARGE:
        target = Vector((0.0, 0.0, 2.8))
        camera_location = (-34.0, -122.0, 82.0)
        ortho_scale = 132.0
    elif MEDIUM:
        target = Vector((12.0, 0.0, 2.3))
        camera_location = (-25.0, -68.0, 54.0)
        ortho_scale = 91.0
    else:
        target = Vector((-1.0, 0.0, 2.3))
        camera_location = (-34.0, -42.0, 32.0)
        ortho_scale = 57.0
    bpy.ops.object.camera_add(location=camera_location)
    camera = bpy.context.object
    camera.name = "preview_camera"
    camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = ortho_scale
    bpy.context.scene.camera = camera

    for name, loc, energy, size in (
        ("key", (-20, -25, 38), 8500, 18),
        ("fill", (18, -4, 23), 5200, 14),
        ("rim", (6, 24, 30), 6800, 12),
    ):
        bpy.ops.object.light_add(type="AREA", location=loc)
        light = bpy.context.object
        light.name = f"preview_{name}_light"
        light.data.energy = energy
        light.data.shape = "DISK"
        light.data.size = size
        light.rotation_euler = (Vector((0, 0, 2)) - light.location).to_track_quat("-Z", "Y").to_euler()

    bpy.ops.mesh.primitive_plane_add(size=200.0, location=(0, 0, -0.06))
    ground = bpy.context.object
    ground.name = "preview_only_ground"
    ground.data.materials.append(material("preview_ground", (0.22, 0.27, 0.19)))

    if LARGE:
        # The game paints these native ground areas from building.ini after
        # placement. Show them only in the proposal render so the yard reads
        # like the playable layout; they are intentionally excluded from OBJ.
        asphalt_preview = material("preview_native_asphalt", (0.105, 0.115, 0.13), 0.94)
        line_preview = material("preview_native_parking_lines", (0.78, 0.76, 0.68), 0.9)
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, -1.5, -0.005))
        court = bpy.context.object
        court.name = "preview_only_native_asphalt_court"
        court.dimensions = (92.0, 69.0, 0.05)
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        court.data.materials.append(asphalt_preview)
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, -40.5, 0.0))
        approach = bpy.context.object
        approach.name = "preview_only_native_road_apron"
        approach.dimensions = (12.0, 12.0, 0.05)
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        approach.data.materials.append(asphalt_preview)
        # Twenty individually readable, single-depth parking positions.
        for side, x in (("left", -36.0), ("right", 36.0)):
            for index, y in enumerate((-19.0, -15.0, -7.0, -3.0, 1.0, 5.0, 9.0, 13.0, 17.0, 21.0), start=1):
                for edge_x in (x - 5.0, x + 5.0):
                    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(edge_x, y, 0.028))
                    stripe = bpy.context.object
                    stripe.name = f"preview_only_{side}_parking_{index}_{edge_x}"
                    stripe.dimensions = (0.09, 3.05, 0.018)
                    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
                    stripe.data.materials.append(line_preview)
                for end_y in (y - 1.5, y + 1.5):
                    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(x, end_y, 0.028))
                    stripe = bpy.context.object
                    stripe.name = f"preview_only_{side}_parking_end_{index}_{end_y}"
                    stripe.dimensions = (10.0, 0.09, 0.018)
                    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
                    stripe.data.materials.append(line_preview)

    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.samples = 48
    scene.render.resolution_x = 2200 if LARGE else (1800 if MEDIUM else 1400)
    scene.render.resolution_y = 1350 if LARGE else 1050
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = str(PREVIEW)
    scene.world.use_nodes = True
    scene.world.node_tree.nodes.get("Background").inputs["Color"].default_value = (0.22, 0.24, 0.25, 1.0)
    scene.world.node_tree.nodes.get("Background").inputs["Strength"].default_value = 0.65
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.view_settings.exposure = 0.65
    scene.render.film_transparent = False
    scene.render.image_settings.color_mode = "RGBA"
    scene.camera.data.lens = 55
    bpy.ops.render.render(write_still=True)


def export_obj():
    # Keep the editable Blender scene organized into small named parts, but
    # export a compact set of material-grouped meshes for ModelViewer. The
    # game's model tool expects triangles and has a per-object vertex limit.
    by_material = {}
    export_copies = []
    for source in EXPORT_OBJECTS:
        bpy.ops.object.select_all(action="DESELECT")
        source.select_set(True)
        bpy.context.view_layer.objects.active = source
        bpy.ops.object.duplicate()
        copy = bpy.context.object
        copy.name = f"export_{source.name}"
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        if not copy.data.materials:
            continue
        key = copy.data.materials[0].name
        by_material.setdefault(key, []).append(copy)
        export_copies.append(copy)

    grouped = []
    for material_name, copies in by_material.items():
        bpy.ops.object.select_all(action="DESELECT")
        for obj in copies:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = copies[0]
        if len(copies) > 1:
            bpy.ops.object.join()
        obj = bpy.context.object
        obj.name = f"mesh_{material_name}"
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.0001)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
        obj.data.update()
        if material_name == "decorative_tank_painted_steel":
            apply_cylinder_atlas_uvs(obj, material_name, (-21.35, -1.0))
        elif material_name in TEXTURED_ATLAS_REGIONS:
            apply_atlas_uvs(obj, material_name)
        grouped.append(obj)

    bpy.ops.object.select_all(action="DESELECT")
    for obj in grouped:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = grouped[0]
    bpy.ops.wm.obj_export(
        filepath=str(OBJ),
        export_selected_objects=True,
        export_materials=True,
        forward_axis="NEGATIVE_Z",
        up_axis="Y",
        apply_modifiers=True,
    )
    # Blender writes LF on Windows while this repository's exported OBJ uses
    # CRLF. Preserve the established line endings so geometry diffs stay legible.
    exported_obj = OBJ.read_bytes().replace(b"\r\n", b"\n").replace(b"\n", b"\r\n")
    OBJ.write_bytes(exported_obj)

    bpy.ops.object.select_all(action="DESELECT")
    for obj in grouped:
        obj.select_set(True)
    bpy.ops.object.delete(use_global=False)


def main():
    bake_facility_textures()
    clear_scene()
    if LARGE:
        make_large_building()
    else:
        make_building()
    set_camera_and_render()
    export_obj()
    bpy.ops.wm.save_as_mainfile(filepath=str(BLEND))
    print(f"STAGE12_{VARIANT.upper()}_MODEL_OBJ={OBJ}")
    print(f"STAGE12_{VARIANT.upper()}_MODEL_PREVIEW={PREVIEW}")
    print(f"STAGE12_{VARIANT.upper()}_MODEL_BLEND={BLEND}")


if __name__ == "__main__":
    main()
