# Kimodo helper: text-to-motion on your own NVIDIA card (optional)

This folder is **not part of PinglyMotion** and not in its Windows zip. It is a small helper that sets up NVIDIA's Kimodo motion model on a Windows computer with an NVIDIA RTX graphics card, and makes a BVH animation from a sentence such as "wave with the right hand". The BVH lands in the PinglyMotion work folder (`Documents\PinglyMotion`), and PinglyMotion opens it with **File > Import BVH from another skeleton**, which knows Kimodo's SOMA skeleton (see [RETARGET_MAPPING.md](../../docs/RETARGET_MAPPING.md)).

Beginner guide: [docs/TEXT_TO_MOTION_GUIDE.md](../../docs/TEXT_TO_MOTION_GUIDE.md). Background and the licence study: [docs/research/TEXT_TO_MOTION.md](../../docs/research/TEXT_TO_MOTION.md) (issue 18).

State (20 November 2026): written and tested without a graphics card (`tests/test_kimodo_helper.py`); the first real run on an RTX card is the project owner's test.

## Files

| File | What it does |
|---|---|
| `PinglyMotion-Kimodo.bat` | Double-click to start. Runs `kimodo_setup.ps1` with all arguments. |
| `kimodo_setup.ps1` | Gets the `uv` installer (pinned, checked by SHA-256), makes a private Python 3.11, then starts `kimodo_helper.py`. |
| `kimodo_helper.py` | The helper itself (Python standard library only): `check`, `login`, `install`, `make`, `menu`, `forget-token`, `where`. |

