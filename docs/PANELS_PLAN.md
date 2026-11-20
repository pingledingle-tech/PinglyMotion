# Panels plan (posing UX step 3)

This page is the plan for step 3 of [POSING_UX.md](POSING_UX.md): panels that can be moved, docked and popped out; **Tab** hides all panels; **Reset layout**; the layout is remembered; the camera keeps the figure in the middle of the free area; two screens work. Issue #14 tracks the posing work.

It is a plan only. No code is changed by it. Each job below is its own pull request.

## 1. The goal

From the mockup (`docs/images/posing-ux/main-window.png`):

- The 3D view fills the window.
- Three panels: **Joints** on the left, **Selected joint** on the right, **Timeline** at the bottom.
- Each panel can be dragged, docked to a side, collapsed, or popped out as its own window (useful with two screens).
- **Tab** hides and shows all panels.
- **View › Reset layout** brings back the default layout. The layout is remembered between runs.
- The camera keeps the figure in the middle of the free area, not behind a panel. On a 1440×900 screen with all panels open the free area is about 780×560 pixels.

## 2. What the program has today

The program already uses Qt dock widgets (`QDockWidget`), built in `PinglyMotionWindow::buildDocks()` in `src/pinglymotionwindow.cpp`:

- Two panels, not three: **Properties** (right, or left) and **Timeline** (bottom, or top).
- The joint list from step 2 (`src/jointlist.*`, tabs, search) sits *inside* the Properties panel, on its Avatar tab, above the rotation and position boxes and the ease in / out box. The Props tab holds the prop list and the prop position, scale and rotation boxes. Loop in / out, frame count and FPS sit under the tabs in the same panel.
- Both panels have an own title bar (`src/paneltitlebar.*`) with a "float / put back" button and a close button. Floating panels are separate windows, so they can already go to a second screen.
- The layout is saved with `QMainWindow::saveState()` in the setting `window_state`; the window size in `main_window_width` and `main_window_height`; the timeline on/off in `show_timeline_panel`. There is a check that puts a panel back on its side when an older layout put it somewhere it no longer may go.
- **Options › Reset panel layout** restores the state that was taken at start-up, before the saved layout was loaded.
- Menus: File, Edit, Options, Tools, Help. There is no View menu yet.
- The 3D view (`AnimationView`, a `QOpenGLWidget`) is the central widget. It sets its projection in `setProjection()` from its own width and height (60 degree field of view). Colour picking (`pickPartByColor()`), the rotation rings and everything else read the same projection from `SceneState`. The view takes keyboard focus (`Qt::StrongFocus`) and handles its keys in `keyPressEvent()`.

So much of step 3 exists in a first form. The step is mostly: split one panel into two, add a View menu, Tab, a sturdier saved layout, and the camera rule.

## 3. The choice: docks or panels over the view

### (a) Qt dock widgets (what we have, extended)

The panels are `QDockWidget`s around the 3D view. A docked panel takes space from the view; the view is the free area.

Good for this code base:

- **It already works.** Moving, docking on another side, floating, the own title bar, saving and loading the layout: all there and tested on Windows.
- **Pop-out and two screens come free.** A floating dock is its own window and can sit on another screen. Qt handles screens with different scaling (high DPI) per window.
- **Saving the layout is one call** (`saveState()` / `restoreState()` with a version number). Qt also has tabbed docks (two panels in one place as tabs) and nested docks, by one option each.
- **The camera rule is almost free.** Docked panels shrink the 3D view, so the middle of the view *is* the middle of the free area. Picking, rings and focus need no change, because mouse positions and the projection still refer to the same rectangle.
- **No mouse problems.** A click on a panel never reaches the 3D view and the other way round.
- **Keyboard focus and screen readers** work as in any Qt program.

Weak points:

