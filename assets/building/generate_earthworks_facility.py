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
    """Large U-shaped depot closely following the accepted five-view concept."""
    def roof_panel(name, vertices, mat):
        mesh = bpy.data.meshes.new(name + "_mesh")
        mesh.from_pydata(vertices, [], [tuple(range(len(vertices)))])
        mesh.materials.append(mat)
        obj = bpy.data.objects.new(name, mesh)
        bpy.context.collection.objects.link(obj)
        EXPORT_OBJECTS.append(obj)

    # The accepted concept is a clearly legible U: a continuous service block
    # closes the back, with the office centered between two loading doors.
    # Native asphalt and parking markings remain building metadata, not mesh.
    add_box("large_service_plinth", (0.0, 28.2, 0.28), (75.0, 17.0, 0.5), CONCRETE, 0.08)
    add_box("large_service_hall", (0.0, 29.0, 3.55), (74.0, 15.0, 6.1), PLASTER, 0.07)
    # Keep the wings of the rear hall single-storey, while a narrower raised
    # center section gives the main building a clear two-storey silhouette.
    for idx, x in enumerate((-24.95, 24.95), start=1):
        add_box(f"large_service_roof_wing_{idx}", (x, 29.0, 6.72), (25.5, 16.0, 0.34), OFFICE_ROOF, 0.06)
        add_box(f"large_service_roof_front_edge_{idx}", (x, 20.85, 6.72), (25.3, 0.24, 0.38), FRAME, 0.025)
    add_box("large_service_upper_floor", (0.0, 29.0, 7.9), (24.0, 15.0, 3.4), PLASTER, 0.05)
    add_box("large_service_upper_front_band", (0.0, 21.38, 6.45), (23.9, 0.12, 0.25), SAFETY_RED, 0.015)
    add_box("large_service_upper_roof", (0.0, 29.0, 9.68), (24.8, 16.0, 0.36), OFFICE_ROOF, 0.04)
    add_box("large_service_upper_roof_front_edge", (0.0, 20.85, 9.68), (24.6, 0.24, 0.4), FRAME, 0.025)
    add_box("large_service_safety_band", (0.0, 21.38, 0.80), (73.9, 0.12, 0.28), SAFETY_RED, 0.02)

    # Front elevation: two broad machinery doors flank a projecting, glazed
    # dispatch office. Its centered entry and steps face the open yard.
    for idx, x in enumerate((-15.0, 15.0), start=1):
        add_box(f"large_loading_door_{idx}_surround", (x, 21.43, 3.0), (8.2, 0.28, 5.25), FRAME, 0.04)
        add_box(f"large_loading_door_{idx}_panel", (x, 21.24, 3.0), (7.55, 0.08, 4.88), STEEL, 0.03)
        for stripe in range(1, 5):
            add_box(f"large_loading_door_{idx}_rib_{stripe}", (x, 21.15, 0.9 + stripe * 0.84), (7.25, 0.05, 0.055), FRAME, 0.008)

    add_box("large_dispatch_plinth", (0.0, 19.15, 0.25), (20.5, 4.8, 0.42), CONCRETE, 0.06)
    add_box("large_dispatch_office", (0.0, 19.2, 3.5), (19.6, 4.3, 6.1), PLASTER, 0.05)
    add_box("large_office_roof", (0.0, 19.2, 6.68), (20.3, 4.8, 0.3), OFFICE_ROOF, 0.04)
    add_box("large_office_safety_band", (0.0, 16.99, 0.86), (19.5, 0.1, 0.27), SAFETY_RED, 0.015)
    add_box("large_entry_door", (0.0, 16.96, 2.2), (1.65, 0.13, 3.4), FRAME, 0.035)
    add_box("large_entry_door_inner", (0.0, 16.86, 2.2), (1.28, 0.06, 3.05), STEEL, 0.02)
    for idx, x in enumerate((-7.0, -3.5, 3.5, 7.0), start=1):
        add_window(f"large_dispatch_window_{idx}", x, 3.45, 2.2, 1.65, y=16.88)
    add_box("large_dispatch_awning", (0.0, 16.52, 4.15), (5.0, 0.85, 0.18), OFFICE_ROOF, 0.03)
    for idx, (width, depth, height) in enumerate(((2.0, 0.5, 0.48), (2.4, 0.55, 0.33), (2.8, 0.6, 0.18))):
        add_box(f"large_entry_step_{idx+1}", (0.0, 16.28 - idx * 0.5, height / 2), (width, depth, height), CONCRETE, 0.035)

    # Four modest roof vents follow the rear building rhythm in the reference.
    for idx, x in enumerate((-27.0, -17.0, 17.0, 27.0), start=1):
        add_box(f"large_service_vent_{idx}_stem", (x, 29.0, 7.22), (0.48, 0.48, 0.62), STEEL, 0.03)
        add_box(f"large_service_vent_{idx}_cap", (x, 29.0, 7.57), (0.9, 0.75, 0.14), OFFICE_ROOF, 0.025)
    # Repeating rear-facing industrial windows keep the back elevation from
    # reading as an unbroken wall in the rear and oblique concept views.
    for idx, x in enumerate((-28.0, -14.0, 14.0, 28.0), start=1):
        add_window(f"large_rear_window_{idx}", x, 3.6, 5.0, 1.35, y=36.62)
    for idx, x in enumerate((-6.0, 6.0), start=1):
        add_window(f"large_upper_front_window_{idx}", x, 8.15, 2.6, 1.2, y=21.20)
        add_window(f"large_upper_rear_window_{idx}", x, 8.15, 2.6, 1.2, y=36.62)

    # Side sheds run forward from the service block and face the central court.
    # Each gets eight covered stalls; two more stalls at the mouth remain open
    # air. Longer wings and a deeper court now match the concept proportions.
    y_front, y_back = -38.0, 21.5
    wing_length = y_back - y_front
    wing_center_y = (y_back + y_front) / 2.0
    stall_centers = (-33.0, -26.0, -19.0, -12.0, -5.0, 2.0, 9.0, 16.0)
    for side, sign in (("left", -1), ("right", 1)):
        x_inner, x_outer = sign * 20.0, sign * 37.0
        x_min, x_max = sorted((x_inner, x_outer))
        x_mid = (x_inner + x_outer) / 2.0
        # Solid outside wall with a muted red plinth strip, like the concept.
        add_box(f"large_{side}_outer_wall", (x_outer, wing_center_y, 2.65), (0.36, wing_length, 5.3), PLASTER, 0.035)
        add_box(f"large_{side}_outer_safety_band", (x_outer - sign * 0.2, wing_center_y, 0.63), (0.1, wing_length - 0.1, 0.25), SAFETY_RED, 0.012)
        # Thin wall pilasters add visible bay rhythm without blocking access.
        frame_positions = (-38.0, -30.5, -23.0, -15.5, -8.0, -0.5, 7.0, 14.5, 22.0)
        for idx, y in enumerate(frame_positions, start=1):
            add_box(f"large_{side}_outer_pilaster_{idx}", (x_outer - sign * 0.23, y, 2.7), (0.16, 0.2, 5.4), FRAME, 0.02)
        # Open courtyard side: steel columns frame the individual vehicle bays.
        for idx, y in enumerate(frame_positions, start=1):
            add_box(f"large_{side}_court_post_{idx}", (x_inner, y, 2.85), (0.36, 0.36, 5.7), STEEL, 0.025)
            add_box(f"large_{side}_court_foot_{idx}", (x_inner, y, 0.24), (0.68, 0.68, 0.48), CONCRETE, 0.035)
            add_box(f"large_{side}_post_safety_{idx}", (x_inner - sign * 0.2, y, 0.82), (0.05, 0.06, 0.72), SAFETY_RED, 0.01)
        # End wall frames the opening and distinguishes the covered bays from
        # the two open-air positions in front of it.
        add_box(f"large_{side}_mouth_end_wall", (x_mid, y_front, 2.75), (17.3, 0.3, 5.5), PLASTER, 0.04)
        add_box(f"large_{side}_mouth_safety_band", (x_mid, y_front - 0.18, 0.63), (17.1, 0.08, 0.24), SAFETY_RED, 0.01)
        add_box(f"large_{side}_end_door_surround", (x_mid, y_front - 0.19, 2.85), (11.2, 0.12, 4.55), FRAME, 0.035)
        add_box(f"large_{side}_end_door_panel", (x_mid, y_front - 0.27, 2.85), (10.55, 0.07, 4.1), STEEL, 0.025)
        for stripe in range(1, 6):
            add_box(f"large_{side}_end_door_rib_{stripe}", (x_mid, y_front - 0.32, 0.95 + stripe * 0.63), (10.25, 0.035, 0.045), FRAME, 0.006)
        # Shallow gable roof, dark like the accepted mockup. Structural portal
        # ties are repeated over the covered run, not over the open-air stalls.
        ridge_x = sign * 28.5
        eave_z, ridge_z = 5.75, 6.55
        for idx, y in enumerate(frame_positions, start=1):
            add_beam(f"large_{side}_roof_tie_{idx}", (x_inner, y, eave_z), (x_outer, y, eave_z), 0.22, 0.24, STEEL, 0.02)
            add_beam(f"large_{side}_roof_rafter_inner_{idx}", (x_inner, y, eave_z), (ridge_x, y, ridge_z), 0.18, 0.2, STEEL, 0.02)
            add_beam(f"large_{side}_roof_rafter_outer_{idx}", (ridge_x, y, ridge_z), (x_outer, y, eave_z), 0.18, 0.2, STEEL, 0.02)
        for roof_side, xa, xb, za, zb in (
            ("inner", x_min - 0.25, ridge_x, eave_z, ridge_z),
            ("outer", ridge_x, x_max + 0.25, ridge_z, eave_z),
        ):
            roof_panel(f"large_{side}_{roof_side}_pitched_roof", [
                (xa, y_front - 0.25, za), (xb, y_front - 0.25, zb),
                (xb, y_back + 0.25, zb), (xa, y_back + 0.25, za),
            ], OFFICE_ROOF)
        add_box(f"large_{side}_ridge_cap", (ridge_x, wing_center_y, ridge_z + 0.06), (0.18, wing_length + 0.6, 0.14), FRAME, 0.02)
        add_box(f"large_{side}_inner_gutter", (x_inner, wing_center_y, eave_z + 0.04), (0.2, wing_length + 0.6, 0.18), FRAME, 0.02)

    # Two shipping containers sit along the exterior side of the rear wing,
    # parallel to that wall and clear of the court and garage entries.
    for idx, (x, y, mat) in enumerate(((-40.0, 26.0, CONTAINER_RED), (-40.0, 33.0, CONTAINER_BLUE)), start=1):
        add_box(f"large_container_{idx}_body", (x, y, 1.3), (2.45, 6.0, 2.6), mat, 0.04)
        for rib in range(1, 8):
            yy = y - 2.8 + rib * 0.8
            add_box(f"large_container_{idx}_rib_{rib}", (x - 1.25, yy, 1.3), (0.07, 0.07, 2.5), FRAME, 0.01)
        add_box(f"large_container_{idx}_roof", (x, y, 2.63), (2.5, 6.1, 0.08), STEEL, 0.02)


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
        target = Vector((0.0, -14.0, 2.8))
        camera_location = (-40.0, -154.0, 96.0)
        ortho_scale = 155.0
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
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, -14.0, -0.005))
        court = bpy.context.object
        court.name = "preview_only_native_asphalt_court"
        court.dimensions = (75.0, 94.0, 0.05)
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        court.data.materials.append(asphalt_preview)
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, -66.0, 0.0))
        approach = bpy.context.object
        approach.name = "preview_only_native_road_apron"
        approach.dimensions = (12.0, 20.0, 0.05)
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        approach.data.materials.append(asphalt_preview)
        # Twenty individually readable, single-depth parking positions.
        for side, x in (("left", -36.0), ("right", 36.0)):
            for index, y in enumerate((-54.0, -47.0, -33.0, -26.0, -19.0, -12.0, -5.0, 2.0, 9.0, 16.0), start=1):
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
