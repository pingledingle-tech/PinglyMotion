# From video to Second Life with XR Animator

This guide shows how to turn a video of yourself into a Second Life animation. It uses two free programs:

- **XR Animator** records your movement from a webcam or a video file. It runs on your own computer.
- **PinglyMotion** puts that movement on the Second Life avatar, lets you fix it, and saves the `.anim` file for upload.

PinglyMotion has no video capture of its own. Why, and what other tools exist, is in [research/VIDEO_TO_MOTION.md](research/VIDEO_TO_MOTION.md).

The facts about XR Animator were checked on 20 November 2026. Its menus can change in a new version.

## What you need

- A computer with a webcam, or a video file of yourself.
- A room with good light, and enough space to be seen from head to feet.
- Tight or plain clothes help. Loose robes and long skirts hide the legs.

## 1. Get XR Animator

1. Go to the XR Animator project page: [github.com/ButzYung/SystemAnimatorOnline](https://github.com/ButzYung/SystemAnimatorOnline).
2. Pick one of the two versions:
   - The **web app**: a link on that page opens it in your browser. Nothing to install.
   - The **app for Windows, Linux or macOS**: download it from the project's **Releases** page, unpack it, and start it.
3. Allow the webcam when your browser or computer asks.

## 2. Record yourself

The names in **bold** are the names XR Animator shows.

1. Double-click the **Motion capture** item.
2. Pick **Full body (MediaPipe Vision)**.
3. Choose the video input: double-click the **Webcam/Media** item. Use your webcam, or drop a video file of yourself onto the window.
4. Step back until the camera sees all of you. Wait a few seconds until the avatar follows you.
5. Double-click the **Motion capture** item again and pick **Record motion**.
6. Pick a speed. **x 1** is normal. On a slow computer pick **x 0.5** for a video file, so no frames are lost. A live webcam always records at normal speed.
7. For a video file: move to the start with the seek bar, then press play.
8. Start by standing still for about one second. Then do your movement.
9. Double-click to stop the recording.

Tips:

- Keep it short. A Second Life animation can be at most 60 seconds.
- Face the camera. Turns, spins and lying on the floor are hard for the camera to read.
- In XR Animator, **Mocap options** has **Mocap data smoothing**. More smoothing means less shake but fewer details.

## 3. Save the movement as BVH

1. Double-click again and pick **Export motion to file**.
2. At **Choose a motion format to export**, pick **BVH**.
3. Save the file, for example `my_dance.bvh`.

XR Animator writes the BVH from the skeleton of a VRM avatar. If BVH is not offered, choose a VRM avatar in XR Animator and record again.

## 4. Open it in PinglyMotion

1. Start PinglyMotion.
2. Choose **File > Import BVH from another skeleton...**.
3. Pick your `.bvh` file.
4. A box shows a list. Each joint of your file is matched to a joint on the avatar. **Matched** means it will move. **Left out** means it has no partner on the avatar.
   - For an XR Animator file, every body and finger joint is matched. The eyes and the jaw are left out. One back joint (`chest`) may be left out too, when the avatar has an extra upper chest joint. That is correct: the back still bends, because its turn goes to the next joint.
   - Leave **My file starts in a T-pose or A-pose** off. An XR Animator file starts with your first recorded pose.
5. Click **Open the animation**. It opens as a new animation without a name.
6. Press play and look at it from several sides.
7. Save your work with **File > Save as...**. The file ends in `.dingle`.

If a joint is matched wrongly, change it in the list in step 4. More about the list: [RETARGET_MAPPING.md](RETARGET_MAPPING.md).

## 5. Make it ready for Second Life

- **Frame 1 is not played.** Second Life uses frame 1 only to compare. Your movement starts at frame 2. This is why you stood still for a second at the start.
- **Fix the feet.** Video capture often lets the feet slide. Fix the worst frames by hand.
- **Make the file.** Choose **File > Export for Second Life (.anim)...**, set the options, then click **Choose file...**. The steps are in [FIRST_ANIMATION.md](FIRST_ANIMATION.md), section 7.

**Clean up the movement.** The window where you check the joint list (step 4) has a box called **Clean up the movement**. Each choice is a tick box, and PinglyMotion remembers your choices for the next import. Move the mouse over a choice to read what it does.

- **Make it 30 frames a second** (on at first). Second Life plays 30 frames a second. The movement looks the same.
- **Add a start pose at frame 1** (on at first). This is the fix for the frame 1 problem above.
- **Keep it in one place** (off at first). Takes away the walk forward. The sway of the hips stays.
- **Make the end match the start** (off at first). Helps a loop run without a jump. The last frames change a little.
- **Smooth out the shake** (off at first). Light smoothing for video capture. Fast small movements get softer.

The first two are on because they never change how the movement looks. The other three change the movement, so you choose them yourself. Keeping the feet from sliding is not a choice yet.

## Limits

- **Length:** at most 60 seconds.
- **File size:** an `.anim` file can be at most 250,000 bytes. PinglyMotion leaves out keys that a straight line explains. If the file is still too big, make the animation shorter.
- **Quality:** one camera cannot see depth well. Expect some shake, sliding feet and odd turns of the hands and head. Fast spins, floor work and fingers often fail.
- **Fingers:** finger capture is rough. Hand poses made in PinglyMotion often look better.
- **Not tested with a real XR Animator file yet:** the joint matching is tested with a file that has the same joint names and layout.

## Privacy and rights

- **Only record yourself, or people who agreed.** A video of a person's body and face is personal data. Ask before you record a friend.
- **Do not copy other people's dances.** A music video or a famous dance may be protected. Make your own movement.
- **Your video stays on your computer.** XR Animator does the capture on your own machine, in the app or in your browser. PinglyMotion never sees the video, only the BVH file, and does not upload it.

## May I give away or sell the result?

XR Animator's own licence text says that its licence (CC BY-NC-SA 4.0) is about its source code, not about content made with it. It says, about content made with its motion capture using your own assets: "XR Animator claims no right or responsibility over such content." So XR Animator puts no limit on giving your animation away or selling it.

Points to check yourself:

- The movement must be yours (see "Privacy and rights").
- The avatars and other assets that come with XR Animator have their own licences. The BVH file holds only the movement, but read those licences if you use the assets for anything else.
- This is a reading of public texts, not legal advice. XR Animator's own text decides: see "Copyright/License/Credits" on its project page.
