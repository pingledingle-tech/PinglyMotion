ID: WP-01
Title: Combined UX Round After v0.3.0-beta.2
Status: running
Owner-approved: yes
Priority: 1
Reason: The project owner approved the start on 28 Nov 2026 14:07 Oslo: «kjør WP1». Part A runs first.

## Tasks

### Part A — Bug fixes
- [x] Fix Reset view after switching from Expert mode to New to animations hiding the left panel
- [x] Fix bottom panel not showing all controls at startup
- [x] Fix "Control the camera" (`setcam`) not selectable under action settings in Script Lab

### Part B — Script Lab layout
- [ ] Show Script Lab tab contents as sub-tabs on the left side of the main view
- [ ] Allow detaching function groups (Object, Script) and individual functions inside tabs
- [ ] Add a dedicated Reset view for Script Lab only (without resetting the whole window)
- [ ] Hide "steps first collar" when Expert mode is enabled

### Part C — Props, 3D controls, and examples
- [ ] Allow Delete key to remove selected prop (with warning if linked to a script)
- [ ] Use Firestorm-style 3D object editing controls in Script Lab regardless of chosen camera preset (Owner decision 28 Nov 14:10: yes — Script Lab 3D uses Firestorm camera controls (Alt + left click) and the other Firestorm edit controls, regardless of camera preset. Owner decision 28 Nov 14:51: yes — a prop blinks briefly when clicked.)
- [ ] Show all RLV toys with props under Examples in Library
- [ ] Provide a clear way to remove Example Projects in Script Lab and discover more examples
- [ ] Add an import/open option to open a whole folder set from Script Lab

### Owner test notes 27 Nov (from the owner's test page, not fixed in this PR)
- [ ] AZ2: the Script Lab window can be dragged, but the panel does not follow
- [ ] AZ8: only one box shows, and the name "box" feels illogical
- [ ] BA2: unsure how the channel map works; wants an example or a better manual
- [ ] BA3: did not understand "flat box" and found no "Lock all"
