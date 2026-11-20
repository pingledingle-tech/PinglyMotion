"""Round trip a PinglyMotion BVH through Blender, then compare.

NOT RUN IN CI. Blender is not installed on the build machines, so this file is
only checked by hand. It was written from the Blender Python API
documentation and the settings in docs/BLENDER.md, and has not been tested in
Blender yet. The comparer it uses (bvh_compare.py, next to this file) needs no
Blender and is tested in CI (tests/test_bvh_compare.py).

What it does, inside Blender, with the settings docs/BLENDER.md prescribes:
1. starts an empty scene and sets the frame rate to the rate of the BVH file
2. imports the BVH (Forward -Z, Up Y, Scale 1.0, each joint keeps its own
   rotation order, scene frame rate and length follow the file)
3. exports it again as BVH (Scale 1.0, own rotation order per joint, only the
   hip gets position channels, whole frame range)
4. with --gltf also exports a glTF binary (.glb) with animation, if this
   Blender has the glTF exporter (it is part of Blender)
5. with --check compares the original and the new BVH with bvh_compare.py and
   exits with 1 when they differ

Usage (the second "--" ends Blender's own options):
  blender -b --python tools/blender/pinglymotion_roundtrip.py -- IN.bvh OUT.bvh [--gltf OUT.glb] [--check] [--tol 0.001]

Joint names are not changed by this script, so SL joint names stay as they are.
Blender adds nothing to bone names on a plain import and export.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import bvh_compare  # noqa: E402  (standard library only)


def parse_args(argv):
    """Arguments after the last "--". Kept simple so it also runs without Blender."""
    if "--" in argv:
        argv = argv[argv.index("--") + 1:]
    opts = {"in": None, "out": None, "gltf": None, "check": False, "tol": 0.001}
    free = []
    i = 0
    while i < len(argv):
        a = argv[i]
        if a == "--gltf" and i + 1 < len(argv):
            opts["gltf"] = argv[i + 1]
            i += 1
        elif a == "--tol" and i + 1 < len(argv):
            opts["tol"] = float(argv[i + 1])
            i += 1
        elif a == "--check":
            opts["check"] = True
        elif a.startswith("--"):
            raise SystemExit("unknown option: " + a)
        else:
            free.append(a)
        i += 1
    if len(free) != 2:
        raise SystemExit("usage: blender -b --python pinglymotion_roundtrip.py -- IN.bvh OUT.bvh [--gltf OUT.glb] [--check] [--tol N]")
    opts["in"], opts["out"] = free
    return opts


def main():
    opts = parse_args(sys.argv)
    src = os.path.abspath(opts["in"])
    dst = os.path.abspath(opts["out"])

    original = bvh_compare.load(src)  # also fails early on a file that is not BVH
    fps = 1.0 / original.frame_time
    print("input: %d joints, %d frames, %.3f fps" % (len(original.joints), original.frames, fps))

    import bpy  # only exists inside Blender

    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.render.fps = int(round(fps))
    scene.render.fps_base = 1.0

    bpy.ops.import_anim.bvh(
        filepath=src,
        axis_forward="-Z",
        axis_up="Y",
        global_scale=1.0,
        frame_start=1,
        use_fps_scale=False,
        update_scene_fps=True,
        update_scene_duration=True,
        rotate_mode="NATIVE",
    )

    arm = [o for o in bpy.data.objects if o.type == "ARMATURE"]
    if len(arm) != 1:
        raise SystemExit("expected one armature after import, found %d" % len(arm))
    arm = arm[0]
    bpy.ops.object.select_all(action="DESELECT")
    arm.select_set(True)
    bpy.context.view_layer.objects.active = arm

    bpy.ops.export_anim.bvh(
        filepath=dst,
        global_scale=1.0,
        frame_start=scene.frame_start,
        frame_end=scene.frame_end,
        rotate_mode="NATIVE",
        root_transform_only=True,
    )
    print("wrote " + dst)

    if opts["gltf"]:
        if hasattr(bpy.ops.export_scene, "gltf"):
            bpy.ops.export_scene.gltf(
                filepath=os.path.abspath(opts["gltf"]),
                export_format="GLB",
                use_selection=True,
                export_animations=True,
                export_yup=True,
            )
            print("wrote " + opts["gltf"])
        else:
            print("this Blender has no glTF exporter, glTF skipped")

    if opts["check"]:
        diffs = bvh_compare.compare(original, bvh_compare.load(dst), opts["tol"])
        if diffs:
            print("ROUND TRIP DIFFERENT")
            for d in diffs:
                print("  " + d)
            sys.exit(1)
        print("ROUND TRIP SAME")


if __name__ == "__main__":
    main()
