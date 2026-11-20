"""
The evaluation set: plain-word prompts, an acceptance check for each, and a reference solution for
each (part of issue 129).

A case has
  id          short name, also the name of the saved file (<id>.dingle)
  prompt      what a beginner would type, in plain words
  setup       optional: commands that make the file the prompt talks about (saved as "start")
  start       the file name of that file; the prompt names it
  reference   the commands that solve the prompt (a scripted "perfect assistant")
  checks      acceptance checks on the saved <id>.dingle (see lib.run_check)
  broken_start  True when the start file must FAIL the checks (a "fix this" prompt)

Frames: frame 1 is the reference pose and cannot be keyed; 1 second at 30 per second is frame 31.
Angles are [x, y, z] degrees as set_pose takes them; docs/ASSISTANT_GUIDE.md has the calibration
table the numbers come from.
"""


def frames(count, fps=30):
    return [("set_fps", {"fps": fps}), ("set_frame_count", {"frames": count})]


def key(frame, **joints):
    return ("set_pose", {"frame": frame, "joints": {name: list(angles) for name, angles in joints.items()}})


def hip(frame, y, x=0, z=0):
    """A hip position key alone, without a hip rotation key at that frame (issue 188)."""
    return [("set_position", {"frame": frame, "x": x, "y": y, "z": z})]


def opening(case):
    """The commands that open the start file of a case."""
    if not case.get("start"):
        return []
    return [("open_file", {"path": case["start"]})]


NO_ERRORS = {"type": "check_animation"}

