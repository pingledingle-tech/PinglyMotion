/*
 * PinglyMotion - the view toolbar at the top left of the 3D view (see viewtoolbar.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "viewtoolbar.h"

#include <QAction>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionToolButton>
#include <QVBoxLayout>

#include "lucideicons.h"
#include "textsize.h"
#include "theme.h"
#include "viewgizmo.h"

// ---------- one button ----------

ViewToolButton::ViewToolButton(const ViewToolSpec::Button& spec, bool menu, QWidget* parent)
  : QToolButton(parent), m_icon(QLatin1String(spec.icon)), m_label(ViewToolSpec::tr(spec.label)),
    m_key(QLatin1String(spec.key)), m_menu(menu)
{
  setObjectName(QLatin1String(spec.id));
  setToolTip(ViewToolSpec::tip(spec));
  setAccessibleName(ViewToolSpec::tr(spec.name));
  setText(m_label);
  setFocusPolicy(Qt::TabFocus);
  setAttribute(Qt::WA_Hover);
  setCursor(Qt::ArrowCursor);
  if (menu) setPopupMode(QToolButton::InstantPopup);
}

void ViewToolButton::setShowLabel(bool show)
{
  if (show == m_showLabel) return;
  m_showLabel = show;
  updateGeometry();
  update();
}

namespace
{
  // the mockup's toolbar buttons: 26 pixels high with names, 28 square as icons (at 100 percent)
  constexpr int NamedHeight = 26;
  constexpr int IconSide = 28;
  int ringMargin() { return Theme::Size::focusRingOffset + Theme::Size::focusRingWidth; }
}

QSize ViewToolButton::sizeHint() const
{
  const int ring = ringMargin();
  if (!m_showLabel)
  {
    const int side = qMax(TextSize::scaled(IconSide), fontMetrics().height() + 2 * Theme::Size::spacingXs);
    return QSize(side + 2 * ring, side + 2 * ring);
  }
  QFont bold = font();
  bold.setBold(true);
  const QFontMetrics metrics(bold);
  const int height = qMax(TextSize::scaled(NamedHeight), metrics.height() + Theme::Size::spacingXs);
  const int icon = TextSize::scaled(Theme::Size::iconSmall);
  int width = Theme::Size::spacingS + icon + Theme::Size::spacingS + metrics.horizontalAdvance(m_label);
  if (m_menu) width += Theme::Size::spacingS + icon;
  else if (!m_key.isEmpty()) width += Theme::Size::spacingS + metrics.horizontalAdvance(m_key) + 2 * Theme::Size::spacingXs;
  width += Theme::Size::spacingS;
  return QSize(width + 2 * ring, height + 2 * ring);
}

void ViewToolButton::paintEvent(QPaintEvent*)
{
  using Theme::Token;
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  const int ring = ringMargin();
  const QRectF box = QRectF(rect()).adjusted(ring + 0.5, ring + 0.5, -ring - 0.5, -ring - 0.5);
  const bool on = isChecked();
  const bool hover = underMouse() && isEnabled();
  const bool pressed = isDown();

  // the mockup's .vtools .btn: no fill and no border; hover surface_raised; pressed mixes 16
  // percent text in; the active tool selection mixed 22 percent into surface, a selection
  // border, bold text and an accent_text bar on its left (so not by colour alone)
  QColor fill = Qt::transparent;
  if (on) fill = Theme::mix(Theme::color(Token::Surface), Theme::color(Token::Selection), 0.22);
  if (hover) fill = Theme::color(Token::SurfaceRaised);
  if (pressed) fill = Theme::mix(hover || on ? fill : Theme::color(Token::Surface), Theme::color(Token::Text), 0.16);
  painter.setPen(on ? QPen(Theme::color(Token::Selection), Theme::Size::borderWidth) : Qt::NoPen);
  painter.setBrush(fill);
  painter.drawRoundedRect(box, Theme::Size::radiusControl, Theme::Size::radiusControl);
  if (on && m_showLabel)
  {
    painter.setPen(Qt::NoPen);
    painter.setBrush(Theme::color(Token::AccentText));
    painter.drawRoundedRect(QRectF(0, box.top() + 6, 3, qMax(2.0, box.height() - 12)), 1.5, 1.5);
  }

  const QColor text = isEnabled() ? Theme::color(Token::Text) : Theme::color(Token::TextDisabled);
  const int icon = TextSize::scaled(Theme::Size::iconSmall);
  const QIcon picture = LucideIcons::icon(m_icon, text, Theme::color(Token::TextDisabled));
  if (!m_showLabel)
  {
    const QRect iconRect(int(box.center().x() - icon / 2.0 + 0.5), int(box.center().y() - icon / 2.0 + 0.5), icon, icon);
    picture.paint(&painter, iconRect);
  }
  else
  {
    QFont labelFont = font();
    labelFont.setBold(on);
    double x = box.left() + Theme::Size::spacingS;
    const QRect iconRect(int(x), int(box.center().y() - icon / 2.0 + 0.5), icon, icon);
    picture.paint(&painter, iconRect);
    x += icon + Theme::Size::spacingS;
    double right = box.right() - Theme::Size::spacingS;
    if (m_menu)
    {
      const QRect chevron(int(right - icon), int(box.center().y() - icon / 2.0 + 0.5), icon, icon);
      LucideIcons::icon(QStringLiteral("chevron-right"), text, Theme::color(Token::TextDisabled)).paint(&painter, chevron);
      right -= icon + Theme::Size::spacingS;
    }
    else if (!m_key.isEmpty())
    {
      // the key as the mockup's kbd: a small frame in border, the key in text_muted
      const QFontMetrics metrics(font());
      const double keyWidth = metrics.horizontalAdvance(m_key) + 2 * Theme::Size::spacingXs;
      const double keyHeight = qMin(box.height() - 4, double(metrics.height()));
      const QRectF keyBox(right - keyWidth, box.center().y() - keyHeight / 2.0, keyWidth, keyHeight);
      painter.setFont(font());
      painter.setPen(QPen(Theme::color(Token::Border), Theme::Size::borderWidth));
      painter.setBrush(Qt::NoBrush);
      painter.drawRoundedRect(keyBox, Theme::Size::radiusSmall, Theme::Size::radiusSmall);
      painter.setPen(Theme::color(Token::TextMuted));
      painter.drawText(keyBox, Qt::AlignCenter, m_key);
      right -= keyWidth + Theme::Size::spacingS;
    }
    painter.setFont(labelFont);
    painter.setPen(text);
    painter.drawText(QRectF(x, box.top(), right - x, box.height()), Qt::AlignLeft | Qt::AlignVCenter, m_label);
  }

  if (hasFocus())
  {
    const double inset = Theme::Size::focusRingWidth / 2.0;
    painter.setPen(QPen(Theme::color(Token::FocusRing), Theme::Size::focusRingWidth));
    painter.setBrush(Qt::NoBrush);
    const double radius = Theme::Size::radiusControl + Theme::Size::focusRingOffset + inset;
    painter.drawRoundedRect(QRectF(rect()).adjusted(inset, inset, -inset, -inset), radius, radius);
  }
}

// ---------- the toolbar ----------

ViewToolbar::ViewToolbar(QWidget* parent) : QWidget(parent)
{
  setObjectName(QStringLiteral("viewToolbar"));
  setAttribute(Qt::WA_TranslucentBackground);
  setAttribute(Qt::WA_NoSystemBackground);

  m_gizmo = new ViewGizmo(this);
  connect(m_gizmo, &ViewGizmo::endChosen, this, &ViewToolbar::endChosen);
  connect(m_gizmo, &ViewGizmo::dragged, this, &ViewToolbar::dragged);

  m_viewName = new QLabel(this);
  m_viewName->setObjectName(QStringLiteral("viewName"));
  m_viewName->setAlignment(Qt::AlignCenter);

  m_panel = new QFrame(this);
  m_panel->setObjectName(QStringLiteral("viewToolsPanel"));
  m_panel->setAccessibleName(ViewToolSpec::tr(ViewToolSpec::toolbarName));

  for (ViewTool tool : {ViewTool::Pick, ViewTool::Orbit, ViewTool::Pan, ViewTool::Zoom})
  {
    ViewToolButton* button = addButton(ViewToolSpec::tool(tool));
    button->setCheckable(true);
    connect(button, &QToolButton::clicked, this, [this, tool, button]()
    {
      // the window decides; until it does the button shows the tool that is on
      button->setChecked(tool == m_tool);
      emit toolChosen(tool);
    });
  }
  m_order.append(nullptr);
  connect(addButton(ViewToolSpec::focus), &QToolButton::clicked, this, &ViewToolbar::focusRequested);
  connect(addButton(ViewToolSpec::wholeFigure), &QToolButton::clicked, this, &ViewToolbar::wholeFigureRequested);
  connect(addButton(ViewToolSpec::home), &QToolButton::clicked, this, &ViewToolbar::homeRequested);
  m_order.append(nullptr);

  ViewToolButton* look = addButton(ViewToolSpec::lookFrom, true);
  m_lookMenu = new QMenu(look);
  m_lookMenu->setObjectName(QStringLiteral("viewLookFromMenu"));
  for (int i = 0; i < ViewToolSpec::lookCount; i++)
  {
    QAction* action = m_lookMenu->addAction(ViewToolSpec::tr(ViewToolSpec::looks[i].text) + QLatin1Char('\t') +
                                            QLatin1String(ViewToolSpec::looks[i].key));
    connect(action, &QAction::triggered, this, [this, i]() { emit lookRequested(i); });
  }
  look->setMenu(m_lookMenu);

  ViewToolButton* show = addButton(ViewToolSpec::show, true);
  m_showMenu = new QMenu(show);
  m_showMenu->setObjectName(QStringLiteral("viewShowMenu"));
  for (Part part : ViewToolSpec::parts)
  {
    QAction* action = m_showMenu->addAction(ViewToolSpec::partName(part));
    action->setCheckable(true);
    action->setChecked(true);
    connect(action, &QAction::triggered, this, [this, part, action](bool on)
    {
      // the window decides and sets it with setPartShown()
      action->setChecked(!on);
      emit partToggled(part, on);
    });
    m_showActions.insert(int(part), action);
  }
  show->setMenu(m_showMenu);

  // the widget and the name above a panel that holds the buttons (the mockup's .vtools:
  // surface, a border, radius.panel, padding spacing.xs)
  QVBoxLayout* outer = new QVBoxLayout(this);
  outer->setContentsMargins(0, 0, 0, 0);
  outer->setSpacing(Theme::Size::spacingXs);
  outer->setSizeConstraint(QLayout::SetFixedSize);
  outer->addWidget(m_gizmo, 0, Qt::AlignHCenter);
  outer->addWidget(m_viewName, 0, Qt::AlignHCenter);
  outer->addWidget(m_panel);
  m_grid = new QGridLayout(m_panel);
  m_grid->setContentsMargins(Theme::Size::spacingXs - 2, Theme::Size::spacingXs - 2, Theme::Size::spacingXs - 2, Theme::Size::spacingXs - 2);
  m_grid->setSpacing(0);

  setTool(ViewTool::Pick);
  setCameraAngles(10.0f, 0.0f);
  applyLook();
  fillGrid(false, true);

  if (parent)
  {
    parent->installEventFilter(this);
    relayout();
  }
}

ViewToolButton* ViewToolbar::addButton(const ViewToolSpec::Button& spec, bool menu)
{
  ViewToolButton* button = new ViewToolButton(spec, menu, m_panel);
  m_buttons.append(button);
  m_order.append(button);
  return button;
}

ViewToolButton* ViewToolbar::button(const char* id) const
{
  for (ViewToolButton* button : m_buttons)
    if (button->objectName() == QLatin1String(id)) return button;
  return nullptr;
}

ViewToolButton* ViewToolbar::toolButton(ViewTool tool) const
{
  return button(ViewToolSpec::tool(tool).id);
}

QAction* ViewToolbar::showAction(Part part) const
{
  return m_showActions.value(int(part));
}

void ViewToolbar::setTool(ViewTool tool)
{
  m_tool = tool;
  for (ViewTool t : {ViewTool::Pick, ViewTool::Orbit, ViewTool::Pan, ViewTool::Zoom})
    if (ViewToolButton* b = toolButton(t)) b->setChecked(t == tool);
}

void ViewToolbar::setPartShown(Part part, bool shown)
{
  QAction* action = showAction(part);
  if (!action) return;
  const bool changed = action->isChecked() != shown;
  action->setChecked(shown);
  if (part == Part::ToolNames && changed) relayout();
}

bool ViewToolbar::partShown(Part part) const
{
  QAction* action = showAction(part);
  return action && action->isChecked();
}

void ViewToolbar::setCameraAngles(float height, float angle)
{
  m_gizmo->setCameraAngles(height, angle);
  const QString name = ViewToolSpec::viewName(Camera::viewNameFor(height, angle));
  if (m_viewName->text() != name)
  {
    m_viewName->setText(name);
    m_viewName->setAccessibleName(name);
  }
}

QString ViewToolbar::viewNameText() const
{
  return m_viewName->text();
}

void ViewToolbar::applyLook()
{
  using Theme::Token;
  QColor fill = Theme::color(Token::Surface);
  fill.setAlphaF(0.85);
  // the mockup's .viewname: caption text on a round pill
  m_viewName->setStyleSheet(QStringLiteral("QLabel#viewName { color: %1; background: rgba(%2,%3,%4,%5); border: %6px solid %7;"
                                           " border-radius: %8px; padding: 0px %9px; }")
                              .arg(Theme::color(Token::Text).name())
                              .arg(fill.red()).arg(fill.green()).arg(fill.blue()).arg(fill.alpha())
                              .arg(Theme::Size::borderWidth).arg(Theme::color(Token::Border).name())
                              .arg(qMax(1, m_viewName->sizeHint().height() / 2)).arg(Theme::Size::spacingS));
  m_panel->setStyleSheet(QStringLiteral("QFrame#viewToolsPanel { background: %1; border: %2px solid %3; border-radius: %4px; }")
                           .arg(Theme::color(Token::Surface).name()).arg(Theme::Size::borderWidth)
                           .arg(Theme::color(Token::Border).name()).arg(Theme::Size::radiusPanel));
  for (QWidget* divider : m_dividers) divider->setStyleSheet(dividerStyle());
  for (ViewToolButton* button : m_buttons) button->update();
  m_gizmo->update();
}

QString ViewToolbar::dividerStyle() const
{
  QColor line = Theme::color(Theme::Token::Border);
  line.setAlphaF(0.6);
  return QStringLiteral("background: rgba(%1,%2,%3,%4); margin: 0px %5px;").arg(line.red()).arg(line.green()).arg(line.blue())
           .arg(line.alpha()).arg(ringMargin());
}

void ViewToolbar::fillGrid(bool twoColumns, bool labels)
{
  while (QLayoutItem* item = m_grid->takeAt(0)) delete item;
  // rows of an earlier layout keep their minimum heights otherwise
  for (int r = 0; r < m_grid->rowCount(); r++) m_grid->setRowMinimumHeight(r, 0);
  for (QWidget* divider : m_dividers) divider->deleteLater();
  m_dividers.clear();

  const int columns = twoColumns ? 2 : 1;
  int row = 0;
  int column = 0;
  for (QWidget* widget : m_order)
  {
    if (!widget)
    {
      if (column) { row++; column = 0; }
      // the mockup's hr: border at 60 percent, spacing.xs above and below
      QFrame* divider = new QFrame(m_panel);
      divider->setObjectName(QStringLiteral("viewToolDivider"));
      divider->setFixedHeight(Theme::Size::borderWidth);
      divider->setStyleSheet(dividerStyle());
      divider->show();
      m_dividers.append(divider);
      m_grid->setRowMinimumHeight(row++, Theme::Size::spacingXs - 2);
      m_grid->addWidget(divider, row++, 0, 1, columns);
      m_grid->setRowMinimumHeight(row++, Theme::Size::spacingXs - 2);
      continue;
    }
    ViewToolButton* button = static_cast<ViewToolButton*>(widget);
    button->setShowLabel(labels);
    m_grid->addWidget(button, row, column, Qt::AlignLeft);
    if (++column == columns) { column = 0; row++; }
  }
  m_twoColumns = twoColumns;
  // the buttons of one column all as wide as the widest (the mockup's toolbar)
  for (ViewToolButton* button : m_buttons)
  {
    button->setMinimumWidth(0);
  }
  if (!twoColumns && labels)
  {
    int widest = 0;
    for (ViewToolButton* button : m_buttons) widest = qMax(widest, button->sizeHint().width());
    for (ViewToolButton* button : m_buttons) button->setMinimumWidth(widest);
  }
  m_grid->invalidate();
  m_grid->activate();
  layout()->invalidate();
  layout()->activate();
  adjustSize();
}

void ViewToolbar::relayout()
{
  if (m_relayouting) return;
  m_relayouting = true;
  const int margin = Theme::Size::spacingM;
  const bool labels = partShown(Part::ToolNames);
  QWidget* parent = parentWidget();
  const int available = parent ? parent->height() - 2 * margin : INT_MAX;
  fillGrid(false, labels);
  // too low for one column: two columns of icons (an icon button is as high as a named one)
  if (sizeHint().height() > available) fillGrid(true, false);
  if (parent) move(margin, margin);
  m_relayouting = false;
}

bool ViewToolbar::eventFilter(QObject* watched, QEvent* event)
{
  if (watched == parentWidget() && event->type() == QEvent::Resize) relayout();
  if (watched == parentWidget() && event->type() == QEvent::FontChange) relayout();
  return QWidget::eventFilter(watched, event);
}
