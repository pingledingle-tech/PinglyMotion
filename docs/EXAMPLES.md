# Script Lab examples

Example objects for the Script Lab, for someone who has never written a script. Job E1 (jobs list, issue 74), approved by the project owner on 23 November 2026, 21:02. Process rules: `CLAUDE.md`.

Where they are: `examples/objects/` in the repository, and the folder `examples/objects` next to the program in the Windows zip. Each example is one folder, and each folder is one object, as in the folder model ([SCRIPT_LAB.md](SCRIPT_LAB.md), "The folder model"): the top folder is the object; the subfolder `Root`, or the top folder itself, is the main prim (the root); every other subfolder is one more prim; `.lsl` files are scripts. Every prim that a script colours has a prop of the same name, so the folder tree and the 3D view show the same pieces with the same names.

Every example has a guide in plain words: what it is, at most five steps, what you should see and what to change next, with a one-line meaning for each new word. The guide is stored as data next to the example (`example.json`, below), so the program can show it later (job U3), and the same texts are in the tables on this page. The texts follow the writing rules of [TEACHING.md](TEACHING.md) (at most 12 words a sentence) and are checked with `python3 tools/check_texts.py docs/EXAMPLES.md`. All texts are drafts until the project owner has read them.

**Honest status (23 November 2026).** Only the first example, First colour, works from start to end in the program today; since job U3 the Script Lab's **Make example project** button sets it up in one click. The others run in the tests with exact answers, but in the program two things are missing; both are listed in "What the program can do now" below, and "For the example button" says what the example button (job U3) must do.

The scripts are written fresh for PinglyMotion and reuse the loose examples in `examples/scriptlab/` (`hud_cuffs_hud.lsl`, `hud_cuffs_cuffs.lsl`, `rlv_lock.lsl`, `states_timer.lsl`), which stay where they are for **Try the example scripts**. The relay follows the open relay specification, described in our own words; no code from other relays is copied.

## What the program can do now

A recipe for a beginner, checked against the program's code on 23 November 2026. Stored as data in `examples/objects/guide_now.json`.

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| EXNOW-01 | Example guide | What it is | Props are simple 3D shapes in the scene. They stand in for prims. | draft | E1, shown by U3 |
| EXNOW-02 | Example guide | Shape: Box | Box: good for a HUD, a board or a box. | draft | E1, shown by U3 |
| EXNOW-03 | Example guide | Shape: Sphere | Sphere: good for a ball, a gag or a button. | draft | E1, shown by U3 |
| EXNOW-04 | Example guide | Shape: Cone | Cone: good for a hood, a hat or a bottle. | draft | E1, shown by U3 |
| EXNOW-05 | Example guide | Shape: Torus | Torus, a ring: good for a cuff, a collar or a blindfold. | draft | E1, shown by U3 |
| EXNOW-06 | Example guide | Props: name, place, joint | To add a prop, pick Props in the Joints panel. Press a shape button. | draft | E1, shown by U3 |
| EXNOW-07 | Example guide | Props: name, place, joint | The program names it, for example Box 0 or Torus 0. | draft | E1, shown by U3 |
| EXNOW-08 | Example guide | Props: name, place, joint | The next box is Box 1. You cannot change the name yet. | draft | E1, shown by U3 |
| EXNOW-09 | Example guide | Props: name, place, joint | Place it with Position, Size and Turn in the Selected joint panel. | draft | E1, shown by U3 |
| EXNOW-10 | Example guide | Props: name, place, joint | Pick a joint in Attach to. The prop then moves with that joint. | draft | E1, shown by U3 |
| EXNOW-11 | Example guide | Props: name, place, joint | Save keeps the props in the animation file (name, shape, place, size, turn and joint); Open brings them back. Save props to a `.prp` file keeps no names. | draft | E1, shown by U3 |
| EXNOW-12 | Example guide | Prop and prim | An object is a folder. Each folder inside it is one prim. | draft | E1, shown by U3 |
| EXNOW-13 | Example guide | Prop and prim | A prop and a prim with the same name belong together. | draft | E1, shown by U3 |
| EXNOW-14 | Example guide | Prop and prim | So a folder Box 0 goes with the prop Box 0. | draft | E1, shown by U3 |
| EXNOW-15 | Example guide | Prop and prim | The folder Root, or the object folder itself, is the main prim. | draft | E1, shown by U3 |
| EXNOW-16 | Example guide | Show effects in 3D view | Show effects in 3D view is on the Live scripts page. | draft | E1, shown by U3 |
| EXNOW-17 | Example guide | Show effects in 3D view | Tick it while a folder is watched. Props then show the scripts' colours. | draft | E1, shown by U3 |
| EXNOW-18 | Example guide | Show effects in 3D view | They also show how clear a script made the prim. | draft | E1, shown by U3 |
| EXNOW-19 | Example guide | Show effects in 3D view | Untick it and the props get their own colour back. | draft | E1, shown by U3 |
| EXNOW-20 | Example guide | Opening an example | First colour works now: add one box, then watch that folder. | draft | E1, shown by U3 |
| EXNOW-21 | Example guide | Opening an example | The other examples need props with names like Wrist_left. | draft | E1, shown by U3 |
| EXNOW-22 | Example guide | Opening an example | Show effects in 3D view makes those props from the prim folders. | draft | E1, shown by U3 |
| EXNOW-23 | Example guide | Does not work yet | A made prop is a simple shape. Its place is a guess. | draft | E1, shown by U3 |
| EXNOW-24 | Example guide | Does not work yet | Clicking a prop is a touch only while Show effects is on. | draft | E1, shown by U3 |
| EXNOW-25 | Example guide | Does not work yet | A prop jumps to its new place. It does not glide there. | draft | E1, shown by U3 |
| EXNOW-26 | Example guide | Does not work yet | Touch, Say and dialog buttons do not reach a watched folder yet. | draft | E1, shown by U3 |
| EXNOW-27 | Example guide | Does not work yet | They reach only scripts that an AI assistant started. | draft | E1, shown by U3 |

