Branch: job/kimodo-helper
Status: active
Pull request: #175
Files:
- tools/kimodo/**
- tests/test_kimodo_helper.py
- .github/workflows/build.yml
- docs/TEXT_TO_MOTION_GUIDE.md
- README.md
- docs/research/TEXT_TO_MOTION.md
- docs/ROADMAP.md

## What

Optional Windows helper outside the program (tools/kimodo/) that sets up NVIDIA
Kimodo-SOMA on a computer with an NVIDIA RTX card, guides the user through the
Llama 3 licence and their own Hugging Face token, and makes a BVH from a text
prompt into the PinglyMotion work folder. Nothing of Kimodo or its weights is
committed or bundled. Part of issue 18 (job 3 of docs/research/TEXT_TO_MOTION.md).
The real run needs the project owner's GPU test.
