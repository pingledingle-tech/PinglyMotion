"""
Golden clips: bundled motions whose description is known and sensible. Each clip must be described
the way a person watching it would describe it (describe_motion, describe_pose) and must pass
check_animation. If a describer changes and starts saying something silly about a clip we know,
this fails (part of issue 129).

Each entry: file (relative to the repository), checks (see lib.run_check).
mixamo_style_wave.bvh is left out on purpose: its skeleton is not a Second Life one, so the program
opens it as a raw skeleton and describe_motion rightly says that nothing moves.
"""

NO_ERRORS = {"type": "check_animation"}

GOLDEN = [
    {   # a person waving: the right arm does the work and the right hand gets above the head
        "file": "tests/motions/cmu/cmu_13_26_wave.bvh",
        "checks": [
            {"type": "moves", "joints": ["rShldr", "rHand"]},
            {"type": "motion", "expect": "joints.length > 8"},
            {"type": "pose_any", "any_of": ["rHand.y@f > head.y@f + 4"]},
            {"type": "pose", "expect": "lHand.y@2 < head.y@2"},
            NO_ERRORS,
        ],
    },
    {   # a greeting: the right hand and arm move, the feet stay where they are
        "file": "tests/motions/mesh2motion/m2m_greeting.bvh",
        "checks": [
            {"type": "moves", "joints": ["rShldr", "rHand"]},
            {"type": "motion", "expect": "hip.bob < 1"},
            {"type": "motion", "expect": "contact.both_feet_off_floor.length == 0"},
            NO_ERRORS,
        ],
    },
    {   # standing still, breathing: almost nothing moves and nothing fast
        "file": "tests/motions/mesh2motion/m2m_idle-a.bvh",
        "checks": [
            {"type": "motion", "expect": "joints.length < 5"},
            {"type": "motion", "expect": "hip.bob < 1"},
            {"type": "motion", "expect": "loop.seam == smooth"},
            NO_ERRORS,
        ],
    },
    {   # a walk: legs and arms in opposition, a weight shift, never both feet in the air
        "file": "tests/motions/mesh2motion/m2m_walk.bvh",
        "checks": [
            {"type": "moves", "joints": ["rThigh", "lThigh", "rShin", "lShin"]},
            {"type": "motion", "expect": "rhythm.legs == move in opposition"},
            {"type": "motion", "expect": "rhythm.arms == move in opposition"},
            {"type": "motion", "expect": "contact.both_feet_off_floor.length == 0"},
            NO_ERRORS,
        ],
    },
    {   # a jog: legs in opposition and both feet off the floor between steps
        "file": "tests/motions/mesh2motion/m2m_jog.bvh",
        "checks": [
            {"type": "motion", "expect": "rhythm.legs == move in opposition"},
            {"type": "airborne"},
            {"type": "motion", "expect": "hip.bob > 8"},
            NO_ERRORS,
        ],
    },
    {   # a jump: legs and arms together, in the air for a while, the hip bobs a lot
        "file": "tests/motions/mesh2motion/m2m_jump-2.bvh",
        "checks": [
            {"type": "motion", "expect": "rhythm.legs == move together"},
            {"type": "airborne"},
            {"type": "motion", "expect": "hip.bob > 10"},
            NO_ERRORS,
        ],
    },
    {   # a simple dance: arms and legs in opposition, the hip bobs a little
        "file": "tests/motions/mesh2motion/m2m_dance-simple.bvh",
        "checks": [
            {"type": "moves", "joints": ["rShin", "rThigh", "rShldr", "lShldr"]},
            {"type": "motion", "expect": "rhythm.legs == move in opposition"},
            {"type": "motion", "expect": "hip.bob > 1"},
            NO_ERRORS,
        ],
    },
    {   # sitting, hardly moving: nothing moves and the legs are raised in front of the body
        "file": "tests/motions/mesh2motion/m2m_sitting-idle.bvh",
        "checks": [
            {"type": "motion", "expect": "joints.length == 0"},
            {"type": "pose", "expect": "lFoot.above_floor@mid > 15"},
            NO_ERRORS,
        ],
    },
    {   # the bundled jump example (.dingle): both feet leave the floor and the hip goes high
        "file": "examples/jump.dingle",
        "checks": [
            {"type": "airborne"},
            {"type": "motion", "expect": "hip.bob > 30"},
            {"type": "motion", "expect": "rhythm.legs == move together"},
            NO_ERRORS,
        ],
    },
    {   # the bundled throne example: seated all the way, hip low, nothing moves
        "file": "examples/throne.dingle",
        "checks": [
            {"type": "pose", "expect": "hip.y@mid < 32"},
            {"type": "motion", "expect": "joints.length == 0"},
            NO_ERRORS,
        ],
    },
]