Where each statement comes from in the code: the four shapes are `Prop::PropType` in `src/prop.h` and the four shape buttons on the Props tab (`src/pinglymotionwindow.ui`); the automatic names Box 0, Box 1 are made in `PropManager::addProp` (`src/propmanager.cpp`) and nothing in the program renames a prop; Position, Size and Turn are moved to the Selected joint panel in `src/panelcontent.h`; Attach to lists the joints (`buildPartLists` in `src/pinglymotionwindow.cpp`); File, Save writes the props with the animation, names included (`storePropsInAnimation` in `src/pinglymotionwindowfile.cpp`, the `props` list of `docs/DINGLE_FORMAT.md`), and Open puts them back under the same names; File, Save props still writes the older `.prp` file with no name (`fileSaveProps`), and Open of a `.prp` or Load props removes the props first, so names start at 0 again; the link by name and Show effects in 3D view are `updatePropTints` in `src/scriptlab/scriptlabpanel.cpp` and the `propTintsChanged` handler in `src/pinglymotionwindow.cpp` (only while a folder is watched as one object); Touch, Say and the dialog buttons under Send input go to the world of the last `lsl_run` (`ScriptLabPanel::sendInput`), and a watched folder keeps its own world only for clicks on props (`LabWatcher::touch`, job A1, pull request 447); props made from the prims and props that move are `updatePrimProps` in `src/scriptlab/scriptlabpanel.cpp` with `src/scriptlab/labprimprops.*`.

## 0. First colour

The smallest example, and the only one that works from start to end today: one script in the prim folder `Box 0` makes the prop `Box 0` red. It uses the program's own name for the first box, because a prop cannot be renamed yet.

```
First_colour/
  example.json
  Box 0/
    colour.lsl
```

Props (shape, joint, position, size and turn as in the program's Props tab; position in the joint's own directions when attached):

| Prop (= prim) | Shape | Attached to | Position | Size | Turn |
|---|---|---|---|---|---|
| Box 0 | box | not attached | 10 40 10 | 10 10 10 | 0 0 0 |

