# Design system

Status: written for the [design charter](DESIGN_CHARTER.md) decided by the
project owner on 21 November 2026. The charter wins if this page disagrees
with it.

This page says exactly how every control looks. Jobs copy from here and never
invent their own colours, shapes, icons or sizes (charter P1). The same values
are in [design_tokens.json](design_tokens.json) for code; the two must agree,
and a change to one changes the other in the same pull request. A value that
is missing here is added here first, in its own small pull request, before a
job uses it.

## Colours

All colours come from these tokens (charter P7). Code never writes a colour
by hand; it asks the theme for a token. Custom colours can come later by
replacing the token values.

| Token | Dark | Light | Used for |
|---|---|---|---|
| `background` | `#1E1F22` | `#F3F4F6` | Window background, behind panels |
| `surface` | `#2A2B2F` | `#FFFFFF` | Panels, dialogs, fields |
| `surface_raised` | `#35373C` | `#E9EBEF` | Normal buttons, tiles, hover rows |
| `border` | `#767B85` | `#7A808A` | Control and field borders, dividers |
| `text` | `#E6E7EA` | `#1C1E22` | Normal text and icons |
| `text_muted` | `#AEB2BA` | `#535963` | Secondary text, hints, units |
| `text_disabled` | `#868A93` | `#7A808A` | Text and icons of disabled controls |
| `accent` | `#3272DD` | `#1F63C9` | The main button of a screen, selection, sliders |
| `on_accent` | `#FFFFFF` | `#FFFFFF` | Text and icons on `accent` or `danger_fill` |
| `accent_text` | `#7FB0FF` | `#1A56B0` | Links and accent-coloured text |
| `focus_ring` | `#9CC2FF` | `#1F63C9` | Keyboard focus ring |
| `danger` | `#FF7B72` | `#B42318` | Error text and icons |
| `danger_fill` | `#C93A32` | `#B42318` | Buttons that delete or stop |
| `success` | `#56D39A` | `#18794E` | Done and ok text and icons |
| `warning` | `#F2C14E` | `#8A5A00` | Warning text and icons (for example joint limits) |
| `selection` | `#3272DD` | `#1F63C9` | Selected row, key or joint |
| `area_body` | `#6EA8FE` | `#1F63C9` | Body area colour: Body |
| `area_face` | `#F4A261` | `#B4570F` | Body area colour: Face |
| `area_hands` | `#C792EA` | `#8241B8` | Body area colour: Hands |
| `area_other` | `#5CC8C8` | `#0F7C7C` | Body area colour: Other |
| `axis_x` | `#FF7A7A` | `#C42B1C` | Axis colour X (red): rings, sliders, the view-direction widget, X labels |
| `axis_y` | `#5FD068` | `#1B7530` | Axis colour Y (green) |
| `axis_z` | `#6A9EFF` | `#1F5FD1` | Axis colour Z (blue) |
| `on_axis` | `#1E1F22` | `#FFFFFF` | The letter on an axis-coloured badge or widget end |
| `go_fill` | `#1D7F45` | `#18794E` | Start button fill (transport only) |
| `replay_fill` | `#7048C8` | `#6A3DBF` | Replay from start button fill (transport only) |

Contrast (WCAG 2.2, charter P9), checked for these values:

- `text`, `text_muted`, `accent_text`, `danger`, `success` and `warning` are
  at least 4.5:1 on `background`, `surface` and `surface_raised` in both
  themes.
- `on_accent` is at least 4.5:1 on `accent`, `danger_fill`, `go_fill` and
  `replay_fill` (white on `go_fill` 5.0:1 dark, 5.4:1 light; on `replay_fill`
  6.1:1 dark, 6.9:1 light).
- `axis_x`, `axis_y` and `axis_z` are at least 4.5:1 on `background`,
  `surface` and `surface_raised` in both themes (lowest: `axis_z` dark on
  `surface_raised` 4.5:1, `axis_y` light on `surface_raised` 4.8:1), so they
  work as text, rings and slider fills. `on_axis` is at least 5.3:1 on all
  three axis colours.
