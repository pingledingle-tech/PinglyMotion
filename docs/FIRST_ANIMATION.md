# Your first animation

Make a short arm movement in about ten minutes.

A **key** is a saved pose at one moment. The program fills in the movement between two keys.

## 1. Start

1. Choose **File > New**. A figure appears.
2. Or choose **File > Open**. The dialog starts in the `examples` folder next to the program. Pick a `.dingle` or `.bvh` file there.

The figure holds one pose, a T-pose. It has 30 frames. A frame is one step in time.

## 2. Pick a moment

1. Find the slider under the timeline. It is the time marker.
2. Drag it to the middle, about frame 15.

Frame 0 is locked. Do not pose there. To unlock it, turn off **Edit > Lock the first frame**.

## 3. Pose the figure

1. Click an arm on the figure. Or click a small ball on it.
2. Drag a coloured ring to turn the arm. Or use the sliders in the Selected joint panel on the right.
3. Hold **Shift** while you drag for finer control.

## 4. Check the key

Turning a joint saves a key for that joint at once. You will see a mark in the timeline.

The key button has a key icon. It sits right of the time marker's bar. It looks pressed when the joint has a key here.

## 5. Set another key

1. Drag the time marker to a later frame, about frame 25.
2. Turn the arm again. This saves a second key.

## 6. Play

1. Click **Start**, under the time marker's bar.
2. Click **Stop** to stop. **Replay from start** plays again from the first frame.
3. With **Loop** ticked (the default) it plays again and again. Untick it to play once.
4. **Frame back** and **Frame forward** go one frame at a time, so you can look at each pose. The left and right arrow keys do the same.

The arm now moves between your keys. To change the feel, click your second key on the timeline. Then look at the row of six tiles under the timeline:

- Each tile shows a small figure walking at its own speed. The dots behind it show where it is fast and where it is slow.
- Click a tile, for example **Slow both ends**. Play again. The arm now starts and stops softly.
- The line under the tiles tells you what each one is good for.

To see all of this in a finished animation, pick **File > Open** and then `wave.dingle` in the `examples` folder. It is a short wave with five keys on the right arm, and each pose change uses a different tile. Click each key of the arm on the timeline: the tiles show the choice of the change that ends there. Notes on the keys:

| Key | Frame | Tile of the change that ends here | Note |
|---|---|---|---|
| 1 | 0 | (none, the start) | A key is a saved pose. This one has the arm down. The wave starts and ends here, so it loops. |
| 2 | 20 | Slow both ends | The arm lifts here. Slow both ends makes it start and stop softly. |
| 3 | 30 | Overshoot, then settle | The hand swings out, goes a little past, then settles. That is Overshoot, then settle. |
| 4 | 40 | Even | The hand swings back at one steady speed. That is Even. It looks stiff, so try Slow both ends. |
| 5 | 60 | Slow finish | The arm slows down as it lands. It matches the first key, so the loop has no jump. |

Your turn: make the arm stop more softly.

The notes line under the timeline shows the note of the current frame, and the task when you stop at the last frame. **Hide notes** hides it; Help > Show hints brings it back. The file is made by `tools/wave_example.py`.

To watch a real example slowly, pick **File > Open** and then `jump_slow.dingle` in the `examples` folder. It is the bundled `jump.dingle` with every key 2.5 times later, so the same jump runs at 40 % of the speed. The file is made by `tools/slow_example.py`, and the original is unchanged.

## 7. Save

1. Choose **File > Save as...**.
2. Pick a name. The file ends in `.dingle`. This is the working format. It keeps all your work.
3. Choose **File > Export for Second Life (.anim)...** to make a file for Second Life.
4. A small box opens. Set what you want, then click **Choose file...**:
   - **Priority (0 to 6):** when two animations move the same joint, the higher number wins.
   - **Slow start:** how long the animation takes to fade in.
   - **Slow finish:** how long it takes to fade out.
   - **Play again and again:** tick it to use the loop points of the animation.
   - **Hands:** how the fingers look while it plays.
5. Pick a name. The file ends in `.anim`.

**File > Export BVH** makes a `.bvh` file instead.

Export, upload and playback in Second Life were validated on 21 November 2026 in Firestorm: timing, loop, slow start and finish, hands, easing, the BVH route and importing the uploaded `.anim` again.

## 8. Bring in other files

All of these are in the **File** menu. Each one opens the result as an untitled animation. Save it with **File > Save as...** to keep it. Your original file is not changed.

- **Import Second Life .anim...** opens an `.anim` file. If something could not be brought in, a list under **What could not be imported:** tells you what.
- **Import a BVH from another skeleton...** is for a `.bvh` file made for a different skeleton. A box shows a list. The program matched each joint in your file to a joint on the avatar. **Matched** means it will move. **Left out** means it has no partner on the avatar. Tick **My file starts in a T-pose or A-pose** if the first frame is a standing pose with the arms out. Then click **Open the animation**.
- **Import glTF animation...** opens a `.glb` or `.gltf` file. If the file holds more than one animation or skeleton, pick the ones you want in the box.

## About and credits

Choose **Help > About...** to see who made the avatar bodies and under which licence.

## If something goes wrong

- **The figure does not move.** You are on frame 0. Drag the time marker to a later frame.
- **A joint will not turn further.** It is at the furthest it can turn. Turn it back.
- **You do not like a key.** Click the key button to remove the key.
- **The view is lost.** Press **Esc** to reset the camera.
- **You cannot see the face balls.** Click the head. They only show then.
- **Nothing moves when you play.** Set a second key at another frame.
- **An imported file does not move right.** Open the list in the import box. Joints marked **Left out** were not brought in.

## The same by remote control

An AI client can do the steps above for you, with the commands `new_animation`, `open_file` and `save_file`. They only work in the `PinglyMotion` folder in your Documents folder. See [CONTROL.md](CONTROL.md).