Guide:

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| EXCOLOUR-01 | Example guide | What it is | The smallest example. It works with what the program has now. | draft | E1, shown by U3 |
| EXCOLOUR-02 | Example guide | What it is | The script in folder Box 0 turns the prop Box 0 red. | draft | E1, shown by U3 |
| EXCOLOUR-03 | Example guide | Word | Prim: one part of an object. Each folder is one prim. | draft | E1, shown by U3 |
| EXCOLOUR-04 | Example guide | Step 1 (Props) | Start with no props. Pick Props in the Joints panel. | draft | E1, shown by U3 |
| EXCOLOUR-05 | Example guide | Step 2 (Props) | On Props, press the box button, the first shape button. It is Box 0. | draft | E1, shown by U3 |
| EXCOLOUR-06 | Example guide | Step 3 (Live scripts) | Open Script Lab with its button in the top bar. Live scripts is its first tab. | draft | E1, shown by U3 |
| EXCOLOUR-07 | Example guide | Step 4 (Watch folder...) | Press Watch folder. Pick your copy of First_colour. | draft | E1, shown by U3 |
| EXCOLOUR-08 | Example guide | Step 5 (Show effects in 3D view) | Press Show effects in 3D view, so it shows a tick. | draft | E1, shown by U3 |
| EXCOLOUR-09 | Example guide | What you should see | The box turns red. The log says: Box 0 is red now. | draft | E1, shown by U3 |
| EXCOLOUR-10 | Example guide | What to change next | Open colour.lsl. Change <1.0, 0.0, 0.0> to <0.0, 1.0, 0.0> and save. | draft | E1, shown by U3 |
| EXCOLOUR-11 | Example guide | What to change next | The box turns green. The three numbers are red, green and blue. | draft | E1, shown by U3 |

## 1. Button HUD

The simplest HUD: one prim, one script, a dialog with two buttons. From the HUD part of `examples/scriptlab/hud_cuffs_hud.lsl`.

```
Button_HUD/
  button_hud.lsl
  example.json
```

Props (shape, joint, position, size and turn as in the program's Props tab; position in the joint's own directions when attached):

| Prop (= prim) | Shape | Attached to | Position | Size | Turn |
|---|---|---|---|---|---|
| Button_HUD | box | not attached | -20 45 0 | 8 8 1 | 0 0 0 |

Guide:

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| EXBUTTON-01 | Example guide | What it is | A HUD is a panel of buttons on your Second Life screen. | draft | E1, shown by U3 |
| EXBUTTON-02 | Example guide | What it is | This one has two buttons, Red and Green. It is one prim with one script. | draft | E1, shown by U3 |
| EXBUTTON-03 | Example guide | Word | Dialog: the small window of buttons a script shows you. | draft | E1, shown by U3 |
| EXBUTTON-04 | Example guide | Word | Channel: a number scripts use to talk, like a radio channel. | draft | E1, shown by U3 |
| EXBUTTON-05 | Example guide | Step 1 (Watch folder...) | Press Watch folder. Pick examples, then objects, then Button_HUD. | draft | E1, shown by U3 |
| EXBUTTON-06 | Example guide | Step 2 (Show effects in 3D view) | Press Show effects in 3D view, so it shows a tick. | draft | E1, shown by U3 |
| EXBUTTON-07 | Example guide | Step 3 (Touch) | Open Send input, low on Live scripts, and press Touch. The dialog buttons appear there. | draft | E1, shown by U3 |
| EXBUTTON-08 | Example guide | Step 4 (Red) | Press Red in the dialog. | draft | E1, shown by U3 |
| EXBUTTON-09 | Example guide | What you should see | The flat box beside the figure turns red. The log says: Now red. | draft | E1, shown by U3 |
| EXBUTTON-10 | Example guide | What to change next | Add a third button, Blue, to the list in llDialog. | draft | E1, shown by U3 |
| EXBUTTON-11 | Example guide | What to change next | Then add a line that gives Blue the colour <0.0, 0.0, 1.0>. | draft | E1, shown by U3 |