- **It does not look like the mockup.** The mockup shows the 3D picture running on behind the panels (panels "float over" the view). With docks the view ends at the panel edge. Styling (panel colour close to the view background, thin edges, rounded title bars) gets close, but the picture does not continue behind a panel.
- **A floating panel that the user moves over the window covers the view.** The figure is not moved away from it unless we add that (job P5).
- Qt's dock drag-and-drop has small differences between Windows and Linux (drop markers, how a floating window snaps back). Users rarely notice; the tests must be done on Windows.

### (b) Panels that float over the 3D view (overlays)

The 3D view fills the whole window. The panels are child widgets placed on top of it, with their own drag, snap-to-edge and collapse code.

Good:

- **Looks exactly like the mockup**, including the picture behind half-transparent panels.
- The 3D view never changes size when panels open or close, so the figure does not jump.

Weak points for this code base:

- **Everything that Qt docks give for free must be written and tested by us:** dragging, snapping to an edge, docking two panels together, collapse, resizing, saving and loading the layout, and keeping panels inside the window when it gets smaller.
- **Pop-out to a second screen needs a second mechanism anyway**: a panel over the view is a child widget; to pop it out it must become a window. That is the dock behaviour again, written by hand.
- **The camera rule needs real work.** The view is full size, so the figure must be moved away from the panels with an off-centre projection (a "lens shift"). Picking and the rings follow automatically only if every path uses the same projection; any path that assumes the middle of the widget is wrong.
- **Mouse routing.** Clicks on a panel must not reach the 3D view, and clicks on empty, see-through parts of a panel should. Hover highlighting in the view (`partHighlighted`) must stop when the mouse is over a panel.
- **Speed.** Child widgets over a `QOpenGLWidget` work in Qt 6, but each frame of the 3D view is composed with the widgets on top. Half-transparent panels are redrawn with every 3D frame; on a large high-DPI screen during playback this can cost frame rate. Opaque panels are cheaper but lose the look.
- **Keyboard focus** gets harder: the panels are children of the 3D view, so key handling in the view and in the panels must be kept apart.
- More code means more to break in later steps (4, 7, 11) that put new widgets into the panels.

### Recommendation

**Use (a), Qt dock widgets.** Reasons:

1. It meets every *working* goal of step 3: move, dock, collapse, pop out, two screens, Tab, reset, remembered layout, figure in the middle of the free area. Option (b) only adds the look of the picture continuing behind the panels.
2. Most of it already exists and has been tested by the project owner. The step becomes small jobs that each change little.
3. Option (b) would mean writing our own docking system, and still needing real windows for pop-out.
4. Option (a) leaves the door open: the look can be brought closer to the mockup in step 13 (visual polish) by styling. If the project owner still wants the picture behind the panels after seeing (a), an overlay mode can be tried as a separate job then, with the layout logic from job P1 reused.

This is a change from the mockup's literal picture (panels over a full-size view). It keeps the agreed behaviour. Whether the look is enough is for the project owner to judge when testing job P2.

## 4. The jobs

Each job is one pull request. The first is technical (nothing changes on screen); the others change what the user sees, so the project owner tests the Windows build before each merge.

**Order and what can run beside other work.** P1 can run at any time, beside anything. P2, P3, P4 and P6 all change `src/pinglymotionwindow.*` and must run one after another. P5 changes `src/animationview.*` and `src/camera.*`; it can run beside P3 or P6, but not beside another job on the 3D view. Today the job `feat/dingle-save-open` (pull request 113) claims `src/pinglymotionwindow.*`; P2 must wait until it is merged (the work board decides).

Before the user-facing jobs: every new text (menu items, tips on the title-bar buttons) is checked against the teaching rules in [TEACHING.md](TEACHING.md) and goes through `tr()`.

### P1 – Layout logic as plain code, with tests (technical)

