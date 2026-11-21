# How the 3D view is drawn

The 3D view (`AnimationView`) draws everything with **OpenGL 3.3 core shaders**. The old fixed-function OpenGL from QAvimator was replaced step by step in #61 (pull requests #72–#96). This page is for anyone who changes the 3D code.

## Requirement

- PinglyMotion asks for an OpenGL 3.3 core context with depth and stencil (`src/main.cpp`). Second Life itself needs OpenGL 3.2 or newer.
- Without OpenGL 3.3 the 3D view stays empty and a message says once that OpenGL 3.3 is needed. There is no old fallback path any more (#89).
- The log says what the view got, for example `3D view drawn with OpenGL 4.5 core shaders`.

## The parts

| File | What it does |
| --- | --- |
| `src/scenestate.h` | Drawing state that used to live in fixed-function OpenGL: a model-view matrix stack (`pushMatrix`, `translate`, `rotate`, `scale` – same rules as the old `glTranslatef` etc.), the projection, the current colour, a lighting on/off switch, line width and fog. The drawing code sets it; the shaders read it. |
| `src/skinrenderer.*` | All shaders and buffers: the avatar body, meshes, lines/triangles, the floor shadow. |
| `src/animationview.*` | Walks the scene (figures, joints, rings, props, floor) and calls the renderer. Picking. |
| `src/avatarskin.*` | Loads the avatar meshes and skin weights. `skinMatrices()` gives the per-joint matrices for the shader; `skin()` is the same maths on the CPU, kept as the reference for the tests. |
| `src/mesh.*`, `src/floor.*` | Geometry only; they draw through a hook that `AnimationView` installs (`Mesh::setDrawHook`, `Floor::setDrawHook`). |

## Body

- The rest pose of every mesh part sits in static vertex buffers on the graphics card. It is sent again only when `AvatarSkin::restRevision()` changes (another body is loaded, or male/female is switched).
- Each frame only the joint matrices and joint colours are sent, in a buffer texture (9 texels per joint: position matrix, normal matrix, colour, pick name). The vertex shader bends the mesh with up to four joints per vertex.
- Several figures share one `AvatarSkin`; the renderer re-uploads the rest pose when the figure type changes.

## Light

- Studio lights that move with the camera: a warm key light, a soft fill, light from above and a darker bounce from below; soft "wrap" shadow edge for skin, a wide soft highlight and a thin rim light. Lighting is done per pixel in linear colour, gamma 2.2 at the end.
- Options > Light… changes brightness, shine and warmth (`SkinRenderer::Look`, settings `light_brightness`, `light_shine`, `light_warmth`; 50 = standard).
- Meshes (Bento balls, props, handles), lines, rings and the floor share the body's fragment shader, so they get the same light. With `SceneState::setLighting(false)` the plain colour is drawn.
- Fog: exponential, density 0.005, towards the background colour ("Use fog" in Options > Settings…).

## Lines

Modern OpenGL does not promise lines wider than one pixel. Lines (ring axes, props crosshair, skeleton lines) are drawn as thin strips facing the screen by a geometry shader, `SceneState::lineWidth()` pixels wide.

## Floor shadow

Every body drawn in a frame is drawn once more after the floor, flattened onto it (y = 0 in the world) along a light from above, front right. It is drawn 12 times from directions spread over a small disc, each a little dark, so the edge is soft (38 % where fully covered). The stencil buffer makes each pass darken a pixel only once. Options > Light… has an on/off box (`floor_shadow`).

## Picking

- Hover and click use **colour IDs**: everything pickable is drawn into an offscreen buffer – only a 5×5 pixel area around the mouse – in a flat colour that encodes its pick name, and the pixels are read back (`AnimationView::pickPartByColor`).
- The drawing code calls `pickName()` before each pickable thing; the same shaders draw it in that colour. The body uses `gl_PrimitiveID` to give every triangle the pick name of the joint it mostly belongs to (`Part::triangleJoint`).
- Which hit wins: a Bento ball in the area; else a ring, handle or prop in front of what is under the mouse; else what is right under the mouse; else the nearest thing in the area.

## Checking that a change does not change the picture

There is no automatic picture test in CI yet. What was used for #61:

1. Build `main` in a separate worktree and the branch in another build folder.
2. Start both under Xvfb (`xvfb-run -a -s "-screen 0 1280x1024x24"`), drive them the same way with `xdotool` (click, drag, open props), and grab the window with `import -window root`.
3. Compare with ImageMagick: `compare -metric AE -fuzz 1% a.png b.png null:` (number of pixels that differ).
4. Also compare a screenshot taken through the remote control (`screenshot` command), because it draws into an offscreen buffer and can differ from the live window.

In the cloud sandbox Mesa gives OpenGL 4.5. Real graphics cards are tested by hand on Windows before or after a merge.