Today in the program: the props of this example cannot be made with these names, and Send input does not reach a watched folder (see "What the program can do now"). The tests run it with the same inputs.

## 2. Gear set

The main example (the project owner, 23 November 2026, 21:17: a set with cuffs for arms and legs, collar, hood, gag and a blindfold as a ring at the eyes, so the user easily understands how it is linked visually). One object: the HUD in `Root` and eight pieces, each a prim folder with its own script and a prop of the same name on the right joint. The HUD tells the pieces with a link message (number 7002) and counts their answers (7003): inside one object, prims talk with link messages, so this takes the place of the chat channel of `hud_cuffs_hud.lsl` and `hud_cuffs_cuffs.lsl`. The collar holds `@detach=n` (from `rlv_lock.lsl`), the gag `@sendchat=n`. Lock all makes every piece and the HUD red, Free all green.

```
Gear_set/
  example.json
  Ankle_left/
    ankle_left.lsl
  Ankle_right/
    ankle_right.lsl
  Blindfold/
    blindfold.lsl
  Collar/
    collar.lsl
  Gag/
    gag.lsl
  Hood/
    hood.lsl
  Root/
    hud.lsl
  Wrist_left/
    wrist_left.lsl
  Wrist_right/
    wrist_right.lsl
```

Props (shape, joint, position, size and turn as in the program's Props tab; position in the joint's own directions when attached):

| Prop (= prim) | Shape | Attached to | Position | Size | Turn |
|---|---|---|---|---|---|
| Root | box | not attached | -20 45 0 | 8 8 1 | 0 0 0 |
| Wrist_left | torus | lHand | -1 0 0 | 2.8 2.8 1.2 | 0 90 0 |
| Wrist_right | torus | rHand | 1 0 0 | 2.8 2.8 1.2 | 0 90 0 |
| Ankle_left | torus | lFoot | 0 2 0 | 3.4 3.4 1.2 | 90 0 0 |
| Ankle_right | torus | rFoot | 0 2 0 | 3.4 3.4 1.2 | 90 0 0 |
| Collar | torus | neck | 0 1.5 0 | 5 5 1.6 | 90 0 0 |
| Hood | cone | head | 0 4 0 | 9 9 10 | -90 0 0 |
| Gag | sphere | head | 0 0.9 3.6 | 1.6 1.6 1.6 | 0 0 0 |
| Blindfold | torus | head | 0 3.1 0.6 | 8.2 8.2 1.4 | 90 0 0 |