- **What:** a small module without widgets that holds the rules, so they can be tested without a screen:
  - is a saved layout usable (not empty, right version number, window not smaller than a minimum);
  - pull a floating panel back onto a screen when its saved place is no longer on any screen (second screen unplugged, other resolution): input is the panel rectangle and the list of screen rectangles, output the corrected rectangle;
  - the Tab state: which panels were open before Tab hid them, so the second Tab brings back exactly those;
  - the middle of the free area: given the view rectangle and the rectangles of panels that cover it, the point where the figure should be (used by P5).
- **Files:** `src/panellayout.*` (new), `tests/tst_panellayout.cpp` (new), `tests/CMakeLists.txt`, `src/CMakeLists.txt`.
- **Owner test:** none (technical job, standing permission).
- **Estimate:** 2 hours.
- **Done (header-only, `src/panellayout.h`, tests in `tests/panellayout/`):** the module holds the setting names, `isLayoutUsable`, `pullOntoScreen`, `TabState`, `freeArea` / `freeAreaCenter`, `chooseStartup` (migration of the old settings), `chooseWindow` and `PanelVisibility`. It is header-only and registered in the top `CMakeLists.txt`, so no `src/CMakeLists.txt` change was needed. The old setting `show_timeline_panel` is not touched by P1 (a separate fix handles it); `PanelVisibility::timelineVisible()` and `chooseStartup(...).timelineHiddenOnce` are where P2 can take it over.

### P2 – Three panels: Joints, Selected joint, Timeline; View menu

- **What:**
  - Split the Properties panel. **Joints** (left): Avatar / Props switch, the joint tabs, search and list (avatar), or the prop list (props). **Selected joint** (right): the rotation and position boxes (avatar) or the prop position, scale and rotation boxes (props). The ease in / out box stays in Selected joint until step 7 moves it to the timeline.
  - Loop in / out, frame count and FPS move to the **Timeline** panel (the mockup has the loop range there).
  - Joints may dock left or right; Selected joint left or right; Timeline top or bottom. Panels on the same side can be tabbed together (`AllowTabbedDocks`) or stacked.
  - A new **View** menu: Joints, Selected joint, Timeline (tick to show), Reset layout. The old Options items "Show Timeline", "Show Properties" and "Reset panel layout" move there.
  - New object names `jointsDock` and `jointCardDock`; the timeline keeps `timelineDock`. The layout is saved under a new setting with a version number (see section 7), so an old layout with `propertiesDock` is not loaded into the new panels.
  - Default: Joints left, Selected joint right, Timeline at the bottom between them. Widths so that on 1440×900 the free area is at least about 780×560.
- **Files:** `src/pinglymotionwindow.cpp`, `src/pinglymotionwindow.h`, `src/pinglymotionwindow.ui`, `src/usersettings.*`, `docs/POSING_UX.md` (mark the step).
- **Owner test on Windows:**
  1. Start the program. The joint list is on the left, the joint settings on the right, the timeline at the bottom.
  2. Click "Forearm" in the list. The right panel shows its rotation.
  3. Drag the title bar of the left panel to the right side. Let go. It docks there.
  4. Click the float button on the timeline title bar. It becomes its own window. Click it again. It goes back.
  5. Click View › Reset layout. Everything is back where it was at step 1.
