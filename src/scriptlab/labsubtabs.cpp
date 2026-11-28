/*
 * PinglyMotion Script Lab - sub-tabs on the left of a Script Lab tab (see labsubtabs.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "labsubtabs.h"

#include <QBoxLayout>
#include <QEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

#include "labtabs.h"
#include "paneltitlebar.h"
#include "scriptlabsection.h"
#include "textsize.h"
#include "theme.h"

namespace
{
  // design_tokens.json: spacing.xs, spacing.s, spacing.m, control_height.compact, radius.control
  const int SpacingXs = 4;
  const int SpacingS = 8;
  const int SpacingM = 12;
  const int ControlCompact = 24;
  const int ControlHeight = 32;
  const int RadiusControl = 6;
  const int IconSmall = 16;

  QString hex(Theme::Token token, bool dark)
  {
    return Theme::color(token, dark).name(QColor::HexRgb).toUpper();
  }

  // the box layout under `layout` that holds `widget`, and its place in it
  QBoxLayout* findBox(QLayout* layout, QWidget* widget, int* index)
  {
    if (!layout) return nullptr;
    if (QBoxLayout* box = qobject_cast<QBoxLayout*>(layout))
    {
      const int at = box->indexOf(widget);
      if (at >= 0) { *index = at; return box; }
    }
    for (int i = 0; i < layout->count(); ++i)
      if (QLayout* inner = layout->itemAt(i)->layout())
        if (QBoxLayout* found = findBox(inner, widget, index)) return found;
    return nullptr;
  }
}

// --- the remembered state ---------------------------------------------------------------------

LabSubTabState::State LabSubTabState::parse(const QString& text, const QMap<QString, QStringList>& groups, const QStringList& units)
{
  State state;
  const QString cut = text.left(MaxLength);
  // the popped ones and their places have the same form as the tabs' own state (labtabs.h)
  const LabTabState::State windows = LabTabState::parse(cut, units);
  state.popped = windows.popped;
  state.places = windows.places;
  for (const QString& rawEntry : cut.split(QLatin1Char(';'), Qt::SkipEmptyParts))
  {
    const QString entry = rawEntry.trimmed();
    if (!entry.startsWith(QLatin1String("sub="))) continue;
    const QString value = entry.mid(4);
    const int colon = value.indexOf(QLatin1Char(':'));
    if (colon <= 0) continue;
    const QString tab = value.left(colon);
    const QString group = value.mid(colon + 1);
    if (groups.value(tab).contains(group)) state.current.insert(tab, group);
  }
  return state;
}

QString LabSubTabState::format(const State& state)
{
  QStringList entries;
  for (auto it = state.current.constBegin(); it != state.current.constEnd(); ++it)
    if (!it.value().isEmpty()) entries << QStringLiteral("sub=%1:%2").arg(it.key(), it.value());
  LabTabState::State windows;
  windows.popped = state.popped;
  windows.places = state.places;
  const QString rest = LabTabState::format(windows);
  if (!rest.isEmpty()) entries << rest;
  return entries.join(QLatin1Char(';')).left(MaxLength);
}

// --- the stack, as tall as its page in front --------------------------------------------------

LabSubStack::LabSubStack(QWidget* parent) :
  QStackedWidget(parent)
{
  connect(this, &QStackedWidget::currentChanged, this, [this]() { updateGeometry(); });
}

QSize LabSubStack::sizeHint() const
{
  QWidget* page = currentWidget();
  return page ? page->sizeHint().expandedTo(page->minimumSizeHint()) : QSize(0, 0);
}

QSize LabSubStack::minimumSizeHint() const
{
  QWidget* page = currentWidget();
  return page ? page->minimumSizeHint() : QSize(0, 0);
}

// --- the sub-tabs -----------------------------------------------------------------------------

LabSubTabs::LabSubTabs(const QString& tabKey, QWidget* parent) :
  QWidget(parent),
  m_tabKey(tabKey)
{
  setObjectName(QStringLiteral("scriptLabSubTabs_") + tabKey);
  QHBoxLayout* outer = new QHBoxLayout(this);
  outer->setContentsMargins(0, 0, 0, 0);
  outer->setSpacing(SpacingM);

  // the list on the left: one row per group, the one in front picked
  m_list = new QListWidget(this);
  m_list->setObjectName(QStringLiteral("scriptLabSubTabList_") + tabKey);
  m_list->setAccessibleName(tr("Parts of this tab"));
  m_list->setToolTip(tr("Pick a part to show it on the right. Up and Down keys move between them."));
  m_list->setFocusPolicy(Qt::StrongFocus);
  m_list->setSelectionMode(QAbstractItemView::SingleSelection);
  m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_list->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  m_list->setTextElideMode(Qt::ElideNone);
  m_list->setWordWrap(false);
  m_list->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  outer->addWidget(m_list, 0, Qt::AlignTop);

  // the right side: the group's name and Pop out on top, the group under it
  QWidget* right = new QWidget(this);
  right->setObjectName(QStringLiteral("scriptLabSubTabRight_") + tabKey);
  QVBoxLayout* column = new QVBoxLayout(right);
  column->setContentsMargins(0, 0, 0, 0);
  column->setSpacing(SpacingS);
  QHBoxLayout* top = new QHBoxLayout;
  top->setContentsMargins(0, 0, 0, 0);
  top->setSpacing(SpacingS);
  m_title = new QLabel(right);
  m_title->setObjectName(QStringLiteral("scriptLabSubTabTitle_") + tabKey);
  QFont label = m_title->font();   // the "label" type token
  label.setWeight(QFont::DemiBold);
  m_title->setFont(label);
  top->addWidget(m_title, 1);
  m_popOut = new QToolButton(right);
  m_popOut->setObjectName(QStringLiteral("scriptLabSubTabPopOut_") + tabKey);
  m_popOut->setText(tr("Pop out"));
  m_popOut->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  m_popOut->setAutoRaise(true);
  m_popOut->setFocusPolicy(Qt::StrongFocus);
  m_popOut->setMinimumHeight(ControlCompact);
  top->addWidget(m_popOut);
  column->addLayout(top);
  m_stack = new LabSubStack(right);
  m_stack->setObjectName(QStringLiteral("scriptLabSubTabStack_") + tabKey);
  column->addWidget(m_stack);
  outer->addWidget(right, 1);

  connect(m_list, &QListWidget::currentRowChanged, this, [this](int row)
  {
    if (m_refreshing || row < 0) return;
    m_autoPicked = false;   // picked by the user (or a saved state): it stays in front while listed
    const QString key = m_list->item(row)->data(Qt::UserRole).toString();
    if (m_groups.contains(key)) m_stack->setCurrentWidget(m_groups[key].page);
    refreshHeader();
    emit stateChanged();
  });
  // a click on a row that is in a window of its own brings that window to the front
  connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem* item)
  {
    if (LabPopWindow* window = m_windows.value(item->data(Qt::UserRole).toString()))
    {
      window->raise();
      window->activateWindow();
    }
  });
  connect(m_popOut, &QToolButton::clicked, this, [this]() { popOut(currentGroup()); });
  applyStyle(palette().color(QPalette::Window).lightness() < 128);
  refreshHeader();
}

QVBoxLayout* LabSubTabs::addGroup(const QString& key, const QString& name, const QString& tip)
{
  Group g;
  g.key = key;
  g.name = name;
  g.page = new QWidget(m_stack);
  g.page->setObjectName(QStringLiteral("scriptLabSubTabPage_") + key);
  QVBoxLayout* pageLayout = new QVBoxLayout(g.page);
  pageLayout->setContentsMargins(0, 0, 0, 0);
  pageLayout->setSpacing(SpacingS);
  g.content = new QWidget(g.page);
  g.content->setObjectName(QStringLiteral("scriptLabSubTabContent_") + key);
  g.contentLayout = new QVBoxLayout(g.content);
  g.contentLayout->setContentsMargins(0, 0, 0, 0);
  g.contentLayout->setSpacing(SpacingS);
  pageLayout->addWidget(g.content);
  pageLayout->addStretch(1);
  m_stack->addWidget(g.page);
  m_groups.insert(key, g);
  m_order << key;

  QListWidgetItem* item = new QListWidgetItem(name, m_list);
  item->setData(Qt::UserRole, key);
  item->setToolTip(tip);
  item->setData(Qt::AccessibleDescriptionRole, tip);
  refreshList();
  return g.contentLayout;
}

void LabSubTabs::setGroupParts(const QString& group, const QList<ScriptLabSection*>& parts)
{
  if (!m_groups.contains(group)) return;
  m_groups[group].parts = parts;
  for (ScriptLabSection* part : parts) part->installEventFilter(this);
  refreshList();
}

void LabSubTabs::addPart(const QString& group, ScriptLabSection* section)
{
  if (!m_groups.contains(group) || !section || m_parts.contains(section->key())) return;
  Part p;
  p.key = section->key();
  p.group = group;
  p.section = section;
  p.pop = new QToolButton(section);
  p.pop->setObjectName(QStringLiteral("scriptLabPartPopOut_") + p.key);
  p.pop->setAutoRaise(true);
  p.pop->setFocusPolicy(Qt::StrongFocus);
  p.pop->setMinimumSize(ControlCompact, ControlCompact);
  p.pop->setToolButtonStyle(Qt::ToolButtonIconOnly);
  p.pop->setAccessibleName(tr("Pop out %1").arg(section->title()));
  const QString tip = tr("Pop out \"%1\" alone into a window of its own. Close that window, or press Put back, to bring it back.")
                        .arg(section->title());
  p.pop->setToolTip(tip);
  p.pop->setStatusTip(tip);
  section->addHeaderWidget(p.pop);
  // the keys go from the part's header to its Pop out, then into the part
  QWidget::setTabOrder(section->header(), p.pop);
  p.placeholder = nullptr;
  const QString key = p.key;
  connect(p.pop, &QToolButton::clicked, this, [this, key]() { popOut(key); });
  m_parts.insert(key, p);
  if (!m_groups[group].parts.contains(section))
  {
    m_groups[group].parts << section;
    section->installEventFilter(this);
  }
  stylePopButton(p.pop);
  refreshList();
}

QString LabSubTabs::groupName(const QString& key) const
{
  return m_groups.contains(key) ? m_groups[key].name : QString();
}

QStringList LabSubTabs::partKeys() const
{
  QStringList keys;
  for (const QString& g : m_order)
    for (ScriptLabSection* s : m_groups[g].parts)
      if (m_parts.contains(s->key())) keys << s->key();
  return keys;
}

QStringList LabSubTabs::unitKeys() const
{
  return m_order + partKeys();
}

QString LabSubTabs::groupOfPart(const QString& part) const
{
  return m_parts.contains(part) ? m_parts[part].group : QString();
}

QWidget* LabSubTabs::groupContent(const QString& key) const
{
  return m_groups.contains(key) ? m_groups[key].content : nullptr;
}

QWidget* LabSubTabs::groupPlaceholder(const QString& key) const
{
  return m_groups.contains(key) ? m_groups[key].placeholder : nullptr;
}

QToolButton* LabSubTabs::partPopButton(const QString& part) const
{
  return m_parts.contains(part) ? m_parts[part].pop : nullptr;
}

QWidget* LabSubTabs::partPlaceholder(const QString& part) const
{
  return m_parts.contains(part) ? m_parts[part].placeholder : nullptr;
}

ScriptLabSection* LabSubTabs::part(const QString& key) const
{
  return m_parts.contains(key) ? m_parts[key].section : nullptr;
}

int LabSubTabs::rowOf(const QString& key) const
{
  for (int row = 0; row < m_list->count(); ++row)
    if (m_list->item(row)->data(Qt::UserRole).toString() == key) return row;
  return -1;
}

QString LabSubTabs::currentGroup() const
{
  QListWidgetItem* item = m_list->currentItem();
  return item && !item->isHidden() ? item->data(Qt::UserRole).toString() : QString();
}

bool LabSubTabs::isListed(const QString& key) const
{
  const int row = rowOf(key);
  return row >= 0 && !m_list->item(row)->isHidden();
}

bool LabSubTabs::setCurrentGroup(const QString& key)
{
  if (!isListed(key)) return false;
  m_list->setCurrentRow(rowOf(key));
  if (LabPopWindow* window = m_windows.value(key); window && m_windowsShown)
  {
    window->show();
    window->raise();
  }
  return true;
}

QWidget* LabSubTabs::makePlaceholder(const QString& key, const QString& name, QWidget* parent)
{
  // in the place of what is out: what it is and the way back, so nothing seems lost (charter P6)
  QWidget* box = new QWidget(parent);
  box->setObjectName(QStringLiteral("scriptLabOutLine_") + key);
  QHBoxLayout* row = new QHBoxLayout(box);
  row->setContentsMargins(0, SpacingXs, 0, SpacingXs);
  row->setSpacing(SpacingS);
  QLabel* line = new QLabel(tr("\"%1\" is in a window of its own.").arg(name), box);
  line->setObjectName("scriptLabOutText");
  line->setWordWrap(true);
  row->addWidget(line, 1);
  QPushButton* back = new QPushButton(tr("Put back"), box);
  back->setObjectName("scriptLabOutPutBack");
  back->setMinimumHeight(ControlHeight);
  back->setAccessibleName(tr("Put %1 back").arg(name));
  back->setToolTip(tr("Put \"%1\" back here. Closing its window does the same.").arg(name));
  row->addWidget(back);
  connect(back, &QPushButton::clicked, this, [this, key]() { putBack(key); });
  box->hide();
  return box;
}

LabPopWindow* LabSubTabs::makeWindow(const QString& key, const QString& name)
{
  LabPopWindow* window = new LabPopWindow(key, name, this);
  window->putBackButton()->setAccessibleName(tr("Put %1 back into Script Lab").arg(name));
  window->putBackButton()->setToolTip(tr("Put \"%1\" back into Script Lab. Closing this window does the same.").arg(name));
  window->putBackButton()->setStatusTip(window->putBackButton()->toolTip());
  window->applyStyle(m_dark);
  // where it was last time, else a size that fits a part, in the middle of the screen
  QRect place = m_places.value(key);
  if (!place.isValid())
  {
    QScreen* screen = this->screen() ? this->screen() : QGuiApplication::primaryScreen();
    const QRect area = screen ? screen->availableGeometry() : QRect(0, 0, 1024, 768);
    place = QRect(QPoint(), QSize(std::max(TextSize::scaled(480), area.width() * 2 / 5),
                                  std::max(TextSize::scaled(360), area.height() / 2)).boundedTo(area.size()));
    place.moveCenter(area.center());
  }
  window->setGeometry(LabPopWindow::fitOnScreen(place));
  m_places.insert(key, window->geometry());
  connect(window, &LabPopWindow::putBackRequested, this, [this](const QString& k) { putBack(k); });
  connect(window, &LabPopWindow::placeChanged, this, [this](const QString& k)
  {
    if (LabPopWindow* w = m_windows.value(k); w && w->isVisible())
    {
      m_places.insert(k, w->geometry());
      emit stateChanged();
    }
  });
  m_windows.insert(key, window);
  m_popOrder << key;
  return window;
}

bool LabSubTabs::popOut(const QString& key)
{
  if (key.isEmpty() || m_windows.contains(key)) return false;
  QWidget* moved = nullptr;
  QString name;
  if (m_groups.contains(key))
  {
    Group& g = m_groups[key];
    // a part of it that is out already comes back first: the group goes as one
    for (ScriptLabSection* s : g.parts)
      if (m_windows.contains(s->key())) putBack(s->key());
    g.page->layout()->removeWidget(g.content);
    // made only while it is out, so no hidden button waits in the keys' order
    g.placeholder = makePlaceholder(key, g.name, g.page);
    static_cast<QVBoxLayout*>(g.page->layout())->insertWidget(0, g.placeholder);
    g.placeholder->show();
    moved = g.content;
    name = g.name;
  }
  else if (m_parts.contains(key))
  {
    Part& p = m_parts[key];
    // a part of a group that is out is popped from that group's window: it stays with it
    if (m_windows.contains(p.group)) return false;
    int index = -1;
    QBoxLayout* box = findBox(p.section->parentWidget() ? p.section->parentWidget()->layout() : nullptr, p.section, &index);
    if (!box) return false;
    p.layout = box;
    p.index = index;
    box->removeWidget(p.section);
    p.placeholder = makePlaceholder(key, p.section->title(), p.section->parentWidget());
    box->insertWidget(index, p.placeholder);
    p.placeholder->show();
    p.pop->hide();   // in its own window the part has Put back on top
    // a folded part is opened: a window with only a header would show nothing
    p.section->setExpanded(true);
    moved = p.section;
    name = p.section->title();
  }
  else return false;

  // a part may be taller than a window: it scrolls there
  QScrollArea* scroll = new QScrollArea;
  scroll->setObjectName(QStringLiteral("scriptLabOutScroll_") + key);
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  QWidget* inner = new QWidget;
  QVBoxLayout* innerLayout = new QVBoxLayout(inner);
  innerLayout->setContentsMargins(SpacingM, SpacingS, SpacingM, SpacingM);
  innerLayout->setSpacing(SpacingS);
  innerLayout->addWidget(moved);
  innerLayout->addStretch(1);
  moved->show();
  scroll->setWidget(inner);
  LabPopWindow* window = makeWindow(key, name);
  window->setPage(scroll);
  if (m_windowsShown)
  {
    window->show();
    window->raise();
  }
  refreshList();
  refreshHeader();
  emit layoutChanged();
  emit stateChanged();
  return true;
}

void LabSubTabs::closeWindow(const QString& key)
{
  LabPopWindow* window = m_windows.take(key);
  m_popOrder.removeAll(key);
  if (!window) return;
  if (window->isVisible()) m_places.insert(key, window->geometry());
  QWidget* scroll = window->takePage();
  window->hide();
  window->deleteLater();
  if (scroll) scroll->deleteLater();
}

bool LabSubTabs::putBack(const QString& key)
{
  LabPopWindow* window = m_windows.value(key);
  if (!window) return false;
  QScrollArea* scroll = qobject_cast<QScrollArea*>(window->page());
  QWidget* inner = scroll ? scroll->widget() : nullptr;
  if (m_groups.contains(key))
  {
    Group& g = m_groups[key];
    if (inner) inner->layout()->removeWidget(g.content);
    g.content->setParent(g.page);
    if (g.placeholder)
    {
      g.page->layout()->removeWidget(g.placeholder);
      g.placeholder->hide();
      g.placeholder->deleteLater();
      g.placeholder = nullptr;
    }
    static_cast<QVBoxLayout*>(g.page->layout())->insertWidget(0, g.content);
    g.content->show();
  }
  else if (m_parts.contains(key))
  {
    Part& p = m_parts[key];
    if (inner) inner->layout()->removeWidget(p.section);
    QWidget* home = p.placeholder ? p.placeholder->parentWidget() : nullptr;
    p.section->setParent(home);
    if (p.layout && p.placeholder)
    {
      const int at = p.layout->indexOf(p.placeholder);
      p.layout->removeWidget(p.placeholder);
      p.layout->insertWidget(at >= 0 ? at : p.index, p.section);
    }
    if (p.placeholder) p.placeholder->deleteLater();
    p.placeholder = nullptr;
    p.section->show();
    p.pop->show();
  }
  closeWindow(key);
  refreshList();
  refreshHeader();
  emit layoutChanged();
  emit stateChanged();
  return true;
}

void LabSubTabs::reset()
{
  // the parts first (a part may be out on its own), then the groups
  for (const QString& key : QStringList(m_popOrder))
    if (m_parts.contains(key)) putBack(key);
  for (const QString& key : QStringList(m_popOrder)) putBack(key);
  m_places.clear();
  for (const QString& key : m_order)
    if (isListed(key)) { m_list->setCurrentRow(rowOf(key)); break; }
  m_autoPicked = true;   // the first listed one stays in front, also when the folder changes what is listed
  emit stateChanged();
}

void LabSubTabs::setPopWindowsShown(bool shown)
{
  m_windowsShown = shown;
  for (LabPopWindow* window : m_windows) window->setVisible(shown);
}

void LabSubTabs::refreshList()
{
  m_refreshing = true;
  const QString before = currentGroup();
  for (const QString& key : m_order)
  {
    const Group& g = m_groups[key];
    bool listed = g.parts.isEmpty();
    for (ScriptLabSection* s : g.parts) listed = listed || !s->isHidden();
    const int row = rowOf(key);
    if (row < 0) continue;
    QListWidgetItem* item = m_list->item(row);
    item->setHidden(!listed);
    // a group out in a window of its own says so in the list too (a word, not only a colour)
    item->setText(m_windows.contains(key) ? tr("%1 (window)").arg(g.name) : g.name);
  }
  // the one in front stays; else the first that is listed
  QString now = before;
  if (now.isEmpty() || !isListed(now) || m_autoPicked)
  {
    now.clear();
    for (const QString& key : m_order)
      if (isListed(key)) { now = key; break; }
  }
  if (!now.isEmpty())
  {
    m_list->setCurrentRow(rowOf(now));
    m_stack->setCurrentWidget(m_groups[now].page);
  }
  // as wide as the longest name and as tall as all rows: the list never scrolls sideways
  int width = 0;
  int height = 0;
  int shown = 0;
  for (int row = 0; row < m_list->count(); ++row)
  {
    if (m_list->item(row)->isHidden()) continue;
    width = std::max(width, m_list->sizeHintForColumn(0));
    height += m_list->sizeHintForRow(row);
    ++shown;
  }
  const int frame = 2 * m_list->frameWidth();
  m_list->setFixedWidth(std::max(width, m_list->fontMetrics().horizontalAdvance(QStringLiteral("M")) * 8) + frame + 2 * SpacingS);
  m_list->setMinimumHeight(height + frame);
  m_list->setMaximumHeight(height + frame + SpacingXs);
  // nothing to show (no folder watched yet): the whole sub-tab area steps aside for the empty state
  if (parentWidget()) setVisible(shown > 0);
  m_refreshing = false;
  if (now != before)
  {
    refreshHeader();
    emit stateChanged();
  }
}

void LabSubTabs::refreshHeader()
{
  const QString key = currentGroup();
  const QString name = groupName(key);
  m_title->setText(name);
  // a group of one part: the part's own header names it already
  m_title->setVisible(!key.isEmpty() && m_groups[key].parts.size() != 1);
  const bool out = m_windows.contains(key);
  m_popOut->setEnabled(!key.isEmpty() && !out);
  // greyed, it says why (charter P6)
  const QString tip = key.isEmpty() ? tr("Nothing to pop out here yet.")
                      : out ? tr("\"%1\" is in a window of its own already. Put back brings it here.").arg(name)
                            : tr("Pop out \"%1\" into a window of its own. Close that window, or press Put back, to bring it back.").arg(name);
  m_popOut->setToolTip(tip);
  m_popOut->setStatusTip(tip);
  m_popOut->setAccessibleName(tr("Pop out this part"));
  m_popOut->setAccessibleDescription(tip);
}

void LabSubTabs::stylePopButton(QToolButton* button)
{
  const int icon = TextSize::scaled(IconSmall);
  QPalette ink = palette();
  ink.setColor(QPalette::WindowText, Theme::color(Theme::Token::Text, m_dark));
  ink.setColor(QPalette::Disabled, QPalette::WindowText, Theme::color(Theme::Token::TextDisabled, m_dark));
  button->setIconSize(QSize(icon, icon));
  // the same symbol as Pop out beside the tabs and in the panel title bars (the same action, the same icon)
  button->setIcon(PanelTitleBar::drawSymbol(PanelTitleBar::Symbol::Float, ink, icon, devicePixelRatioF()));
  button->setStyleSheet(QStringLiteral("QToolButton { border: none; border-radius: %1px; padding: 0 %2px; color: %4; }"
                                       "QToolButton:hover { background: %3; }"
                                       "QToolButton:disabled { color: %5; }"
                                       "QToolButton:focus { border: 2px solid %6; }")
                          .arg(RadiusControl).arg(SpacingXs)
                          .arg(hex(Theme::Token::SurfaceRaised, m_dark), hex(Theme::Token::Text, m_dark),
                               hex(Theme::Token::TextMuted, m_dark), hex(Theme::Token::FocusRing, m_dark)));
}

void LabSubTabs::applyStyle(bool dark)
{
  m_dark = dark;
  // the list: the picked row has the selection fill, a bar on its left and bold text (not colour alone, P10)
  m_list->setStyleSheet(QStringLiteral(
    "QListWidget { background: transparent; border: 1px solid %1; border-radius: %2px; padding: %3px; color: %4; outline: none; }"
    "QListWidget:focus { border: 2px solid %5; }"
    "QListWidget::item { padding: %6px %7px; border-radius: %2px; border-left: 3px solid transparent; min-height: %8px; }"
    "QListWidget::item:hover { background: %9; }"
    "QListWidget::item:selected { background: %10; color: %11; border-left: 3px solid %12; font-weight: 600; }")
      .arg(hex(Theme::Token::Border, dark)).arg(RadiusControl).arg(SpacingXs).arg(hex(Theme::Token::Text, dark),
           hex(Theme::Token::FocusRing, dark)).arg(SpacingXs).arg(SpacingS).arg(TextSize::scaled(ControlCompact) - 2 * SpacingXs)
      .arg(hex(Theme::Token::SurfaceRaised, dark), hex(Theme::Token::Selection, dark), hex(Theme::Token::Text, dark),
           hex(Theme::Token::Accent, dark)));
  stylePopButton(m_popOut);
  for (const Part& p : m_parts) stylePopButton(p.pop);
  for (LabPopWindow* window : m_windows) window->applyStyle(dark);
  refreshList();
}

bool LabSubTabs::eventFilter(QObject* watched, QEvent* event)
{
  // a part shown or hidden by the panel (no folder watched, nothing to show): the list follows
  if (event->type() == QEvent::ShowToParent || event->type() == QEvent::HideToParent)
    if (!m_refreshing) refreshList();
  return QWidget::eventFilter(watched, event);
}

void LabSubTabs::changeEvent(QEvent* event)
{
  QWidget::changeEvent(event);
  if (event->type() == QEvent::PaletteChange) applyStyle(palette().color(QPalette::Window).lightness() < 128);
}