Guide:

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| EXGEAR-01 | Example guide | What it is | A full set of gear in one object, with one HUD. | draft | E1, shown by U3 |
| EXGEAR-02 | Example guide | What it is | Eight pieces: wrist cuffs, ankle cuffs, a collar, hood, gag and blindfold. | draft | E1, shown by U3 |
| EXGEAR-03 | Example guide | What it is | Each piece is a folder with its own script and prop. | draft | E1, shown by U3 |
| EXGEAR-04 | Example guide | What it is | The HUD tells every piece what to do. Each piece answers. | draft | E1, shown by U3 |
| EXGEAR-05 | Example guide | Which piece is which | Root: the flat box beside the figure. It is the HUD. | draft | E1, shown by U3 |
| EXGEAR-06 | Example guide | Which piece is which | Wrist_left and Wrist_right: the rings at the wrists. | draft | E1, shown by U3 |
| EXGEAR-07 | Example guide | Which piece is which | Ankle_left and Ankle_right: the rings at the ankles. | draft | E1, shown by U3 |
| EXGEAR-08 | Example guide | Which piece is which | Collar: the ring at the neck. | draft | E1, shown by U3 |
| EXGEAR-09 | Example guide | Which piece is which | Hood: the cone over the head. | draft | E1, shown by U3 |
| EXGEAR-10 | Example guide | Which piece is which | Gag: the small ball at the mouth. | draft | E1, shown by U3 |
| EXGEAR-11 | Example guide | Which piece is which | Blindfold: the flat ring at the eyes. | draft | E1, shown by U3 |
| EXGEAR-12 | Example guide | Word | Root: the main prim of an object. Here it holds the HUD. | draft | E1, shown by U3 |
| EXGEAR-13 | Example guide | Word | Link message: how the prims of one object talk to each other. | draft | E1, shown by U3 |
| EXGEAR-14 | Example guide | Word | RLV: commands a script sends to your viewer, such as @detach=n. | draft | E1, shown by U3 |
| EXGEAR-15 | Example guide | Word | @detach=n: you cannot take the object off. The collar sends it. | draft | E1, shown by U3 |
| EXGEAR-16 | Example guide | Word | @sendchat=n: you cannot talk in open chat. The gag sends it. | draft | E1, shown by U3 |
| EXGEAR-17 | Example guide | Step 1 (Watch folder...) | Press Watch folder. Pick examples, then objects, then Gear_set. | draft | E1, shown by U3 |
| EXGEAR-18 | Example guide | Step 2 (Show effects in 3D view) | Press Show effects in 3D view, so it shows a tick. Every piece turns green. | draft | E1, shown by U3 |
| EXGEAR-19 | Example guide | Step 3 (Touch) | Open Send input, low on Live scripts, and press Touch. The dialog buttons appear there. | draft | E1, shown by U3 |
| EXGEAR-20 | Example guide | Step 4 (Lock all) | Press Lock all in the dialog. | draft | E1, shown by U3 |
| EXGEAR-21 | Example guide | Step 5 (Free all) | Press Free all to open everything again. | draft | E1, shown by U3 |
| EXGEAR-22 | Example guide | What you should see | After Lock all, every piece turns red, and the HUD too. | draft | E1, shown by U3 |
| EXGEAR-23 | Example guide | What you should see | The log says: All 8 pieces are locked. | draft | E1, shown by U3 |
| EXGEAR-24 | Example guide | What you should see | RLV restrictions now lists detach and sendchat. | draft | E1, shown by U3 |
| EXGEAR-25 | Example guide | What you should see | After Free all, everything turns green and the list is empty. | draft | E1, shown by U3 |
| EXGEAR-26 | Example guide | What to change next | Open hood.lsl and change the red <1.0, 0.0, 0.0> to blue. | draft | E1, shown by U3 |
| EXGEAR-27 | Example guide | What to change next | Blue is <0.0, 0.0, 1.0>. Save, then press Touch and Lock all again. | draft | E1, shown by U3 |

Today in the program: the props of this example cannot be made with these names, and Send input does not reach a watched folder (see "What the program can do now"). The tests run it with the same inputs.

## 3. Timed lock

A cuff on the left wrist that locks on a touch and opens by itself after 5 seconds. From `examples/scriptlab/states_timer.lsl` (two states and a timer) and `rlv_lock.lsl`.

```
Timed_lock/
  example.json
  timed_lock.lsl
```

Props (shape, joint, position, size and turn as in the program's Props tab; position in the joint's own directions when attached):

| Prop (= prim) | Shape | Attached to | Position | Size | Turn |
|---|---|---|---|---|---|
| Timed_lock | torus | lHand | -1 0 0 | 2.8 2.8 1.2 | 0 90 0 |

Guide:

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| EXTIMER-01 | Example guide | What it is | A cuff that locks when you touch it. It opens by itself after 5 seconds. | draft | E1, shown by U3 |
| EXTIMER-02 | Example guide | Word | State: a mode of the script. This one has default and locked. | draft | E1, shown by U3 |
| EXTIMER-03 | Example guide | Word | Timer: wakes the script every second, here to count down. | draft | E1, shown by U3 |
| EXTIMER-04 | Example guide | Step 1 (Watch folder...) | Press Watch folder. Pick examples, then objects, then Timed_lock. | draft | E1, shown by U3 |
| EXTIMER-05 | Example guide | Step 2 (Show effects in 3D view) | Press Show effects in 3D view, so it shows a tick. | draft | E1, shown by U3 |
| EXTIMER-06 | Example guide | Step 3 (Touch) | Open Send input, low on Live scripts, and press Touch. | draft | E1, shown by U3 |
| EXTIMER-07 | Example guide | What you should see | The ring at the left wrist turns red. | draft | E1, shown by U3 |
| EXTIMER-08 | Example guide | What you should see | The log counts down: 4 seconds left, 3 seconds left. | draft | E1, shown by U3 |
| EXTIMER-09 | Example guide | What you should see | After 5 seconds the ring turns green again. | draft | E1, shown by U3 |
| EXTIMER-10 | Example guide | What to change next | Change SECONDS = 5 to SECONDS = 10 and save. Touch it again. | draft | E1, shown by U3 |