- **Estimate:** 4 hours.
- **Done:** built as planned. The ease in / out box was not moved to Selected joint: the easing tile row on the timeline (issue #53, step 5) replaces it, so it stays where the designer file puts it until that job is merged. The rotation and position boxes come from the designer file and are moved into the Selected joint panel by `buildDocks()`, with a stacked page for the prop boxes that follows the Avatar / Props switch; the View menu items have the object names `viewJointsAction`, `viewJointCardAction`, `optionsShowTimelineAction` and `viewResetLayoutAction`. The timeline still stands visible at every start (decision of 19 November). Round-trip test with real docks: `tests/paneldocks/`.

### P3 – The layout is remembered, also the window

- **What:** save and load the panel layout and the window place with Qt's own calls (`saveState(version)`, `saveGeometry()`), so a maximised window and the screen it was on are remembered. A floating panel whose place is no longer on a screen is pulled back (P1 logic). Reset layout uses a default built in code, not the state taken at start-up. Old settings are read once and then not written again (see section 7).
- **Files:** `src/pinglymotionwindow.cpp`, `src/pinglymotionwindow.h`, `src/usersettings.*`, `src/panellayout.*`.
- **Owner test on Windows:**
  1. Move the timeline to the top and drag its lower edge so it is taller. Close the program.
  2. Start it again. The timeline is at the top and as tall as you left it.
  3. Maximise the window, close, start again. It opens maximised.
  4. Click View › Reset layout. The default layout is back.
- **Estimate:** 2 hours.
- **Done** (with P4 in one pull request; `src/panelwindow.h` is the glue between `panellayout.h` and the real docks): the window is saved as `main_window_geometry` with `saveGeometry()` (place, size, maximised, screen) and loaded with `restoreGeometry()`, which also pulls a window off every screen back; the old `main_window_width` / `main_window_height` are read once, used only while no geometry is saved, and never written again; `show_timeline_panel` is not written any more either. After `restoreState()` every floating panel goes through `pullOntoScreen` with the usable rectangles of all screens. Reset layout applies `PanelWindow::applyDefaultLayout` (all three docked on their sides, shown, untabbed, timeline 250 high), the same code `buildDocks()` uses at start; the state taken at start-up is gone. The owner test says "taller timeline" instead of "wider left panel" because P2 fixes the tall panels' width to their contents.

### P4 – Tab hides and shows all panels

- **What:** Tab hides all open panels (docked and floating); Tab again shows the same ones. Also as View › Hide panels, with Tab shown next to it. Tab works only when the 3D view has keyboard focus, so Tab still moves between fields in the panels and still works in the search box and number fields. Clicking the 3D view gives it focus (it does today). The hidden state is not remembered: the program always starts with panels shown, so a new user is never lost.
- **Files:** `src/pinglymotionwindow.cpp`, `src/pinglymotionwindow.h`, `src/animationview.cpp`, `src/animationview.h` (catch Tab before Qt uses it for moving focus), `src/panellayout.*`.
- **Owner test on Windows:**
  1. Click on empty space in the 3D view. Press Tab. All panels disappear and the figure stays in the middle.
  2. Press Tab again. The panels come back.
  3. Float the timeline as its own window. Click the 3D view, press Tab. The floating timeline disappears too. Press Tab. It comes back.
  4. Click in the joint search box and press Tab. The panels stay; the cursor moves on.
- **Estimate:** 2 hours.
- **Done** (with P3 in one pull request): `PanelWindow::toggleHidden` applies `PanelLayout::TabState` to the real docks (`openPanels` is every dock that is not hidden, docked or floating). The View menu item **Hide panels** is a checkable action with the shortcut Tab in `Qt::WidgetShortcut` context, added to the 3D view, so the shortcut fires only while the 3D view has the keyboard focus; `AnimationView::event()` also takes a plain Tab key press before `QWidget` would move the focus. A panel ticked in the View menu while hidden ends the hidden state (`panelOpened`); one unticked stays closed (`panelClosed`). Reset layout clears the state. Before the layout is saved at exit, hidden panels are shown again, so the program always starts with them shown.

### P5 – The figure in the middle of the free area

- **What:** with docks, the view is the free area, so this is already true for docked panels. This job covers the rest:
  - a floating panel that lies over the view: the projection gets a small off-centre shift (a lens shift in `setProjection()`), so the figure sits in the middle of the uncovered part (P1 logic). Picking, rings and focus use the same projection, so they follow;
  - when panels are shown or hidden, the camera keeps the figure the same size on screen where possible, so it does not jump.
- **Files:** `src/animationview.*`, `src/camera.*`, `src/panellayout.*`, `tests/tst_camera.cpp`.
- **Owner test on Windows:**
  1. Float the left panel and drag it over the left half of the 3D view. The figure moves to the middle of the part you can still see.
  2. Click a hand on the figure. The hand is selected (not something next to it).
  3. Drag a ring on the selected joint. It turns as before.
  4. Move the floating panel off the view. The figure goes back to the middle.
- **Estimate:** 3 hours.

### P6 – Pop-out and two screens

- **What:** a popped-out panel behaves like a real window: it can be moved to the other screen, resized there and remembered there. Docking back by its button or by double-clicking the title bar. Panels dragged together keep together (`GroupedDragging`). If the other screen is gone at the next start, the panel comes back onto the main screen (P1 logic). Check on screens with different scaling (for example 100 % and 150 %).
- **Files:** `src/paneltitlebar.*`, `src/pinglymotionwindow.cpp`.
- **Owner test on Windows** (needs a second screen; skip if there is none):
  1. Click the float button on the timeline. Drag the window to the second screen and make it wide.
  2. Close the program and start it again. The timeline is on the second screen.
  3. Close the program, unplug the second screen, start again. The timeline is on the main screen.
  4. Click its button to put it back. It docks at the bottom.
- **Estimate:** 2 hours.
- **Done:** most of "remembered on the second screen" and "pulled back onto the main screen when that screen is gone" were already in place from P1 and P3 (`Qt::saveState`/`saveGeometry` save a floating panel's real place, including which screen, and `PanelWindow::pullFloatingPanelsOntoScreen` runs at start-up). P6 adds the two pieces that were still missing: `QMainWindow::GroupedDragging` in `buildDocks()`'s `setDockOptions()`, so panels docked or tabbed together move as one when one of them is dragged; and a double click on `PanelTitleBar` (away from its buttons) that floats a docked panel or docks a floating one, matching the float button. The toggle on double click is deferred one pass through the event loop (`QTimer::singleShot(0, ...)`), because `QDockWidget` watches mouse presses and releases on its own title bar, even a custom one, to detect a drag; a double click with no real drag makes it decide, right after, that the panel was just dropped back where it was and redocks it, which would otherwise undo an immediate toggle. Different screen scaling needs no code change (docs/PANELS_PLAN.md, section 3(a), notes Qt already handles per-window high-DPI scaling for dock widgets); it is the owner's Windows test to confirm, together with the real second-monitor steps above, which need real hardware the automated tests cannot have. Round-trip tests (`tests/paneldocks/`): `GroupedDragging` is set; a floating panel's exact place and size survive a save/restore round trip and are left alone by `pullFloatingPanelsOntoScreen` while its screen is still known (the already-existing test covers the case where it is not); a double click on the title bar floats and then docks a panel again.

**Total:** about 15 hours of session time, plus the project owner's Windows tests (five rounds, P2 to P6). P3 and P4 are small enough to be tested in one round if the project owner prefers (done so: one pull request, test AH).

## 5. What can break

- **Picking and rings.** They stay correct only while mouse coordinates and the projection refer to the same rectangle. P5's lens shift must go into `setProjection()` only, which every path uses. Test: click small joints (fingers) near the edge of the free area.
- **Camera centring.** Focus (F, double-click) puts the joint in the middle of the projection; after P5 that is the middle of the free area, which is what we want. A view that changes height changes the figure's size (fixed vertical field of view); P5 decides whether to keep that.
- **Keyboard focus.** Moving widgets into new panels changes the focus order. Space, arrow keys, F and the number pad must still reach the 3D view after a click on it. A panel that keeps focus after a click must not swallow these keys (today's `WheelGuard` for the mouse wheel stays on the panel scroll areas).
- **Tab.** Qt uses Tab to move focus; a window-wide Tab shortcut would break typing in fields. That is why P4 catches Tab only in the 3D view.
- **Settings migration.** An old `window_state` names `propertiesDock`, which no longer exists. Loading it would put the new panels in odd places. P2 therefore uses a new setting with a version number and ignores the old one. Users who moved panels in the old version get the default layout once.
- **Floating panels off-screen.** A layout saved with two screens and loaded with one can leave a window out of sight. P1 and P6 pull it back.
- **Qt versions.** Windows uses Qt 6.12 from CI; Linux builds may use the distribution's older Qt 6. Only use dock options and calls that exist in the oldest Qt in the build (see [DEV.md](DEV.md)). `saveState()`, `restoreState()`, `AllowTabbedDocks`, `GroupedDragging` and `saveGeometry()` all exist since Qt 5 and are safe.
- **Linux and Wayland.** On Wayland a program cannot place its own windows, so floating panels may not come back at their saved place. Windows is the main target; on Linux this is a known limit, not a bug to chase.
- **Smoke tests.** `tests/smoke_gui.sh` starts the window; it must still pass after P2 (it does not name the docks today).

## 6. Tests that can be automated

- **P1 unit tests** (`tests/tst_panellayout.cpp`, no screen needed): saved layout usable or not; pull back onto a screen (one screen, two screens, screen gone, negative positions on a left-hand second screen); Tab state (hide remembers which were open, show restores only those, a panel closed while hidden stays closed); middle of the free area (no panel, a panel on one side, panels on two sides, a panel bigger than the view).
- **Round trip with real docks** under `QT_QPA_PLATFORM=offscreen`: build a `QMainWindow` with the three docks, move one, `saveState()`, build a new window, `restoreState()`, check the dock areas. Also: an old state with `propertiesDock` and the wrong version is refused.
- **Projection with lens shift** (P5, in `tests/tst_camera.cpp` or a new test): a point at the middle of the free area projects to the free area's middle; with no shift the matrix is unchanged.
- Not automated: how dragging and docking feel, Tab on a real keyboard, two screens with different scaling. Those are the project owner's tests.

## 7. Setting names

In the existing user settings group (`src/usersettings.*`):

| Setting | Type | Used from | Meaning |
|---|---|---|---|
| `panel_layout` | bytes | P2 | `saveState(1)` of the main window: places, sizes, floating panels |
| `panel_layout_version` | number | P2 | The version passed to `saveState()`; starts at 1. Raised when panels are added or renamed; a different version means "use the default layout" |
| `main_window_geometry` | bytes | P3 | `saveGeometry()`: window place, size, maximised, screen |

Retired (read once for the move, never written again):

- `window_state`: ignored from P2 on (it names the old Properties panel).
- `show_timeline_panel`: from P2 on the timeline's visibility is part of `panel_layout`. If `panel_layout` is empty and this is false, the timeline starts hidden once.
- `main_window_width`, `main_window_height`: from P3 on only used when `main_window_geometry` is empty.

Not saved on purpose: the Tab "panels hidden" state (P4).

## 8. Later steps that depend on this

- **Step 4, Selected-joint card** (trackball, sliders, limit warning, turn-around frame): goes into the Selected joint panel made in P2. Can start after P2 is merged. P3 to P6 do not block it, but they change the same window file, so they run one after another.
- **Step 7, Timeline** (only edited joints, loop range, ease chips): goes into the Timeline panel; needs P2 (loop range and frames moved there, ease box still in Selected joint until step 7 moves it).
- **Step 11, Pose library and export selection**: the pose library may be a fourth panel or a dialog. As a panel it needs P2 and P3 (it must get its own object name and raise `panel_layout_version`).
- Also: step 5 (Left / Right / Mirror) and step 6 (Hands tab) put controls into the Joints panel, so they need P2. Step 9 (hint bar in the 3D view) should follow P5, so the bar sits in the free area. Step 12 (Settings page) gets "Reset layout" from P3. Step 13 (visual polish) is where the panel look is brought closer to the mockup.
