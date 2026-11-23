/*
 * PinglyMotion - the top bar (see topbar.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "topbar.h"

#include <QAction>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>

#include "lucideicons.h"
#include "menuspec.h"
#include "theme.h"

namespace
{
  // Save and Export need an animation; Undo and Redo have a reason of their own
  bool needsAnimation(const MenuSpec::Button& spec)
  {
    return &spec == &MenuSpec::topSave || &spec == &MenuSpec::topExport;
  }
}

TopBar::TopBar(QToolBar* bar, const Sources& sources, QObject* parent) : QObject(parent), m_bar(bar)
{
  bar->clear();
  bar->setMovable(false);
  bar->setFloatable(false);
  bar->setIconSize(QSize(Theme::Size::iconSmall, Theme::Size::iconSmall));
  bar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  bar->setWindowTitle(QStringLiteral("Quick actions"));

  // everything to the right, as in the mockup
  QWidget* spacer = new QWidget(bar);
  spacer->setObjectName(QStringLiteral("topBarSpacer"));
  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  bar->addWidget(spacer);

  const QHash<const MenuSpec::Button*, QAction*> follow = {
    {&MenuSpec::topUndo, sources.undo}, {&MenuSpec::topRedo, sources.redo}, {&MenuSpec::topSave, sources.save},
    {&MenuSpec::topExport, sources.exportAnim}, {&MenuSpec::topScriptLab, sources.scriptLab}};

  int index = 0;
  for (const MenuSpec::Button* spec : MenuSpec::topBar)
  {
    QAction* button = new QAction(this);
    button->setObjectName(QLatin1String(spec->id));
    button->setText(MenuSpec::text(*spec));
    bar->addAction(button);
    m_buttons.append(button);
    if (QAction* source = follow.value(spec, nullptr))
    {
      m_source.insert(button, source);
      connect(source, &QAction::changed, this, [this, button]() { sync(button); });
      if (!source->isCheckable())
        connect(button, &QAction::triggered, source, [source]() { source->trigger(); });
      else
      {
        button->setCheckable(true);
        button->setChecked(source->isChecked());
        connect(source, &QAction::toggled, button, &QAction::setChecked);
        // trigger, not setChecked: a panel's toggleViewAction shows or hides its panel only on
        // triggered, so setChecked alone left the button pressed and the Script Lab panel hidden
        connect(button, &QAction::triggered, source, [source](bool on) { if (source->isChecked() != on) source->trigger(); });
      }
    }
    index++;
    for (int divider : MenuSpec::topBarDividers)
      if (divider == index) bar->addSeparator();
  }
  connect(m_buttons.at(5), &QAction::triggered, this, [this]()
  {
    if (m_buttons.at(5)->isEnabled()) emit setupRequested();
  });
  connect(m_buttons.at(6), &QAction::triggered, this, &TopBar::themeRequested);

  // the buttons that show only an icon
  for (QAction* button : m_buttons)
    if (QToolButton* tool = qobject_cast<QToolButton*>(bar->widgetForAction(button)))
    {
      tool->setObjectName(button->objectName() + QStringLiteral("Button"));
      tool->setToolButtonStyle(button->text().isEmpty() ? Qt::ToolButtonIconOnly : Qt::ToolButtonTextBesideIcon);
      tool->setFocusPolicy(Qt::StrongFocus);
    }
  m_dark = Theme::isDark();
  syncAll();
  applyLook();
}

QAction* TopBar::button(const char* id) const
{
  for (QAction* button : m_buttons)
    if (button->objectName() == QLatin1String(id)) return button;
  return nullptr;
}

QWidget* TopBar::widgetFor(QAction* button) const
{
  return m_bar->widgetForAction(button);
}

void TopBar::setAnimationOpen(bool open)
{
  m_open = open;
  syncAll();
}

void TopBar::setDark(bool dark)
{
  m_dark = dark;
  syncAll();
  applyLook();
}

void TopBar::syncAll()
{
  for (QAction* button : m_buttons) sync(button);
}

// enabled state, tip and status line of one button
void TopBar::sync(QAction* button)
{
  const MenuSpec::Button* spec = nullptr;
  for (const MenuSpec::Button* candidate : MenuSpec::topBar)
    if (button->objectName() == QLatin1String(candidate->id)) spec = candidate;
  if (!spec) return;

  QAction* source = m_source.value(button, nullptr);
  bool enabled = true;
  QString why;
  if (spec->reason[0] && (!source || !source->isEnabled()))
  {
    // not built yet: the reason of the item, for good
    enabled = false;
    why = MenuSpec::reason(*spec);
  }
  else if (needsAnimation(*spec) && !m_open)
  {
    enabled = false;
    why = QCoreApplication::translate("MenuSpec", MenuSpec::openFirst);
  }
  else if (source && !source->isEnabled())
  {
    enabled = false;
    why = source->statusTip();
  }

  QString name = MenuSpec::name(*spec);
  if (spec == &MenuSpec::topTheme)
    name = QCoreApplication::translate("MenuSpec", m_dark ? MenuSpec::themeToLight : MenuSpec::themeToDark);

  // a button greyed for a reason that never goes away (not built yet) keeps that reason
  const bool changed = button->isEnabled() != enabled;
  const QString tip = MenuSpec::tip(*spec, name, why);
  if (changed) button->setEnabled(enabled);
  if (button->toolTip() != tip) button->setToolTip(tip);
  const QString status = why.isEmpty() ? name : why;
  if (button->statusTip() != status) button->setStatusTip(status);
}

void TopBar::applyLook()
{
  using Theme::Token;
  const QColor text = Theme::color(Token::Text);
  const QColor disabled = Theme::color(Token::TextDisabled);
  for (int i = 0; i < m_buttons.size(); i++)
  {
    const MenuSpec::Button* spec = MenuSpec::topBar[i];
    QString icon = QLatin1String(spec->icon);
    if (spec == &MenuSpec::topTheme) icon = m_dark ? QStringLiteral("sun") : QStringLiteral("moon");
    m_buttons.at(i)->setIcon(LucideIcons::icon(icon, text, disabled));
  }
  const QString radius = QString::number(Theme::Size::radiusControl);
  const QColor raised = Theme::color(Token::SurfaceRaised);
  m_bar->setStyleSheet(QStringLiteral(
    "QToolBar { background: %1; border: none; spacing: %9px; padding: %9px; }"
    "QToolBar QToolButton { background: %2; color: %3; border: %7px solid %4; border-radius: %5px; padding: 4px 12px; min-height: 24px; }"
    "QToolBar QToolButton:hover { background: %6; }"
    "QToolBar QToolButton:checked { background: %6; border-color: %10; }"
    "QToolBar QToolButton:disabled { color: %8; }"
    "QToolBar QToolButton:focus { border: %11px solid %12; }")
    .arg(Theme::color(Token::Surface).name(), raised.name(), text.name(), Theme::color(Token::Border).name(), radius,
         Theme::mix(raised, text, 0.08).name())
    .arg(Theme::Size::borderWidth)
    .arg(disabled.name())
    .arg(Theme::Size::spacingS)
    .arg(Theme::color(Token::Accent).name())
    .arg(Theme::Size::focusRingWidth)
    .arg(Theme::color(Token::FocusRing).name()));
}
