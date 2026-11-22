# Design charter

Status: decided by the project owner on 21 November 2026. This is the one
top-authority document for how PinglyMotion looks and behaves. When any other
document, mockup picture, issue or old code disagrees with it, this charter
wins. The exact colours, shapes, icons and sizes are in
[DESIGN_SYSTEM.md](DESIGN_SYSTEM.md) and [design_tokens.json](design_tokens.json).

## The goal

The whole program gets ONE modern design: self-explaining and guided at first
sight, the same look for every control, and only what is needed now on screen.
It must work for beginners, for disabled users and for Maya veterans, with one
layout for everybody. All old icons and old styles are removed everywhere.

## How to use this charter

- Every job reads this charter first (`CLAUDE.md`, "Before you start").
- Every pull request with a visible change fills in **Charter principles
  followed** in the pull request text: the ids it follows (for example
  `P1, P5, P7`) and one short line each on how. A visible pull request without
  this field is not merged.
- A job that cannot follow a principle stops and says so in its pull request.
  An exception needs the project owner's yes and an entry in
  [DECISIONS.md](DECISIONS.md).
- Order of authority: this charter, then [DESIGN_SYSTEM.md](DESIGN_SYSTEM.md),
  then [UX_DECISIONS.md](UX_DECISIONS.md), then older plans
  ([POSING_UX.md](POSING_UX.md), [PANELS_PLAN.md](archive/PANELS_PLAN.md)) and their
  mockup pictures.

## Principles

Each principle can be checked by looking at the program or the code.

**P1 – One design.** Every control (button, field, slider, tile, panel title,
menu, dialog) uses the colours, shapes, icons, spacing and type of
[DESIGN_SYSTEM.md](DESIGN_SYSTEM.md). Jobs copy from it and never invent their
own. Check: no colour, radius, font size or icon in the change that is not a
token or a listed icon.

**P2 – Old stuff goes.** Old icons and old styles are removed, not mixed with
the new ones. A job that touches a screen moves the whole of that screen to the
design system; new screens never use old icons or styles. Check: no old icon
file or hand-made style left on the touched screen.

**P3 – Self-explaining at first sight.** A new user can see what each control
does without reading a manual: a visible plain-word label, or an icon with a
tooltip that names it. The main task of a screen is the most visible thing on
it. Check: every control has a label or tooltip; no icon-only control without
a name.

**P4 – Guided.** The program guides a new user: first-run hints, an empty
state that says what to do next, and short help where a step is not obvious.
Guidance never blocks work and can be switched off (and back on) in one place.
Check: each hint can be closed; the switch exists and is remembered.

**P5 – Show only what is needed now.** Advanced controls stay hidden until
asked for (Expert mode, a "More" section, a menu). A control that cannot be
used right now is shown only if seeing it helps. Check: the beginner view of a
screen has no expert-only control in it.

**P6 – The program talks back.** Every action gives visible feedback. A
control that cannot be used right now is greyed out **with a reason** (in its
tooltip and, when hovered or focused, in the status line), for example "Open
an animation first". Error messages say what happened and what to do next, in
plain words. Check: every disabled state has a reason text; no silent failure.

**P7 – Dark and light themes.** Both themes are complete and equal, and all
colours come from the theme tokens, so custom colours can be added later
without changing code. Check: the change looks right in both themes; no
hard-coded colour.

**P8 – Full keyboard use.** Everything can be done without a mouse: every
action is reachable by keyboard, the tab order follows the screen, focus is
always visible, Esc closes or cancels, and shortcuts are shown in menus and
tooltips. Check: the new control can be reached, used and left with the
keyboard only.

**P9 – Contrast and scalable text.** Text and controls meet WCAG 2.2 AA in
both themes (4.5:1 for normal text, 3:1 for large text, icons, borders and
focus rings). Text follows the system text size and the program's own scale
setting (100 to 200 percent) without clipping or overlap. Check: the
contrast values in [DESIGN_SYSTEM.md](DESIGN_SYSTEM.md) are used; the screen
works at 200 percent.

**P10 – No meaning by colour alone.** Colour is always backed by a shape, an
icon, a text or a position (for example a red Stop button also has a square
icon and the word Stop; body-area colours also have the area name). Check:
the screen still makes sense in grey scale.

**P11 – Fast for experts.** Experienced users are not slowed down by
guidance: shortcuts for common actions, camera and key schemes for Maya,
Blender and Second Life habits next to the default, and remembered choices.
Check: the action has a shortcut or a reason why not; guidance switched off
leaves no extra click.

**P12 – One layout for all, with adapting parts.** Beginners and experts use
the same window and the same places for things. What adapts is the guidance
and which advanced parts are shown, not where things are. Check: switching
Beginner or Expert mode or guidance on or off does not move existing controls.

**P13 – Plain, consistent words.** Interface text is short, plain English
through `tr()`, and the same thing has the same word everywhere (wording in
[TEACHING_TEXTS.md](TEACHING_TEXTS.md)). Check: new texts follow the wording
there or add to it.

## Notes

- The homepage text in [UX_DECISIONS.md](UX_DECISIONS.md) ("Homepage":
  English only, with the mascot as the star) may be outdated; the homepage is
  being redone from the project owner's own text in several languages. That
  section is left as it was until the project owner decides.
- The mockup pictures in `docs/images/posing-ux/` show an older direction.
  Where they disagree with this charter or the design system, the charter
  wins.