Today in the program: the props of this example cannot be made with these names, and Send input does not reach a watched folder (see "What the program can do now"). The tests run it with the same inputs.

## 4. Simple RLV relay

A simple RLV relay on the relay channel (-1812221819). A message is `name,target,commands`: the target is the relay's key or `ffffffff-ffff-ffff-ffff-ffffffffffff` (every relay), the commands are separated by `|`. Every `@` command goes to the viewer (`llOwnerSay`) and is answered with `name,key,command,ok`; `!version` is answered with `1100`, `!release` sends `@clear`; anything else gets `ko`. It says yes to every object, which a real relay should not do; the guide says so. Red while it holds a restriction, green when free.

```
RLV_relay/
  example.json
  relay.lsl
```

Props (shape, joint, position, size and turn as in the program's Props tab; position in the joint's own directions when attached):

| Prop (= prim) | Shape | Attached to | Position | Size | Turn |
|---|---|---|---|---|---|
| RLV_relay | sphere | chest | 0 0 4 | 2 2 2 | 0 0 0 |

Guide:

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| EXRELAY-01 | Example guide | What it is | A relay lets other objects, like a chair, send you RLV commands. | draft | E1, shown by U3 |
| EXRELAY-02 | Example guide | What it is | This simple relay says yes to every object. A real relay asks first. | draft | E1, shown by U3 |
| EXRELAY-03 | Example guide | Word | Relay: passes RLV commands from other objects to your viewer. | draft | E1, shown by U3 |
| EXRELAY-04 | Example guide | Word | The long key of only f letters means every relay. | draft | E1, shown by U3 |
| EXRELAY-05 | Example guide | Step 1 (Watch folder...) | Press Watch folder. Pick examples, then objects, then RLV_relay. | draft | E1, shown by U3 |
| EXRELAY-06 | Example guide | Step 2 (Show effects in 3D view) | Press Show effects in 3D view, so it shows a tick. | draft | E1, shown by U3 |
| EXRELAY-07 | Example guide | Step 3 (Channel) | Open Send input. Type the RELAY number from relay.lsl into Channel. Type chair,ffffffff-ffff-ffff-ffff-ffffffffffff,@detach=n into Text. | draft | E1, shown by U3 |
| EXRELAY-08 | Example guide | Step 4 (Say) | Press Say. | draft | E1, shown by U3 |
| EXRELAY-09 | Example guide | What you should see | The small ball on the chest turns red. | draft | E1, shown by U3 |
| EXRELAY-10 | Example guide | What you should see | RLV restrictions now lists detach. The answer in the log ends with ok. | draft | E1, shown by U3 |
| EXRELAY-11 | Example guide | What to change next | Say the same line with !release in place of @detach=n. | draft | E1, shown by U3 |
| EXRELAY-12 | Example guide | What to change next | The relay lets go of everything and turns green. | draft | E1, shown by U3 |

Values for the step (in `example.json` as `values`, so the program can fill them in):

- Channel: `-1812221819` (EXRELAY-07)
- Text: `trap,ffffffff-ffff-ffff-ffff-ffffffffffff,@detach=n` (EXRELAY-07)

Today in the program: the props of this example cannot be made with these names, and Send input does not reach a watched folder (see "What the program can do now"). The tests run it with the same inputs.

## 5. Work system

