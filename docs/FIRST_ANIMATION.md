# Your first animation

Make a short arm movement in about ten minutes.

A **key** is a saved pose at one moment. The program fills in the movement between two keys.

## 1. Start

1. Choose **File > New**. A figure appears.
2. Or choose **File > Open**. The dialog starts in the `examples` folder next to the program; pick a file there.

The figure holds one pose, a T-pose. It has 30 frames. A frame is one step in time.

## 2. Pick a moment

1. Find the slider under the timeline. It is the time marker.
2. Drag it to the middle, about frame 15.

Frame 0 is locked. Do not pose there. To unlock it, turn off **Options > Protect First Frame**.

## 3. Pose the figure

1. Click an arm on the figure. Or click a small ball on it.
2. Drag a coloured ring to turn the arm. Or use the sliders in the Properties panel.
3. Hold **Shift** while you drag for finer control.

## 4. Check the key

Turning a joint saves a key for that joint at once. You will see a mark in the timeline.

The key button has a key icon. It sits right of the play button. It looks pressed when the joint has a key here.

## 5. Set another key

1. Drag the time marker to a later frame, about frame 25.
2. Turn the arm again. This saves a second key.

## 6. Play

1. Click the play button, left of the key button.
2. Click it again to stop. If it still plays, click once more.

The arm now moves between your keys. To change the feel, click a key's joint. Then use the boxes in the Properties panel:

- Tick **Ease Out** for a slow start away from this key.
- Tick **Ease In** for a slow finish into this key.

## 7. Save

1. Choose **File > Save As**.
2. Pick a name. The file ends in `.dingle` and keeps all your work.
3. Choose **File > Export For Second Life**. This makes a `.bvh` file beside it.

Upload to Second Life is still being tested. Do not rely on it yet.

## If something goes wrong

- **The figure does not move.** You are on frame 0. Drag the time marker to a later frame.
- **A joint will not turn further.** It is at the furthest it can turn. Turn it back.
- **You do not like a key.** Press **Ctrl+Z** to undo. Or click the key button to remove the key.
- **The view is lost.** Press **Esc** to reset the camera.
- **You cannot see the face balls.** Click the head. They only show then.
- **Nothing moves when you play.** Set a second key at another frame.
