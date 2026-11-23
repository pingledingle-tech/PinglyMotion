/*
 * PinglyMotion - the texts, keys and reasons of the menu items that follow
 * the clickable mockup (docs/mockup/index.html), UX phase 1a.
 *
 * One table for the window and for tests/menutexts: the window builds these
 * items from it, and the test checks every text, key and reason exactly. The
 * key is the text the menu shows to the right of the item, as the mockup
 * writes it. An item with a reason is greyed, and the reason is its tip and
 * its status line text (design charter P6: a control that cannot be used says
 * why) until the feature is built.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef MENUSPEC_H
#define MENUSPEC_H

#include <QCoreApplication>
#include <QString>

namespace MenuSpec
{
  struct Item
  {
    const char* id;       // object name of the action
    const char* text;     // as in the mockup; "..." only when a dialog follows
    const char* key;      // as the menu shows it ("" for none)
    const char* reason;   // why it is greyed ("" when it works)
  };

  // File
  inline constexpr Item fileNew = {"fileNewAction", QT_TRANSLATE_NOOP("MenuSpec", "New animation"), "Ctrl+N", ""};
  inline constexpr Item fileExport = {"fileExportAnimAction", QT_TRANSLATE_NOOP("MenuSpec", "Export for Second Life (.anim)..."), "Ctrl+Shift+E", ""};
  inline constexpr Item fileClose = {"fileCloseAction", QT_TRANSLATE_NOOP("MenuSpec", "Close animation"), "Ctrl+W", ""};

  // Edit (UX phase 1b). Undo and Redo are not built yet, so they are greyed with a reason;
  // Set key works through the key button and its reasons come from the state of the
  // window (the setKey* texts below). Paste mirrored is built since UX phase 3b; its
  // reasons come from the state too (ClipSpec::pasteBlocked, src/timelineclip.h).
  inline constexpr Item editUndo = {"editUndoAction", QT_TRANSLATE_NOOP("MenuSpec", "Undo"), "Ctrl+Z",
    QT_TRANSLATE_NOOP("MenuSpec", "Undo is not built yet. Save a copy before big changes.")};
  inline constexpr Item editRedo = {"editRedoAction", QT_TRANSLATE_NOOP("MenuSpec", "Redo"), "Ctrl+Shift+Z",
    QT_TRANSLATE_NOOP("MenuSpec", "Redo is not built yet. It comes with Undo.")};
  inline constexpr Item editSetKey = {"editSetKeyAction", QT_TRANSLATE_NOOP("MenuSpec", "Set key"), "K", ""};
  inline constexpr Item editPasteMirrored = {"editPasteMirroredAction", QT_TRANSLATE_NOOP("MenuSpec", "Paste mirrored"), "Ctrl+Shift+V", ""};

  // why a control cannot be used right now (design charter P6), not tied to one item
  inline constexpr const char* openFirst = QT_TRANSLATE_NOOP("MenuSpec", "Open an animation first.");
  inline constexpr const char* setKeyPickJoint = QT_TRANSLATE_NOOP("MenuSpec", "Pick a joint first. Click it on the figure or in the list.");
  inline constexpr const char* setKeyStopPlaying = QT_TRANSLATE_NOOP("MenuSpec", "Stop playing first.");
  inline constexpr const char* setKeyLocked = QT_TRANSLATE_NOOP("MenuSpec", "The first frame is locked. Turn off Lock the first frame to set a key.");

  // View: Look from
  inline constexpr Item viewFront = {"viewFrontAction", QT_TRANSLATE_NOOP("MenuSpec", "Front"), "Numpad 1", ""};
  inline constexpr Item viewSide = {"viewSideAction", QT_TRANSLATE_NOOP("MenuSpec", "Side"), "Numpad 3", ""};
  inline constexpr Item viewTop = {"viewTopAction", QT_TRANSLATE_NOOP("MenuSpec", "Top"), "Numpad 7", ""};
  inline constexpr Item viewHome = {"viewHomeAction", QT_TRANSLATE_NOOP("MenuSpec", "Home view"), "0", ""};
  inline constexpr const char* lookFromHeading = QT_TRANSLATE_NOOP("MenuSpec", "Look from");

  // View: Panels
  inline constexpr const char* panelsHeading = QT_TRANSLATE_NOOP("MenuSpec", "Panels");
  inline constexpr Item viewScriptLab = {"viewScriptLabAction", QT_TRANSLATE_NOOP("MenuSpec", "Script Lab"), "Ctrl+Shift+L", ""};
  // built with the Library panel (roadmap item 6, docs/UX_DECISIONS.md 6l)
  inline constexpr Item viewLibrary = {"viewLibraryAction", QT_TRANSLATE_NOOP("MenuSpec", "Library"), "Ctrl+Shift+B", ""};
  inline constexpr Item viewHidePanels = {"viewHidePanelsAction", QT_TRANSLATE_NOOP("MenuSpec", "Hide panels"), "Tab", ""};
  inline constexpr Item viewResetLayout = {"viewResetLayoutAction", QT_TRANSLATE_NOOP("MenuSpec", "Reset layout"), "", ""};
  inline constexpr Item viewNextArea = {"viewNextAreaAction", QT_TRANSLATE_NOOP("MenuSpec", "Next area"), "F6",
    QT_TRANSLATE_NOOP("MenuSpec", "Next area is not built yet. For now use Tab to move between the fields.")};

  // View: look and feel
  inline constexpr Item viewLightTheme = {"viewLightThemeAction", QT_TRANSLATE_NOOP("MenuSpec", "Light theme"), "T", ""};
  inline constexpr Item viewExpertMode = {"viewExpertModeAction", QT_TRANSLATE_NOOP("MenuSpec", "Expert mode"), "Shift+E", ""};
  inline constexpr Item viewTextSize = {"viewTextSizeAction", QT_TRANSLATE_NOOP("MenuSpec", "Text size: %1%"), "Ctrl+=", ""};
  // Ctrl+= stops at 200 percent; these two step back (UX decisions, section 8)
  inline constexpr Item viewTextSmaller = {"viewTextSmallerAction", QT_TRANSLATE_NOOP("MenuSpec", "Smaller text"), "Ctrl+-", ""};
  inline constexpr Item viewTextReset = {"viewTextResetAction", QT_TRANSLATE_NOOP("MenuSpec", "Text size back to 100%"), "Ctrl+0", ""};
  // built in UX phase 1d: the Setup dialog (src/setupdialog.*)
  inline constexpr Item viewSetup = {"viewSetupAction", QT_TRANSLATE_NOOP("MenuSpec", "Setup..."), "", ""};

  // Help
  inline constexpr Item helpSetup = {"helpSetupAction", QT_TRANSLATE_NOOP("MenuSpec", "Setup: beginner or experienced, mouse and keys..."), "", ""};
  // built in UX phase 6a: the welcome card (src/welcomedialog.*)
  inline constexpr Item helpWelcome = {"helpWelcomeAction", QT_TRANSLATE_NOOP("MenuSpec", "Show the welcome card again"), "", ""};
  inline constexpr Item helpHints = {"helpHintsAction", QT_TRANSLATE_NOOP("MenuSpec", "Show hints"), "Shift+H", ""};
  // built in UX phase 1d: the Keys and mouse dialog (src/keysmousedialog.*)
  inline constexpr Item helpKeys = {"helpKeysAction", QT_TRANSLATE_NOOP("MenuSpec", "Keys and mouse"), "?", ""};
  // built in UX phase 1d: the Camera and keys dialog (src/camerakeysdialog.*)
  inline constexpr Item helpCamera = {"helpCameraAction", QT_TRANSLATE_NOOP("MenuSpec", "Camera and keys..."), "", ""};
  inline constexpr Item helpCharter = {"helpCharterAction", QT_TRANSLATE_NOOP("MenuSpec", "Show charter principles"), "C",
    QT_TRANSLATE_NOOP("MenuSpec", "The charter overlay is not built yet.")};

  inline constexpr const Item* all[] = {
    &fileNew, &fileExport, &fileClose,
    &editUndo, &editRedo, &editSetKey, &editPasteMirrored,
    &viewFront, &viewSide, &viewTop, &viewHome,
    &viewScriptLab, &viewLibrary, &viewHidePanels, &viewResetLayout, &viewNextArea,
    &viewLightTheme, &viewExpertMode, &viewTextSize, &viewTextSmaller, &viewTextReset, &viewSetup,
    &helpSetup, &helpWelcome, &helpHints, &helpKeys, &helpCamera, &helpCharter};

  // The top bar (UX phase 1b), left to right as in the mockup: Undo, Redo | Save, Export | Script Lab,
  // Library, Setup, theme. A button with no text shows only its icon; its name is its tip. The tip is the
  // name, then the key, then the reason when it is greyed, then the tip line.
  struct Button
  {
    const char* id;       // object name of the action
    const char* text;     // on the button ("" for an icon only)
    const char* name;     // first line of the tip
    const char* key;      // as the tip shows it ("" for none)
    const char* tipline;  // last line of the tip ("" for none)
    const char* reason;   // why it is greyed for good ("" when it works)
    const char* icon;     // Lucide name
  };

  inline constexpr Button topUndo = {"topUndoAction", "", QT_TRANSLATE_NOOP("MenuSpec", "Undo"), "Ctrl+Z", "",
    editUndo.reason, "undo-2"};
  inline constexpr Button topRedo = {"topRedoAction", "", QT_TRANSLATE_NOOP("MenuSpec", "Redo"), "Ctrl+Shift+Z", "",
    editRedo.reason, "redo-2"};
  inline constexpr Button topSave = {"topSaveAction", QT_TRANSLATE_NOOP("MenuSpec", "Save"), QT_TRANSLATE_NOOP("MenuSpec", "Save"), "Ctrl+S",
    QT_TRANSLATE_NOOP("MenuSpec", "Saves your animation to a file. You can open it again later."), "", "save"};
  inline constexpr Button topExport = {"topExportAction", QT_TRANSLATE_NOOP("MenuSpec", "Export"),
    QT_TRANSLATE_NOOP("MenuSpec", "Export for Second Life"), "Ctrl+Shift+E", "", "", "upload"};
  inline constexpr Button topScriptLab = {"topScriptLabAction", QT_TRANSLATE_NOOP("MenuSpec", "Script Lab"),
    QT_TRANSLATE_NOOP("MenuSpec", "Script Lab: test RLV and LSL scripts"), "Ctrl+Shift+L", "", "", "flask-conical"};
  inline constexpr Button topLibrary = {"topLibraryAction", QT_TRANSLATE_NOOP("MenuSpec", "Library"),
    QT_TRANSLATE_NOOP("MenuSpec", "Library: ready-made moves to start from"), "Ctrl+Shift+B", "", "", "library-big"};
  inline constexpr Button topSetup = {"topSetupAction", QT_TRANSLATE_NOOP("MenuSpec", "Setup"), QT_TRANSLATE_NOOP("MenuSpec", "Setup"), "",
    QT_TRANSLATE_NOOP("MenuSpec", "Beginner or experienced, mouse and keys habits, theme, text size and hints."),
    viewSetup.reason, "settings"};
  // the theme button shows the sun in the dark theme and the moon in the light theme; its name
  // says what a click does
  inline constexpr Button topTheme = {"topThemeAction", "", QT_TRANSLATE_NOOP("MenuSpec", "Switch theme"), "T", "", "", "sun"};
  inline constexpr const char* themeToLight = QT_TRANSLATE_NOOP("MenuSpec", "Switch to the light theme");
  inline constexpr const char* themeToDark = QT_TRANSLATE_NOOP("MenuSpec", "Switch to the dark theme");

  inline constexpr const Button* topBar[] = {&topUndo, &topRedo, &topSave, &topExport, &topScriptLab, &topLibrary, &topSetup, &topTheme};
  // a divider stands after the second and the fourth button
  inline constexpr int topBarDividers[] = {2, 4};

  inline QString text(const Item& item)
  {
    return QCoreApplication::translate("MenuSpec", item.text);
  }

  inline QString reason(const Item& item)
  {
    return QCoreApplication::translate("MenuSpec", item.reason);
  }

  inline QString text(const Button& button)
  {
    return QCoreApplication::translate("MenuSpec", button.text);
  }

  inline QString name(const Button& button)
  {
    return QCoreApplication::translate("MenuSpec", button.name);
  }

  inline QString reason(const Button& button)
  {
    return QCoreApplication::translate("MenuSpec", button.reason);
  }

  inline QString tipline(const Button& button)
  {
    return QCoreApplication::translate("MenuSpec", button.tipline);
  }

  // the tip of a button: name and key, then why it is greyed (when it is), then the tip line
  inline QString tip(const Button& button, const QString& shownName, const QString& why)
  {
    QString out = shownName;
    if (button.key[0]) out += QStringLiteral("  (") + QLatin1String(button.key) + QLatin1Char(')');
    if (!why.isEmpty()) out += QLatin1Char('\n') + why;
    if (button.tipline[0]) out += QLatin1Char('\n') + tipline(button);
    return out;
  }

  // what the menu shows: "text<TAB>key", the part after the tab is drawn at the right
  inline QString label(const Item& item, const QString& shownText)
  {
    return item.key[0] ? shownText + QLatin1Char('\t') + QLatin1String(item.key) : shownText;
  }
}

#endif