A HUD that drives two other prims: each Work press fills the bottle a quarter (it gets more solid: alpha 0.4, 0.6, 0.8, 1.0), a full bottle tells the score board, which counts a point and turns gold for a second.

```
Work_system/
  example.json
  Bottle/
    bottle.lsl
  Root/
    work_hud.lsl
  Score_board/
    score_board.lsl
```

Props (shape, joint, position, size and turn as in the program's Props tab; position in the joint's own directions when attached):

| Prop (= prim) | Shape | Attached to | Position | Size | Turn |
|---|---|---|---|---|---|
| Root | box | not attached | -20 45 0 | 8 8 1 | 0 0 0 |
| Bottle | cone | not attached | 20 5 10 | 6 6 12 | -90 0 0 |
| Score_board | box | not attached | 20 30 10 | 10 6 1 | 0 0 0 |

Guide:

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| EXWORK-01 | Example guide | What it is | One HUD drives two other parts, a bottle and a score board. | draft | E1, shown by U3 |
| EXWORK-02 | Example guide | What it is | Each press of Work fills the bottle a quarter. A full bottle scores a point. | draft | E1, shown by U3 |
| EXWORK-03 | Example guide | Which piece is which | Root: the flat box beside the figure. It is the HUD. | draft | E1, shown by U3 |
| EXWORK-04 | Example guide | Which piece is which | Bottle: the cone standing on the floor. | draft | E1, shown by U3 |
| EXWORK-05 | Example guide | Which piece is which | Score_board: the flat box above the bottle. | draft | E1, shown by U3 |
| EXWORK-06 | Example guide | Word | Alpha: how solid a prim is. 1 is solid, 0 is clear. | draft | E1, shown by U3 |
| EXWORK-07 | Example guide | Step 1 (Watch folder...) | Press Watch folder. Pick examples, then objects, then Work_system. | draft | E1, shown by U3 |
| EXWORK-08 | Example guide | Step 2 (Show effects in 3D view) | Press Show effects in 3D view, so it shows a tick. | draft | E1, shown by U3 |
| EXWORK-09 | Example guide | Step 3 (Touch) | Open Send input, low on Live scripts, and press Touch. The dialog buttons appear there. | draft | E1, shown by U3 |
| EXWORK-10 | Example guide | Step 4 (Work) | Press Work in the dialog four times. | draft | E1, shown by U3 |
| EXWORK-11 | Example guide | What you should see | The bottle gets more solid with every press. | draft | E1, shown by U3 |
| EXWORK-12 | Example guide | What you should see | At the fourth press the score board turns gold for a second. | draft | E1, shown by U3 |
| EXWORK-13 | Example guide | What you should see | The log says: Score: 1. The bottle starts almost clear again. | draft | E1, shown by U3 |
| EXWORK-14 | Example guide | What to change next | Open bottle.lsl and change quarters == 4 to quarters == 2. | draft | E1, shown by U3 |
| EXWORK-15 | Example guide | What to change next | Now two presses fill the bottle. | draft | E1, shown by U3 |

Today in the program: the props of this example cannot be made with these names, and Send input does not reach a watched folder (see "What the program can do now"). The tests run it with the same inputs.

## The data file `example.json`

One file in each example's top folder. The folder model reads only `.lsl`, `.txt` and `.notecard` files, so the file is not part of the object. Format 1:

