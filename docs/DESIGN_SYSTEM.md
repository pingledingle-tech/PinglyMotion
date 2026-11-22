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

Contrast (WCAG 2.2, charter P9), checked for these values:

- `text`, `text_muted`, `accent_text`, `danger`, `success` and `warning` are
  at least 4.5:1 on `background`, `surface` and `surface_raised` in both
  themes.
- `on_accent` is at least 4.5:1 on `accent` and `danger_fill`.
- `border`, `focus_ring`, `accent` and the four area colours are at least
  3:1 on `surface` and `background`.
- `text_disabled` is at least 3:1 on every surface, so the reason a control is
  disabled can still be read.

A new colour token must meet the same numbers in both themes before it is
added.

## Shape

| Token | Value | Used for |
|---|---|---|
| `radius.small` | 4 px | Check boxes, small tags, keys on the timeline |
| `radius.control` | 6 px | Buttons, fields, drop-downs, tiles |
| `radius.panel` | 8 px | Panels, dialogs, menus, tooltips |
| `radius.round` | full | Round toggle switches and round icon buttons only |
| `border_width` | 1 px | All borders |

Every button has the same shape: `radius.control`, `border_width`, height
from `control_height`. There are three kinds and no others:

- **Main button** (one per screen or dialog at most): `accent` fill,
  `on_accent` text.
- **Normal button**: `surface_raised` fill, `border`, `text`.
- **Danger button** (delete, stop, remove): `danger_fill` fill, `on_accent`
  text, and always an icon or word that says what it does (charter P10).

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
clipping (charter P9). Numbers that change while playing use tabular digits.

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
  habits; the choice is remembered (charter P11).

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