- `border`, `focus_ring`, `accent` and the four area colours are at least
  3:1 on `surface` and `background`.
- `text_disabled` is at least 3:1 on every surface, so the reason a control is
  disabled can still be read.

A new colour token must meet the same numbers in both themes before it is
added.

## Axis colours

Decided by the project owner on 22 November 2026 ([DECISIONS.md](DECISIONS.md)).
Everywhere in the program, **X is red, Y is green and Z is blue**: the rotation
rings around the picked joint, the X / Y / Z sliders of the Selected joint
panel, the view-direction widget in the 3D view, the floor's centre lines and
every X, Y or Z label. The colours are `axis_x`, `axis_y` and `axis_z`.

- The letter is always shown next to or on the colour (a small badge with
  `radius.small`, the axis colour as fill and the letter in `on_axis`, or the
  letter on the ring or widget end). Red and green look alike for many people
  with colour blindness, so the letter carries the meaning and the colour backs
  it up (charter P10).
- The axis colours mean axes only. They are not used for anything else, and no
  other colour is used for an axis.

## Colours in the timeline and the 3D view

Built in UX phase 0a (22 November 2026): these parts ask the theme for a
token and write no colour by hand, and the window palette is made from the
tokens in both themes (`src/theme.cpp`). Each answer has an automatic test
(`tests/uxcolours`).