- `format` (1), `order` (the place in the list, 0 first), `object` (the top folder's name), `title`, `works_now` (true when every step works in the program today).
- `props`: one entry per prop, `name` (the prim's name: its folder's name, `Root` for the folder `Root`, the object's name for files right in the top folder), `shape` (`box`, `sphere`, `cone` or `torus`, as `Prop::PropType`), `joint` (a joint name of the figure, such as `lHand`, or empty for a prop that is not attached), `position`, `scale`, `rotation` (three numbers each, the same values as the Position, Size and Turn boxes and the columns of a `.prp` file).
- `guide`: the texts in order, each with `id` (the row on this page), `part` (`what`, `word`, `piece`, `step`, `see`, `next`) and `text`. A step also has `control`: the button, field or dialog button it names, as the program or the example's script spells it; a step can have `values`, field name to text.
- `examples/objects/guide_now.json` holds the recipe above in the same way, with the parts `what`, `shape` (with `name`, the shape's name in `src/prop.h`), `how`, `link`, `effects`, `open` and `not`.

The joint is a name, not a number: a `.prp` file stores the attachment as a track number of the skeleton, which changes with the skeleton (the old `boxonhead.prp` says 5, which was the head of the old skeleton and is `mSpine3` in today's), and a `.prp` file has no names.

## For the example button (job U3)

What a later "make example project" button has to do, found in the code on 23 November 2026. None of it is built in E1. **Built in job U3 (pull request 443):** points 1, 2, 3 and 5 for the first example, First colour (the button copies it into `Documents/PinglyMotion/Scripts`, puts its props in the scene by name, watches it, turns the effects on and shows the steps); point 4 is not built, so the steps that need Touch, Say or the dialog buttons are greyed with "This step does not work in the program yet." ([UX_DECISIONS.md](UX_DECISIONS.md), 6o).

1. **Put the props into the scene with their names.** The route that exists: `Scene::addProp(type, mesh, attach, x, y, z, scale x, y, z, turn x, y, z)` (`src/scene.h`), as File, Open props does in `PinglyMotionWindow::fileLoadProps` (`src/pinglymotionwindowfile.cpp`): the shape picks the mesh (`cubeMesh`, `sphereMesh`, `coneMesh`, `torusMesh`), each new prop is added to `propNameCombo`, and `emit clearProps()` empties the scene first. The missing part: `PropManager::addProp` always names a prop `Box 0`, `Box 1` and so on. The smallest change is an optional name argument on `PropManager::addProp` and `Scene::addProp` (the name used when it is free) and a new prop name in `propNameCombo`; then the tint code finds the props by name with no other change.
2. **Turn the joint name into the attachment number:** the number is the joint's index in `AnimationView::partNames()` (0 is position, then every joint and end site in skeleton order), the same list `buildPartLists` puts in Attach to.
3. **Watch the folder as one object and tick Show effects in 3D view:** the panel's Watch folder does `lab_watch` with `linkset: true` on the folder; ticking `m_effectsBox` (`ScriptLabPanel::effectsToggled`) then sends the tints. Both need a small public call on `ScriptLabPanel`, as the button lives outside the panel.
4. **Let Send input reach the watched object.** Today Touch, Say and the dialog buttons go to the world of the last `lsl_run` (`lsl::Control::event` with the panel's `lsl::Session`), and `LabFolder::run` keeps no world, so pressing Touch or Lock all does nothing to a watched example. The examples need the watched folder's run to keep its world in a session, and the tints to follow each input (today `LabTint::fromFolder` reads only the folder run's log). Without this, steps such as "Press Touch" and "Press Lock all" in the guides above cannot work. **Partly built in job A1 (pull request 447):** the watched folder's run now keeps its world, and a click on a prop in the 3D view sends a touch to it (the tints and places follow); Touch, Say and the dialog buttons under Send input still go to the world of the last `lsl_run`.
5. **Show the guide:** read `guide` from `example.json`; the texts are in the order to show, and each step's `control` names what it points at.

## Tests

- `tests/tst_examples.cpp` (`pinglymotion_examples_tests`): every example opens as the expected object (prims, links, scripts), passes the import check with no error or warning, and runs with the inputs its guide asks for, with exact owner lines, exact RLV restrictions and the exact colour of every prop of the same name (the main window's tint rule).
- `tests/test_examples.py` (ctest `examples_guides`): every `example.json` and `guide_now.json` has the format above; at most five steps, each naming a control that exists in the program's texts or in the example's own dialog; every prop has a shape from `src/prop.h`, a joint of both skeletons and a prim of its name; the texts here and in the files are the same; this page passes `tools/check_texts.py`; and "What the program can do now" is the first section and lists exactly the shapes of `src/prop.h`.
