/*
 * PinglyMotion Script Lab - the Script Lab panel: the channel map (job A4) and one window with its own tabs (job UX-B)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QGuiApplication>
#include <QScreen>
#include <QDockWidget>
#include <QFile>
#include <QFileInfo>
#include <QScrollArea>
#include <QMainWindow>
#include <QToolButton>
#include <QCheckBox>

#include "labtree.h"
#include "channelmapview.h"
#include "paneltitlebar.h"
#include "textsize.h"
#include "lucideicons.h"
#include "usersettings.h"
#include "scriptlabpanelui.h"

#include "scriptlabpanel.h"

using namespace LabUi;

// --- job A4: the channel map ------------------------------------------------------------------

void ScriptLabPanel::refreshChannelMap(const QJsonObject& folder)
{
  if (!m_mapSection) return;
  const bool shown = !folder.isEmpty();
  m_mapSection->setVisible(shown);
  m_mapEmpty->setVisible(!shown);   // job UX-B: the tab says what fills it
  // made again only when the run or its log changed (a click on a prop adds lines to the same run)
  const QString key = shown ? QStringLiteral("%1|%2|%3").arg(m_watcher ? m_watcher->folder() : QString())
                                .arg(folder.value("run").toInt())
                                .arg(folder.value("result").toObject().value("log").toArray().size())
                            : QString();
  if (shown && key == m_mapKey) return;
  m_mapKey = key;
  QPointer<LabWatcher> watcher = m_watcher;
  const LabChannelMap::Map map = shown ? LabChannelMap::fromFolderResult(folder, [watcher](const QString& file) -> QString
  {
    // only a script the watch knows: never a path out of the folder
    const QString path = watcher ? watcher->absolutePath(file) : QString();
    QFile in(path);
    if (path.isEmpty() || QFileInfo(path).size() > LabWatcher::MAX_FILE_BYTES || !in.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(in.read(LabWatcher::MAX_FILE_BYTES));
  }) : LabChannelMap::Map();
  m_mapView->setMap(map);
  const int messages = map.messages();
  const int unheard = map.unheard();
  QString count = messages == 0 ? tr("none") : messages == 1 ? tr("1 message") : tr("%1 messages").arg(messages);
  if (unheard > 0) count = tr("%1, %2").arg(count, unheard == 1 ? tr("1 problem") : tr("%1 problems").arg(unheard));
  m_mapSection->setCount(count);
  QStringList problems;
  if (unheard > 0)
    problems << (unheard == 1 ? tr("1 arrow reached nobody. It has a cross and says why.")
                              : tr("%1 arrows reached nobody. Each has a cross and says why.").arg(unheard));
  if (map.truncated) problems << tr("The map shows the first %1 arrows only.").arg(LabChannelMap::MAX_ARROWS);
  m_mapProblems->setText(problems.join(QLatin1Char(' ')));
  QPalette p = m_mapProblems->palette();
  p.setColor(QPalette::WindowText, unheard > 0 ? dangerColour(isDark(palette())) : mutedColour(isDark(palette())));
  m_mapProblems->setPalette(p);
  m_mapProblems->setVisible(!problems.isEmpty());
}

bool ScriptLabPanel::activateMapBox(int box)
{
  const LabChannelMap::Map& map = m_mapView->map();
  if (box < 0 || box >= map.boxes.size() || map.boxes[box].kind != LabChannelMap::BoxKind::Part) return false;
  const QString name = map.boxes[box].name;
  if (!m_folderTree->selectPrim(name)) return false;
  // as a click on the row: the prop lights up when selection follows (job U3)
  if (m_followBox->isChecked() && m_hooks.selectProp) m_hooks.selectProp(name);
  return true;
}

bool ScriptLabPanel::activateMapArrow(int arrow)
{
  const LabChannelMap::Map& map = m_mapView->map();
  if (arrow < 0 || arrow >= map.arrows.size()) return false;
  const LabChannelMap::Arrow& a = map.arrows[arrow];
  if (a.file.isEmpty() || a.line <= 0) return false;
  if (a.file != m_shownFile && !showScript(a.file)) return false;
  // job UX-B: the editor is on Live scripts; that tab comes to the front unless it is in sight
  // already (popped out beside the map)
  if (!isPoppedOut(LivePage)) showPage(LivePage);
  showScriptPlace(a.line, a.column);
  return true;
}

// --- job UX-B: one window with its own tabs ----------------------------------------------------

QWidget* ScriptLabPanel::makePageScroll(QWidget* content, const char* name)
{
  QScrollArea* scroll = new QScrollArea(m_tabs);
  scroll->setObjectName(QString::fromLatin1(name));
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll->setWidget(content);
  return scroll;
}

QString ScriptLabPanel::pageKey(Page page)
{
  switch (page)
  {
    case LivePage: return QStringLiteral("live");
    case CommandsPage: return QStringLiteral("commands");
    case MapPage: return QStringLiteral("map");
    case PlayerPage: return QStringLiteral("player");
    default: return QString();
  }
}

QString ScriptLabPanel::pageName(Page page) const
{
  switch (page)
  {
    case LivePage: return tr("Live scripts");
    case CommandsPage: return tr("Type commands");
    case MapPage: return tr("Who talks to whom");
    case PlayerPage: return tr("Player");
    default: return QString();
  }
}

void ScriptLabPanel::addPageTab(Page page)
{
  // in the order of the pages: after every page before it that is in the window
  int at = 0;
  for (int p = 0; p < page; ++p)
    if (tabIndex(Page(p)) >= 0) ++at;
  const int index = m_tabs->insertTab(at, m_pages[page], pageName(page));
  // the tips: Live and Type commands as before, the map and player as in the mockup (LABTWO-08, -09)
  static const char* const tips[PageCount] = {
    QT_TR_NOOP("Live: your .lsl scripts run each time you save them"), QT_TR_NOOP("Commands: type RLV lines by hand"),
    QT_TR_NOOP("Arrows between the parts: who sends what to whom."), QT_TR_NOOP("Step back and forth through your recorded steps.")};
  m_tabs->setTabToolTip(index, tr(tips[page]));
  m_tabs->setTabWhatsThis(index, tr(tips[page]));
  stylePageTabs();
}

void ScriptLabPanel::stylePageTabs()
{
  if (!m_tabs) return;
  const bool dark = isDark(palette());
  const Tokens& t = tokens(dark);
  // Lucide icons, in the text colour: as the mockup draws the tabs (file-code, terminal, network, rotate-ccw)
  static const char* const icons[PageCount] = {"file-code", "terminal", "network", "rotate-ccw"};
  for (int p = 0; p < PageCount; ++p)
  {
    const int index = tabIndex(Page(p));
    if (index < 0) continue;
    m_tabs->setTabIcon(index, LucideIcons::icon(QString::fromLatin1(icons[p]), QColor(QLatin1String(t.text)), QColor(QLatin1String(t.textDisabled))));
  }
  if (m_popOutButton)
  {
    const int icon = TextSize::scaled(16);
    QPalette ink = palette();
    ink.setColor(QPalette::WindowText, QColor(QLatin1String(t.text)));
    ink.setColor(QPalette::Disabled, QPalette::WindowText, QColor(QLatin1String(t.textDisabled)));
    m_popOutButton->setIconSize(QSize(icon, icon));
    m_popOutButton->setIcon(PanelTitleBar::drawSymbol(PanelTitleBar::Symbol::Float, ink, icon, devicePixelRatioF()));
    m_popOutButton->setStyleSheet(QStringLiteral("QToolButton { border: none; border-radius: %1px; padding: 0 %2px; color: %4; }"
                                                 "QToolButton:hover { background: %3; }"
                                                 "QToolButton:disabled { color: %5; }"
                                                 "QToolButton:focus { border: 2px solid %6; }")
                                  .arg(RadiusControl).arg(SpacingS)
                                  .arg(QLatin1String(t.surfaceRaised), QLatin1String(t.text), QLatin1String(t.textMuted), QLatin1String(t.focusRing)));
  }
  for (LabPopWindow* window : m_popWindows)
    if (window) window->applyStyle(dark);
}

int ScriptLabPanel::tabIndex(Page page) const
{
  if (!m_tabs || page < 0 || page >= PageCount || !m_pages[page]) return -1;
  return m_tabs->indexOf(m_pages[page]);
}

ScriptLabPanel::Page ScriptLabPanel::currentPage() const
{
  QWidget* current = m_tabs ? m_tabs->currentWidget() : nullptr;
  for (int p = 0; p < PageCount; ++p)
    if (current && m_pages[p] == current) return Page(p);
  return LivePage;
}

LabPopWindow* ScriptLabPanel::popWindow(Page page) const
{
  return page >= 0 && page < PageCount ? m_popWindows[page] : nullptr;
}

void ScriptLabPanel::showPage(Page page)
{
  if (page < 0 || page >= PageCount) return;
  if (LabPopWindow* window = m_popWindows[page])
  {
    if (m_popWindowsShown) window->show();
    window->raise();
    window->activateWindow();
    return;
  }
  m_tabs->setCurrentIndex(tabIndex(page));
}

bool ScriptLabPanel::popOut(Page page, const QPoint& at)
{
  if (page < 0 || page >= PageCount || m_popWindows[page] || m_tabs->count() <= 1) return false;
  const QString key = pageKey(page);
  QWidget* content = m_pages[page];
  m_tabs->removeTab(m_tabs->indexOf(content));
  LabPopWindow* window = new LabPopWindow(key, pageName(page), this);
  m_popWindows[page] = window;
  window->setMinimumSize(minimumPanelSize());
  window->setPage(content);
  // where it was last time, else big beside the main window (or where a dragged tab was let go)
  QRect place = m_popPlaces.value(key);
  if (!place.isValid())
  {
    QScreen* screen = at.isNull() ? this->screen() : QGuiApplication::screenAt(at);
    if (!screen) screen = QGuiApplication::primaryScreen();
    const QRect area = screen ? screen->availableGeometry() : QRect(0, 0, 1024, 768);
    place = QRect(QPoint(), LabPopWindow::firstSize(area).expandedTo(minimumPanelSize()));
    if (at.isNull()) place.moveCenter(area.center());
    else place.moveTopLeft(at - QPoint(TextSize::scaled(40), TextSize::scaled(16)));
  }
  window->setGeometry(LabPopWindow::fitOnScreen(place));
  m_popPlaces.insert(key, window->geometry());
  connect(window, &LabPopWindow::putBackRequested, this, [this, page]() { putBack(page); });
  connect(window, &LabPopWindow::placeChanged, this, [this, page]()
  {
    if (LabPopWindow* w = m_popWindows[page]; w && w->isVisible()) { m_popPlaces.insert(pageKey(page), w->geometry()); saveWindowState(); }
  });
  if (m_popWindowsShown)
  {
    window->show();
    window->raise();
    if (m_windowStateReady) window->activateWindow();
  }
  stylePageTabs();
  saveWindowState();
  refreshPopOutButton();
  updateWorkLayout();
  refreshTips();
  return true;
}

bool ScriptLabPanel::putBack(Page page)
{
  if (page < 0 || page >= PageCount || !m_popWindows[page]) return false;
  LabPopWindow* window = m_popWindows[page];
  m_popWindows[page] = nullptr;
  if (window->isVisible()) m_popPlaces.insert(pageKey(page), window->geometry());
  window->takePage();
  window->hide();
  window->deleteLater();
  addPageTab(page);
  m_tabs->setCurrentIndex(tabIndex(page));
  saveWindowState();
  refreshPopOutButton();
  updateWorkLayout();
  refreshTips();
  return true;
}

void ScriptLabPanel::refreshPopOutButton()
{
  if (!m_popOutButton) return;
  const bool can = m_tabs->count() > 1;
  m_popOutButton->setEnabled(can);
  // greyed, it says why (charter P6)
  const QString tip = can ? tr("Pop out \"%1\" into a window of its own. Close that window, or press Put back, to bring it back.")
                                .arg(pageName(currentPage()))
                          : tr("The last tab stays in Script Lab. Put another tab back first.");
  m_popOutButton->setToolTip(tip);
  m_popOutButton->setStatusTip(tip);
  m_popOutButton->setAccessibleDescription(tip);
}

void ScriptLabPanel::setPopWindowsShown(bool shown)
{
  m_popWindowsShown = shown;
  for (LabPopWindow* window : m_popWindows)
    if (window) window->setVisible(shown);
}

QString ScriptLabPanel::windowState() const
{
  LabTabState::State state;
  state.current = pageKey(currentPage());
  for (int p = 0; p < PageCount; ++p)
    if (m_popWindows[p]) state.popped << pageKey(Page(p));
  state.places = m_popPlaces;
  return LabTabState::format(state);
}

void ScriptLabPanel::applyWindowState(const QString& text)
{
  QStringList known;
  for (int p = 0; p < PageCount; ++p) known << pageKey(Page(p));
  const LabTabState::State state = LabTabState::parse(text, known);
  const bool ready = m_windowStateReady;
  m_windowStateReady = false;   // nothing is saved half way
  for (int p = 0; p < PageCount; ++p)
    if (m_popWindows[p] && !state.popped.contains(pageKey(Page(p)))) putBack(Page(p));
  m_popPlaces = state.places;
  for (const QString& key : state.popped)
  {
    const int p = int(known.indexOf(key));
    if (p >= 0 && !m_popWindows[p]) popOut(Page(p));
  }
  const int current = int(known.indexOf(state.current));
  if (current >= 0 && tabIndex(Page(current)) >= 0) m_tabs->setCurrentIndex(tabIndex(Page(current)));
  m_windowStateReady = ready;
  saveWindowState();
  refreshPopOutButton();
}

void ScriptLabPanel::saveWindowState()
{
  if (m_windowStateReady) UserSettings::setScriptLabWindows(windowState());
}

void ScriptLabPanel::resetWindows()
{
  for (int p = 0; p < PageCount; ++p) putBack(Page(p));
  m_popPlaces.clear();
  m_tabs->setCurrentIndex(tabIndex(LivePage));
  UserSettings::setScriptLabSized(0);
  saveWindowState();
}

int ScriptLabPanel::bigDockedWidth(const QSize& window)
{
  // most of the main window (the owner: big by default, so everything can be seen), never less
  // than the panel's own first size and never more than leaves the 3D view a strip
  const int wanted = std::max(defaultSize().width(), window.width() * 3 / 5);
  return std::max(minimumPanelSize().width(), std::min(wanted, window.width() - TextSize::scaled(240)));
}

QSize ScriptLabPanel::bigFloatingSize(const QRect& screen)
{
  const QSize wanted(screen.width() * 4 / 5, screen.height() * 4 / 5);
  return wanted.expandedTo(minimumPanelSize()).boundedTo(screen.size());
}

bool ScriptLabPanel::s_layoutRestoring = false;

bool ScriptLabPanel::openBig(QMainWindow* window, QDockWidget* dock)
{
  if (!window || !dock || dock->isHidden()) return false;
  const int sized = UserSettings::scriptLabSized();
  if (dock->isFloating())
  {
    if (sized & 2) return false;
    UserSettings::setScriptLabSized(sized | 2);
    QScreen* screen = window->screen() ? window->screen() : QGuiApplication::primaryScreen();
    if (!screen) return false;
    const QRect area = screen->availableGeometry();
    QRect place(QPoint(), bigFloatingSize(area));
    place.moveCenter(area.center());
    dock->setGeometry(place);
    return true;
  }
  if (sized & 1) return false;
  UserSettings::setScriptLabSized(sized | 1);
  const Qt::DockWidgetArea area = window->dockWidgetArea(dock);
  if (area == Qt::TopDockWidgetArea || area == Qt::BottomDockWidgetArea)
    window->resizeDocks({dock}, {std::max(minimumPanelSize().height(), window->height() * 3 / 5)}, Qt::Vertical);
  else
  {
    window->resizeDocks({dock}, {bigDockedWidth(window->size())}, Qt::Horizontal);
    // sharing its side with another panel (under Selected joint at the first start): most of the height too
    window->resizeDocks({dock}, {window->height()}, Qt::Vertical);
  }
  return true;
}
