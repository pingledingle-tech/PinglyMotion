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
  inline constexpr Item fileClose = {"fileCloseAction", QT_TRANSLATE_NOOP("MenuSpec", "Close animation"), "Ctrl+W",
    QT_TRANSLATE_NOOP("MenuSpec", "Close animation is not built yet. Use New animation to start again.")};

  // View: Look from
  inline constexpr Item viewFront = {"viewFrontAction", QT_TRANSLATE_NOOP("MenuSpec", "Front"), "Numpad 1", ""};
  inline constexpr Item viewSide = {"viewSideAction", QT_TRANSLATE_NOOP("MenuSpec", "Side"), "Numpad 3", ""};
  inline constexpr Item viewTop = {"viewTopAction", QT_TRANSLATE_NOOP("MenuSpec", "Top"), "Numpad 7", ""};
  inline constexpr Item viewHome = {"viewHomeAction", QT_TRANSLATE_NOOP("MenuSpec", "Home view"), "0", ""};
  inline constexpr const char* lookFromHeading = QT_TRANSLATE_NOOP("MenuSpec", "Look from");

  // View: Panels
  inline constexpr const char* panelsHeading = QT_TRANSLATE_NOOP("MenuSpec", "Panels");
  inline constexpr Item viewScriptLab = {"viewScriptLabAction", QT_TRANSLATE_NOOP("MenuSpec", "Script Lab"), "Ctrl+Shift+L", ""};
  inline constexpr Item viewHidePanels = {"viewHidePanelsAction", QT_TRANSLATE_NOOP("MenuSpec", "Hide panels"), "Tab", ""};
  inline constexpr Item viewResetLayout = {"viewResetLayoutAction", QT_TRANSLATE_NOOP("MenuSpec", "Reset layout"), "", ""};
  inline constexpr Item viewNextArea = {"viewNextAreaAction", QT_TRANSLATE_NOOP("MenuSpec", "Next area"), "F6",
    QT_TRANSLATE_NOOP("MenuSpec", "Next area is not built yet. For now use Tab to move between the fields.")};

  // View: look and feel
  inline constexpr Item viewLightTheme = {"viewLightThemeAction", QT_TRANSLATE_NOOP("MenuSpec", "Light theme"), "T", ""};
  inline constexpr Item viewExpertMode = {"viewExpertModeAction", QT_TRANSLATE_NOOP("MenuSpec", "Expert mode"), "Shift+E", ""};
  inline constexpr Item viewTextSize = {"viewTextSizeAction", QT_TRANSLATE_NOOP("MenuSpec", "Text size: %1%"), "Ctrl+=", ""};
  inline constexpr Item viewSetup = {"viewSetupAction", QT_TRANSLATE_NOOP("MenuSpec", "Setup..."), "",
    QT_TRANSLATE_NOOP("MenuSpec", "Setup is not built yet. For now use Options > Settings.")};

  // Help
  inline constexpr Item helpSetup = {"helpSetupAction", QT_TRANSLATE_NOOP("MenuSpec", "Setup: beginner or experienced, mouse and keys..."), "",
    QT_TRANSLATE_NOOP("MenuSpec", "Setup is not built yet. For now use Options > Settings.")};
  inline constexpr Item helpWelcome = {"helpWelcomeAction", QT_TRANSLATE_NOOP("MenuSpec", "Show the welcome card again"), "",
    QT_TRANSLATE_NOOP("MenuSpec", "The welcome card is not built yet.")};
  inline constexpr Item helpHints = {"helpHintsAction", QT_TRANSLATE_NOOP("MenuSpec", "Show hints"), "Shift+H", ""};
  inline constexpr Item helpKeys = {"helpKeysAction", QT_TRANSLATE_NOOP("MenuSpec", "Keys and mouse"), "?",
    QT_TRANSLATE_NOOP("MenuSpec", "The keys and mouse page is not built yet.")};
  inline constexpr Item helpCamera = {"helpCameraAction", QT_TRANSLATE_NOOP("MenuSpec", "Camera and keys..."), "",
    QT_TRANSLATE_NOOP("MenuSpec", "Camera and keys is not built yet. It comes with Setup.")};
  inline constexpr Item helpCharter = {"helpCharterAction", QT_TRANSLATE_NOOP("MenuSpec", "Show charter principles"), "C",
    QT_TRANSLATE_NOOP("MenuSpec", "The charter overlay is not built yet.")};

  inline constexpr const Item* all[] = {
    &fileNew, &fileExport, &fileClose,
    &viewFront, &viewSide, &viewTop, &viewHome,
    &viewScriptLab, &viewHidePanels, &viewResetLayout, &viewNextArea,
    &viewLightTheme, &viewExpertMode, &viewTextSize, &viewSetup,
    &helpSetup, &helpWelcome, &helpHints, &helpKeys, &helpCamera, &helpCharter};

  inline QString text(const Item& item)
  {
    return QCoreApplication::translate("MenuSpec", item.text);
  }

  inline QString reason(const Item& item)
  {
    return QCoreApplication::translate("MenuSpec", item.reason);
  }

  // what the menu shows: "text<TAB>key", the part after the tab is drawn at the right
  inline QString label(const Item& item, const QString& shownText)
  {
    return item.key[0] ? shownText + QLatin1Char('\t') + QLatin1String(item.key) : shownText;
  }
}

#endif