- **Timeline** (`src/timelinelook.h`): rows `surface`, the selected row
  `selection`, every other block of ten frames 35 percent toward `background`,
  keys and the lines between them `text` (`on_accent` on the selected row), a
  key being dragged and the line at the current frame `danger`, track names
  `text` (`text_disabled` for a hidden group, `on_accent` when selected).
  Since UX phase 3a each row follows its body area (`tests/timelinerows`): the
  key row is `surface` with 5 percent of the area token, the name 9 percent,
  a 4 pixel stripe in the area token on the left of the name, the area's name
  in `text_muted`, and keys and the lines between them in the area token; the
  selected row stays `selection` with `on_accent`. Since UX phase 3b the
  selected stretch is a box of `selection` at 22 percent with a 1 pixel
  `selection` border (`tests/timelineclip`), and the Paste mirrored button
  beside Set key is a normal button with the Lucide `flip-horizontal-2` icon.
  Since UX phase 3c the notes line under the timeline (`src/timelinenotes.*`,
  the mockup's `.note`) is `surface` with 8 percent `area_other`, a 1 pixel
  `border` line on top, the Lucide `book-open` icon in `area_other`, the note
  in `text`, and Hide notes as a normal button with the Lucide `eye-off` icon;
  padding `spacing.s` and `spacing.m`, gap `spacing.s` (`tests/timelinenotes`).
- **3D view** (`src/viewcolours.h`): rotation rings and the picked prop's
  handles (arrows, rings, size squares, `src/propedit.h`) in the axis tokens (a
  ring or handle that is not in use is 20 percent toward `background`), the
  handles' letter badges in the full axis colour with the letter in `on_axis`,
  the size squares with a 2 pixel `surface` edge, joint rings
  (`src/jointrings.h`, the mockup's `.jt`: 8 pixels outer radius, hollow with a
  2 pixel `text` edge, filled with the body area colour when the joint has
  changed, `selection` with a 3 pixel `on_accent` edge when selected, a 3 pixel
  `accent_text` edge under the mouse), bones `text_muted` (to the selected joint
  and in a pulled limb `accent_text`),
  effect markers `accent` (box, object) and `danger_fill` (ball, lock), Script Lab chain lines
  (job A4, `src/chainlines.h`) `danger` and solid when locked, `success` and dashed (8 on, 6 off,
  3 pixels wide, scaled with the text size) when free, each with a word tag (the line's colour as
  text and 1 pixel border on `surface`, `radius.small`). The 3D
  view's background and the avatar's own body shading do not follow the theme yet.

## Code colours (Script Lab editor)

Added with lane 1 job F1 (23 November 2026). The colours of LSL words in the
Script Lab editor; they are `syntax` in [design_tokens.json](design_tokens.json),
next to `themes`. They are decoration only and never carry a meaning (charter
P10): keywords are also semibold, comments also italic, and an error is always
a wavy underline, an icon by the line number and words in the error list. Each
is at least 4.5:1 on `background`, `surface` and `surface_raised` in both
themes (lowest: `function` light 5.1:1, `event` light 5.2:1).

| Token | Dark | Light | Used for |
|---|---|---|---|
| `keyword` | `#7FB0FF` | `#1A56B0` | `default`, `state`, `if`, `else`, `for`, `do`, `while`, `jump`, `return` (same values as `accent_text`) |
| `type` | `#D0A4F0` | `#7A3AB0` | `integer`, `float`, `string`, `key`, `vector`, `rotation`, `quaternion`, `list` |
| `event` | `#F4A261` | `#9C4A0A` | Event names such as `state_entry`, `touch_start` |
| `function` | `#5CC8C8` | `#0B6E6E` | Built-in functions such as `llOwnerSay` |
| `constant` | `#E5C07B` | `#7A5200` | Built-in constants such as `TRUE`, `PI` |
| `string` | `#A8D88A` | `#2E6B12` | Text in quotes |
| `number` | `#F0A8D8` | `#A0306E` | Numbers |
| `comment` | `#AEB2BA` | `#535963` | Comments (same values as `text_muted`) |

## Shape

| Token | Value | Used for |
|---|---|---|
| `radius.small` | 4 px | Check boxes, small tags, keys on the timeline |
| `radius.control` | 6 px | Buttons, fields, drop-downs, tiles |
| `radius.panel` | 8 px | Panels, dialogs, menus, tooltips |
| `radius.round` | full | Round toggle switches and round icon buttons only |
| `border_width` | 1 px | All borders |

Every button has the same shape: `radius.control`, `border_width`, height
from `control_height`. There are three kinds, plus the two transport colours
below, and no others:

- **Main button** (one per screen or dialog at most): `accent` fill,
  `on_accent` text.
- **Normal button**: `surface_raised` fill, `border`, `text`.
- **Danger button** (delete, stop, remove): `danger_fill` fill, `on_accent`
  text, and always an icon or word that says what it does (charter P10).

- **Transport buttons** (only in the playback row under the timeline, decided
  by the project owner on 22 November 2026): **Start** has a `go_fill` fill,
  **Replay from start** a `replay_fill` fill (violet, because yellow means a
  warning), both with `on_accent` text; **Stop** is a danger button. Each has
  its icon and its word (play triangle, circular arrow, square), so the colour
  only backs them up (charter P10). A disabled transport button looks like any
  disabled button. Green and violet fills are used nowhere else; the main
  button of the timeline stays Set key in `accent`.

Icon-only buttons are square (width = height) with the same radius and a
tooltip that names the action and its shortcut (charter P3, P8).

## Spacing and size

Spacing steps: 4, 8, 12, 16, 24, 32 px (`spacing.xs` to `spacing.xxl`). Use
only these. Inside a control 8 px, between controls in a group 8 px, between
groups 16 px, panel padding 12 px, dialog padding 24 px.

Control heights: 24 px compact (timeline, dense lists), 32 px default,
40 px large (first-run and empty-state actions). Click targets are at least
24 by 24 px.

## Type

The system UI font, so the program follows the system's font and size.

| Token | Size | Weight | Used for |
|---|---|---|---|
| `caption` | 11 px | regular | Units, small hints under fields |
| `body` | 13 px | regular | Normal text, field values, tooltips |
| `label` | 13 px | semibold | Control labels, panel section names |
| `title` | 16 px | semibold | Panel and dialog titles |
| `heading` | 20 px | semibold | First-run and empty-state headings |

Sizes are at 100 percent. Everything scales with the system text size and
the program's scale setting from 100 to 200 percent; layouts grow instead of
clipping (charter P9). In the program the scale setting is
`src/textsize.*`: the application font is the system font times the percent,
and a size written as a number is scaled with `TextSize::scaled()`; how the
panels rearrange is in [UX_DECISIONS.md](UX_DECISIONS.md), 6c. Numbers that change while playing use tabular digits.

## Icons

- One set only: [Lucide](https://lucide.dev) (ISC licence), outline style,
  24 px grid, stroke 2, drawn at 16, 20 or 24 px. The first job that adds
  Lucide icons adds Lucide to `CREDITS.md` and keeps its licence file.
- Icons take the colour of the text next to them (`text`, `text_disabled`,
  `on_accent`, and so on); they are never multi-coloured pictures.
- If Lucide has no icon for an action, a new one is drawn in the same grid
  and stroke and added to the list of own icons in this page first.
- The same action always has the same icon. Old icons are removed when their
  screen moves to the design system (charter P2).

## States

Every interactive control has all of these states, in both themes:

| State | How it looks |
|---|---|
| Normal | As in "Shape" above |
| Hover | Fill one step lighter (dark theme) or darker (light theme): `surface` becomes `surface_raised`, `surface_raised` mixes 8 percent `text` in, `accent` mixes 12 percent `on_accent` in |
| Pressed | Fill mixes 16 percent `text` in (8 percent for `accent`) |
| Focus (keyboard) | A `focus_ring` ring, 2 px wide, 2 px outside the control, following its radius. Shown on keyboard focus always; never removed |
| Selected or on | `selection` fill or an `accent` mark, plus a shape (tick, filled dot, bold) so it does not rely on colour (charter P10) |
| Disabled | `text_disabled` for text and icons, border at 50 percent, no hover. Always has a **reason**: the tooltip and the status line say why and what to do, for example "Open an animation first" (charter P6). A disabled control still shows its tooltip and can still be reached with the keyboard when it has a reason to show |
| Error | `danger` border and a `danger` message under the control with an icon; the message says what to do |
| Busy | The control shows a small spinner and stays the same size; the status line says what is happening |

Transitions use 120 ms (small changes) or 200 ms (panels opening). Nothing
moves on its own unless the user asked for it.

## Keyboard

- Every action has a menu entry or a keyboard path (charter P8).
- Tab and Shift+Tab move in reading order; arrow keys move inside a group
  (tiles, toolbar, list).
- Enter or Space starts the focused control; Esc closes a dialog, menu or
  popup, or cancels a drag.
- Shortcuts are shown in the menu entry and in the tooltip.
- Camera and key schemes: PinglyMotion default, Maya, Blender and Second Life
  habits; the choice is remembered (charter P11) and the camera follows it at
  once (UX phase 4a).
- The 3D view's view tools: V Pick and pull, O Look around, M Move the view, Z Zoom (UX phase 4b);
  dragging a joint: Shift when pressing turns only that joint, Esc cancels the pull, Enter or G
  picks the picked joint up for the arrow keys (12 pixels a press, Shift 3) and Enter sets the key;
  the view-direction widget is one Tab stop, its ends are chosen with the arrow keys and
  Enter or Space.

## Guidance

- First-run hints are small cards with `radius.panel`, `surface_raised` fill,
  a title, one or two lines of `body` text and a close button. Never more than
  one at a time.
- Empty states (no animation open, no keys yet) show a `heading`, one line
  saying what to do and a large main button.
- One switch, Help > Show hints, turns all guidance on or off; it is
  remembered (charter P4).

## For code

- Qt: the theme (`src/theme.*`) reads these tokens; widgets ask the theme and
  never set colours or fonts directly.
- Tests can read `docs/design_tokens.json` and assert that a widget uses the
  token value (a mechanically checkable visible change, `CLAUDE.md`).