Started with no arguments it walks through everything: check the computer, Meta's licence and your token, install, then asks "Motion:" and "Length in seconds:" again and again. From a command prompt: `PinglyMotion-Kimodo.bat make "wave with the right hand" 4s` (options: `--name`, `--seed`, `--constraints <file saved from Kimodo's own demo>`).

## Exact versions

| Part | Version |
|---|---|
| Kimodo code | `nv-tlabs/kimodo` commit `e1c219c99b4b94ba48be1fb6a1cbc64ecee4c954` (15 October 2026), installed from GitHub's archive of that commit |
| Model | `Kimodo-SOMA-RP-v1.2` (Hugging Face `nvidia/Kimodo-SOMA-RP-v1.2`), downloaded by Kimodo on first use |
| Text encoder | LLM2Vec on Meta Llama 3 8B Instruct (`McGill-NLP/LLM2Vec-Meta-Llama-3-8B-Instruct-mntp` and `-supervised`, base `meta-llama/Meta-Llama-3-8B-Instruct`), downloaded by Kimodo on first use |
| PyTorch | `torch==2.8.0` from the CUDA 12.6 index (`download.pytorch.org/whl/cu126`); CUDA 12.6 builds still include RTX 20xx (Turing) cards |
| Python | 3.11, a private copy made by uv |
| uv | 0.13.9, the Windows wheel from PyPI, SHA-256 `6a44b2ed8111d2f60d2b92627c5dccda3f7ed7570b18ce115e569cd68c28246b` |

Kimodo writes the BVH with `--bvh --bvh_standard_tpose` (rest pose = Kimodo's standard T-pose, the closest to Second Life's T-pose) at 30 frames per second, at most 10 seconds per motion.

## Why this route (a private Python, not Docker)

Kimodo's README suggests Docker for Windows. On a home PC that means Docker Desktop, WSL 2, GPU support in WSL and an NVIDIA PyTorch image of many gigabytes, plus a separate text-encoder service; each is a place where a beginner gets stuck. A private Python made by `uv` needs no administrator rights, no Docker and no WSL, installs nothing for the whole computer, and is removed by deleting one folder. The cost: Kimodo was developed on Linux and its README only says Windows "should work"; the owner's test is the first real run on Windows.

## What it needs

- 64-bit Windows 10 or 11, an NVIDIA graphics card with at least 6 GB (an RTX 2080 has 8 GB, a 2080 Ti 11 GB) and an NVIDIA driver 528 or newer.
- **At least 20 GB of RAM, 32 GB is better.** Low-VRAM mode puts the text encoder (Llama 3 8B, bfloat16, about 16 GB) on the processor; the graphics card then needs under 3 GB (Kimodo README).
- About 40 GB of free disk space: PyTorch about 3 GB, the text encoder about 16 GB, the rest small.
- A free Hugging Face account that has accepted Meta's Llama 3 licence, and a "Read" token.

## Where things go

- Helper folder: `%LOCALAPPDATA%\PinglyMotion\kimodo` (uv, Python, PyTorch, Kimodo, the downloaded models in `huggingface\`, and the token in `hf_token`). Delete this folder to remove everything.
- BVH files: `Documents\PinglyMotion\kimodo_<words>.bvh`; an existing file is never replaced (`_2`, `_3` …).

## The Hugging Face token

- Read without echo, checked against Hugging Face (`HEAD` of a small file of the gated Llama 3 model: 401 = wrong token, 403 = licence not accepted yet), and stored only in the helper folder (on Linux with permissions 600; on Windows the user's own `LOCALAPPDATA` is private to the user).
- Given to Kimodo only as the environment variable `HF_TOKEN` of its own process. Every line Kimodo or the installer prints goes through a filter that blanks out anything that looks like a token. The helper never prints, logs or uploads it, and never puts it in a file name or address.
- `PinglyMotion-Kimodo.bat forget-token` deletes it. You can also revoke it at any time on huggingface.co under Settings > Access Tokens.

## Licences (read them yourself before selling animations; this is not legal advice)

- **Kimodo code:** Apache-2.0.
- **Kimodo-SOMA models (`Kimodo-SOMA-RP-v1.2`):** NVIDIA Open Model License, per Kimodo's README at the pinned commit; the licence says the models are commercially usable and NVIDIA claims no ownership of the outputs ([licence text](https://www.nvidia.com/en-us/agreements/enterprise-software/nvidia-open-model-license/)). The Hugging Face model page could not be read from the session that wrote this; check it before relying on it.
- **Kimodo-SMPLX-RP-v1 is research only** (NVIDIA R&D Model license). This helper never uses it, and nobody should use it for animations that will be sold.
- **Meta Llama 3 8B Instruct** (the text encoder): Meta Llama 3 Community License, accepted by each user on Hugging Face. The helper does not ship or pass on Llama; each user downloads it with their own account. The text encoder only reads the sentence; the motion comes from the Kimodo model.
- **BONES-SEED dataset:** not needed and not downloaded.
- **Training data:** Kimodo-SOMA-RP was trained on Bones Rigplay 1 (about 700 hours of optical motion capture), which Kimodo's README calls commercially friendly.

Nothing of Kimodo, its models or its output is committed to this repository or bundled with PinglyMotion.

## Key poses from a .dingle as constraints (later)

Kimodo accepts full-body keyframe constraints, but as local rotations of the SOMA skeleton (axis-angle, metres, its own rest pose) in a JSON file. Turning a PinglyMotion `.dingle`'s Second Life key poses into that needs the retargeting in the other direction (Second Life to SOMA), which PinglyMotion does not have yet. Until then `--constraints` passes a file saved from Kimodo's own demo straight through. A `.dingle`-to-constraints export is a later job.

## For developers

- `python3 tests/test_kimodo_helper.py` (runs in CI): arguments, length, file names, the token never in the output, a whole `make` with a stand-in for Kimodo.
- Test-only environment variables: `PINGLYMOTION_KIMODO_HOME` (helper folder), `PINGLYMOTION_CONTROL_FOLDER` (work folder, the same variable as the program's remote control), `PINGLYMOTION_KIMODO_GENERATOR` (a script that stands in for Kimodo).
- When you change a version, change it in `kimodo_helper.py` or `kimodo_setup.ps1` and in the table above (the test checks the table).