CASES = [
    {
        "id": "bow",
        "prompt": "Make a polite bow: bend forward from the waist, hold for a moment and stand up again. About two seconds.",
        "reference": frames(61) + [
            key(2, abdomen=(0, 0, 0), chest=(0, 0, 0), neck=(0, 0, 0)),
            key(26, abdomen=(40, 0, 0), chest=(15, 0, 0), neck=(15, 0, 0)),
            key(41, abdomen=(40, 0, 0), chest=(15, 0, 0), neck=(15, 0, 0)),
            key(61, abdomen=(0, 0, 0), chest=(0, 0, 0), neck=(0, 0, 0)),
        ],
        "checks": [
            {"type": "duration", "seconds": [1.5, 3.0]},
            {"type": "pose", "expect": "head.z@33 > head.z@2 + 8"},
            {"type": "pose", "expect": "head.y@33 < head.y@2 - 4"},
            {"type": "pose", "expect": "head.y@last > head.y@33 + 4"},
            {"type": "changed", "frame": 33, "joints": ["abdomen"]},
            NO_ERRORS,
        ],
    },
    {
        "id": "sit_chair",
        "prompt": "Make the avatar sit down on a chair, hands in the lap.",
        "reference": frames(31) + [
            key(31, lThigh=(-81.5, -0.6, 3.6), rThigh=(-80.5, 0.6, -3.6), lShin=(78.8, -1.4, -5), rShin=(77.8, 1.4, 5),
                lShldr=(3.7, -8, -54.4), rShldr=(3.7, 8, 54.4), lForeArm=(20, -77, 0), rForeArm=(20, 77, 0)),
            *hip(31, 28, 0, -8),
        ],
        "checks": [
            {"type": "duration", "seconds": [0.5, 4.0]},
            {"type": "pose", "expect": "hip.y@last < 32"},
            {"type": "pose", "expect": "hip.y@last > 20"},
            {"type": "pose", "expect": "lFoot.z@last > hip.z@last + 8"},
            {"type": "pose", "expect": "rFoot.z@last > hip.z@last + 8"},
            {"type": "pose", "expect": "lFoot.above_floor@last < 8"},
            {"type": "changed", "joints": ["lThigh", "rThigh", "lShin", "rShin"]},
            NO_ERRORS,
        ],
    },
    {
        "id": "wave_loop",
        "prompt": "Wave with the right hand for one second, and make it loop without a jump.",
        "reference": frames(31) + [
            key(2, rShldr=(0, 0, -70), rForeArm=(0, 40, 0)),
            key(16, rShldr=(0, 0, -70), rForeArm=(0, 90, 0)),
            # the default loop ends on the last frame (issue 189): the last key equal to the first
            # closes the loop
            key(31, rShldr=(0, 0, -70), rForeArm=(0, 40, 0)),
        ],
        "checks": [
            {"type": "duration", "seconds": [0.8, 1.3]},
            {"type": "pose", "expect": "rHand.y@2 > head.y@2"},
            {"type": "pose", "expect": "rHand.y@mid > head.y@mid"},
            {"type": "moves", "joints": ["rForeArm"]},
            {"type": "loop_seam", "allowed": ["smooth"]},
            NO_ERRORS,
        ],
    },
    {
        "id": "kneel",
        "prompt": "Make the avatar kneel on both knees, back straight.",
        "reference": frames(31) + [
            key(31, lShin=(140, 0, 0), rShin=(140, 0, 0)),
            *hip(31, 21, 0, 0),
        ],
        "checks": [
            {"type": "pose", "expect": "hip.y@last < 26"},
            {"type": "pose", "expect": "hip.y@last > 14"},
            {"type": "pose", "expect": "head.y@last > 40"},
            {"type": "pose", "expect": "lFoot.z@last < hip.z@last - 4"},
            {"type": "pose", "expect": "rFoot.z@last < hip.z@last - 4"},
            {"type": "changed", "joints": ["lShin", "rShin"]},
            NO_ERRORS,
        ],
    },
    {
        "id": "mirror_pose",
        "prompt": "Open the file mirror_pose_start.dingle. The right arm is raised. Make the left arm do the same, mirrored, and save it.",
        "start": "mirror_pose_start.dingle",
        "setup": frames(31) + [
            key(2, rShldr=(0, 0, -60), rForeArm=(0, 70, 0)),
            key(31, rShldr=(0, 0, -60), rForeArm=(0, 70, 0)),
        ],
        "reference": [
            key(2, lShldr=(0, 0, 60), lForeArm=(0, -70, 0)),
            key(31, lShldr=(0, 0, 60), lForeArm=(0, -70, 0)),
        ],
        "checks": [
            {"type": "pose", "expect": "lHand.y@last > rHand.y@last - 1"},
            {"type": "pose", "expect": "lHand.y@last < rHand.y@last + 1"},
            {"type": "pose", "expect": "lHand.z@last > rHand.z@last - 1"},
            {"type": "pose", "expect": "lHand.z@last < rHand.z@last + 1"},
            {"type": "pose", "expect": "lHand.x@last > ~rHand.x@last - 1"},
            {"type": "pose", "expect": "lHand.x@last < ~rHand.x@last + 1"},
            {"type": "pose", "expect": "rHand.y@last > head.y@last - 30"},
            {"type": "duration", "seconds": [0.8, 1.3]},
            NO_ERRORS,
        ],
        "broken_start": True,
    },
    {
        "id": "both_arms_up",
        "prompt": "Raise both arms straight up over the head.",
        "reference": frames(31) + [
            key(31, lShldr=(0, 0, 90), rShldr=(0, 0, -90)),
        ],
        "checks": [
            {"type": "pose", "expect": "lHand.y@last > head.y@last + 3"},
            {"type": "pose", "expect": "rHand.y@last > head.y@last + 3"},
            {"type": "pose", "expect": "lHand.y@last > rHand.y@last - 1"},
            {"type": "pose", "expect": "lHand.y@last < rHand.y@last + 1"},
            {"type": "changed", "joints": ["lShldr", "rShldr"]},
            NO_ERRORS,
        ],
    },
    {
        "id": "fix_slide",
        "prompt": "In fix_slide_start.dingle the left foot slides along the floor while the avatar stands in place. Fix it and keep the arm movement. Save the result.",
        "start": "fix_slide_start.dingle",
        "setup": frames(31) + [
            key(2, lThigh=(-12, 0, 0), rShldr=(0, 0, 0)),
            key(16, lThigh=(12, 0, 0), rShldr=(0, 0, -50)),
            key(31, lThigh=(-12, 0, 0), rShldr=(0, 0, 0)),
        ],
        "reference": [
            key(2, lThigh=(0, 0, 0)),
            key(16, lThigh=(0, 0, 0)),
            key(31, lThigh=(0, 0, 0)),
        ],
        "checks": [
            {"type": "stays", "point": "lFoot", "within": 1.0},
            {"type": "slide", "max_speed": 2.0},
            {"type": "moves", "joints": ["rShldr"]},
            {"type": "pose", "expect": "rHand.y@16 > rHand.y@2 + 10"},
            {"type": "duration", "seconds": [0.8, 1.3]},
            NO_ERRORS,
        ],
        "broken_start": True,
    },
    {
        "id": "nod_yes",
        "prompt": "Nod the head yes three times in about two seconds.",
        "reference": frames(62) + [
            key(2, head=(0, 0, 0)),
            key(12, head=(20, 0, 0)),
            key(22, head=(0, 0, 0)),
            key(32, head=(20, 0, 0)),
            key(42, head=(0, 0, 0)),
            key(52, head=(20, 0, 0)),
            key(62, head=(0, 0, 0)),
        ],
        "checks": [
            {"type": "duration", "seconds": [1.5, 2.6]},
            {"type": "angle", "joint": "head", "axis": "x", "frame": 12, "min": 10, "max": 45},
            {"type": "angle", "joint": "head", "axis": "x", "frame": 22, "min": -3, "max": 3},
            {"type": "angle", "joint": "head", "axis": "y", "frame": 12, "min": -3, "max": 3},
            {"type": "moves", "joints": ["head"]},
            {"type": "repeats", "value": True},
            NO_ERRORS,
        ],
    },
    {
        "id": "lift_left_leg",
        "prompt": "Stand on the right leg and lift the left knee up in front.",
        "reference": frames(31) + [
            key(31, lThigh=(-80, 0, 0), lShin=(80, 0, 0)),
        ],
        "checks": [
            {"type": "pose", "expect": "lFoot.above_floor@last > 12"},
            {"type": "pose", "expect": "rFoot.above_floor@last < 6"},
            {"type": "pose", "expect": "lFoot.z@last > hip.z@last + 3"},
            {"type": "changed", "joints": ["lThigh", "lShin"]},
            NO_ERRORS,
        ],
    },
    {
        "id": "look_left",
        "prompt": "Turn the head to look to the avatar's left.",
        "reference": frames(31) + [
            key(31, head=(0, 40, 0)),
        ],
        "checks": [
            # the figure's own left (docs/ASSISTANT_GUIDE.md, calibration table: head Y plus turns the face left)
            {"type": "angle", "joint": "head", "axis": "y", "frame": "last", "min": 15, "max": 45},
            {"type": "angle", "joint": "head", "axis": "x", "frame": "last", "min": -8, "max": 8},
            {"type": "changed", "joints": ["head"]},
            NO_ERRORS,
        ],
    },
    {
        "id": "point_right",
        "prompt": "Point straight forward with the right arm.",
        "reference": frames(31) + [
            key(31, rShldr=(0, 90, 0)),
        ],
        "checks": [
            {"type": "pose", "expect": "rHand.z@last > hip.z@last + 15"},
            {"type": "pose", "expect": "rHand.y@last > 50"},
            {"type": "pose", "expect": "rHand.y@last < head.y@last"},
            {"type": "pose", "expect": "lHand.z@last < hip.z@last + 10"},
            NO_ERRORS,
        ],
    },
    {
        "id": "arms_relaxed",
        "prompt": "Let the arms hang down relaxed at the sides.",
        "reference": frames(31) + [
            key(31, lShldr=(-4, 4, -50), rShldr=(-4, -4, 50), lForeArm=(-4, -9, 0), rForeArm=(-4, 9, 0)),
        ],
        "checks": [
            {"type": "pose", "expect": "lHand.y@last < hip.y@last + 8"},
            {"type": "pose", "expect": "rHand.y@last < hip.y@last + 8"},
            {"type": "pose", "expect": "lHand.x@last < 22"},
            {"type": "pose", "expect": "rHand.x@last > -22"},
            {"type": "changed", "joints": ["lShldr", "rShldr"]},
            NO_ERRORS,
        ],
    },
    {
        "id": "jump",
        "prompt": "A small jump on the spot in one second: crouch, jump up, land.",
        "reference": frames(31) + [
            key(2, lThigh=(-40, 0, 0), rThigh=(-40, 0, 0), lShin=(70, 0, 0), rShin=(70, 0, 0)), *hip(2, 33),
            key(9, lThigh=(-40, 0, 0), rThigh=(-40, 0, 0), lShin=(70, 0, 0), rShin=(70, 0, 0)), *hip(9, 33),
            key(14, lThigh=(0, 0, 0), rThigh=(0, 0, 0), lShin=(0, 0, 0), rShin=(0, 0, 0)), *hip(14, 58),
            key(22, lThigh=(-40, 0, 0), rThigh=(-40, 0, 0), lShin=(70, 0, 0), rShin=(70, 0, 0)), *hip(22, 33),
            key(31, lThigh=(0, 0, 0), rThigh=(0, 0, 0), lShin=(0, 0, 0), rShin=(0, 0, 0)), *hip(31, 43.5),
        ],
        "checks": [
            {"type": "duration", "seconds": [0.8, 1.5]},
            {"type": "pose", "expect": "hip.y@14 > hip.y@2 + 12"},
            {"type": "pose", "expect": "lFoot.above_floor@14 > 8"},
            {"type": "pose", "expect": "rFoot.above_floor@14 > 8"},
            {"type": "pose", "expect": "hip.y@last < hip.y@14 - 8"},
            {"type": "airborne"},
            NO_ERRORS,
        ],
    },
    {
        "id": "turn_around",
        "prompt": "Turn the whole body around to face the other way, in one second.",
        "reference": frames(31) + [
            key(31, hip=(0, 180, 0)),
        ],
        "checks": [
            {"type": "duration", "seconds": [0.8, 1.5]},
            {"type": "angle", "joint": "hip", "axis": "y", "frame": "last", "min": 150, "abs": True},
            {"type": "angle", "joint": "hip", "axis": "y", "frame": "mid", "min": 60, "max": 120, "abs": True},
            NO_ERRORS,
        ],
    },
]
