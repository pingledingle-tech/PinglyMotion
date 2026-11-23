/*
 * PinglyMotion Script Lab - the watched folder as a tree (lane 1, job D)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QEvent>
#include <QHash>
#include <QHeaderView>
#include <QJsonArray>
#include <QPainter>
#include <QPixmap>

#include "lucideicons.h"
#include "theme.h"

#include "labtree.h"

namespace
{
  // design_tokens.json: spacing.s; a dot of 8 px as the mockup's joint list (.jlist .dot)
  const int SpacingS = 8;
  const int DotSize = 8;
  const int IconSize = 16;

  QIcon dotIcon(const QColor& colour)
  {
    // the dot sits in a 16 px square, so the words after it line up with the other rows
    QPixmap pixmap(IconSize, IconSize);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(colour);
    const qreal offset = (IconSize - DotSize) / 2.0;
    painter.drawEllipse(QRectF(offset, offset, DotSize, DotSize));
    return QIcon(pixmap);
  }

  // as ScriptLabPanel::isDark: the window colour decides
  bool paletteIsDark(const QPalette& palette) { return palette.color(QPalette::Window).lightness() < 128; }

  QString plural(int n, const QString& one, const QString& many)
  {
    return n == 1 ? one : many.arg(n);
  }
}

LabFolderTree::LabFolderTree(QWidget* parent) : QTreeWidget(parent)
{
  setObjectName("scriptLabFolderTree");
  setHeaderLabels({tr("Name"), tr("Kind"), tr("State"), tr("Memory")});
  setUniformRowHeights(true);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setIconSize(QSize(IconSize, IconSize));
  setIndentation(IconSize + SpacingS / 2);
  header()->setStretchLastSection(false);
  header()->setSectionResizeMode(NameColumn, QHeaderView::Stretch);
  header()->setSectionResizeMode(KindColumn, QHeaderView::ResizeToContents);
  header()->setSectionResizeMode(StateColumn, QHeaderView::ResizeToContents);
  header()->setSectionResizeMode(MemoryColumn, QHeaderView::ResizeToContents);
  setToolTip(tr("The folder as one object: each subfolder is a prim. Click a script to show it."));
  setMinimumHeight(5 * fontMetrics().height() + 2 * SpacingS);
  m_dark = paletteIsDark(palette());

  // one click or Enter on a script shows it
  const auto picked = [this](QTreeWidgetItem* item)
  {
    if (m_building || !item || item->data(NameColumn, KindRole).toInt() != ScriptRow) return;
    emit scriptPicked(item->data(NameColumn, FileRole).toString());
  };
  connect(this, &QTreeWidget::itemClicked, this, [picked](QTreeWidgetItem* item, int) { picked(item); });
  connect(this, &QTreeWidget::itemActivated, this, [picked](QTreeWidgetItem* item, int) { picked(item); });
}

QString LabFolderTree::stateWord(State state)
{
  switch (state)
  {
    case Ok: return tr("OK");
    case NotRun: return tr("Not run");
    case Warning: return tr("Warning");
    case Stopped: return tr("Stopped");
    case Error: return tr("Error");
    case NoScripts: break;
  }
  return tr("No scripts");
}

QString LabFolderTree::memoryText(qint64 bytes)
{
  return tr("%1 KB").arg(QString::number(double(qMax<qint64>(0, bytes)) / 1024.0, 'f', 1));
}

QString LabFolderTree::sumText(const QList<State>& states)
{
  int count[Error + 1] = {0, 0, 0, 0, 0, 0};
  for (State s : states) ++count[s];
  QStringList parts;
  if (count[Error]) parts << plural(count[Error], tr("1 error"), tr("%1 errors"));
  if (count[Stopped]) parts << plural(count[Stopped], tr("1 stopped"), tr("%1 stopped"));
  if (count[Warning]) parts << plural(count[Warning], tr("1 warning"), tr("%1 warnings"));
  if (count[NotRun]) parts << plural(count[NotRun], tr("1 not run"), tr("%1 not run"));
  if (count[Ok]) parts << tr("%1 OK").arg(count[Ok]);
  return parts.isEmpty() ? stateWord(NoScripts) : parts.join(QStringLiteral(", "));
}

QColor LabFolderTree::stateColour(State state, bool dark)
{
  switch (state)
  {
    case Ok: return Theme::color(Theme::Token::Success, dark);
    case Warning: return Theme::color(Theme::Token::Warning, dark);
    case Stopped:
    case Error: return Theme::color(Theme::Token::Danger, dark);
    case NotRun:
    case NoScripts: break;
  }
  return Theme::color(Theme::Token::TextMuted, dark);
}

void LabFolderTree::setFolderResult(const QJsonObject& folder)
{
  const QString picked = pickedScript();
  m_building = true;
  clear();
  m_count.clear();

  const QJsonObject linkset = folder.value("linkset").toObject();
  const QJsonArray prims = linkset.value("prims").toArray();
  if (prims.isEmpty())
  {
    m_building = false;
    return;
  }

  // each script's check, the folder's limits and rules per file, and what the run says per script
  QHash<QString, QJsonObject> checks;
  for (const QJsonValue& v : folder.value("check").toObject().value("scripts").toArray())
    checks.insert(v.toObject().value("file").toString(), v.toObject());
  QHash<QString, QString> problems;
  for (const QJsonValue& v : linkset.value("problems").toArray())
  {
    const QJsonObject p = v.toObject();
    if (!p.value("file").toString().isEmpty() && !problems.contains(p.value("file").toString()))
      problems.insert(p.value("file").toString(), p.value("message").toString());
  }
  const bool ran = folder.value("ok").toBool() && folder.contains("result");
  QHash<QString, QJsonObject> runs;
  const QJsonArray objects = folder.value("result").toObject().value("objects").toArray();
  if (!objects.isEmpty())
    for (const QJsonValue& v : objects.first().toObject().value("scripts").toArray())
      runs.insert(v.toObject().value("name").toString(), v.toObject());

  QTreeWidgetItem* object = new QTreeWidgetItem(this);
  object->setText(NameColumn, linkset.value("object").toString());
  object->setText(KindColumn, plural(int(prims.size()), tr("Object, 1 prim"), tr("Object, %1 prims")));
  object->setData(NameColumn, KindRole, ObjectRow);
  object->setToolTip(NameColumn, tr("The top folder is one object, named after the folder"));

  QList<State> allStates;
  qint64 allMemory = 0;
  bool anyMemory = false;
  int scriptCount = 0;
  for (const QJsonValue& pv : prims)
  {
    const QJsonObject p = pv.toObject();
    const int link = p.value("link").toInt();
    QTreeWidgetItem* prim = new QTreeWidgetItem(object);
    prim->setText(NameColumn, p.value("name").toString());
    prim->setText(KindColumn, link == 1 ? tr("Root prim, link 1") : tr("Prim, link %1").arg(link));
    prim->setData(NameColumn, KindRole, PrimRow);
    prim->setToolTip(NameColumn, p.value("folder").toString().isEmpty() ? tr("The files right in the top folder")
                                                                         : tr("The folder %1").arg(p.value("folder").toString()));
    QList<State> primStates;
    qint64 primMemory = 0;
    bool primHasMemory = false;
    for (const QJsonValue& sv : p.value("scripts").toArray())
    {
      const QJsonObject s = sv.toObject();
      const QString file = s.value("file").toString();
      const QString name = s.value("name").toString();
      const QJsonObject check = checks.value(file);
      const QJsonObject run = runs.value(name);
      State state = Ok;
      QString why;
      if (problems.contains(file)) { state = Error; why = problems.value(file); }
      else if (!check.isEmpty() && !check.value("valid").toBool())
      {
        state = Error;
        const QJsonArray errors = check.value("errors").toArray();
        why = plural(int(errors.size()), tr("1 error. Click to show it in the script."), tr("%1 errors. Click to show them in the script."));
      }
      else if (!ran) { state = NotRun; why = tr("Not run: %1").arg(folder.value("error").toString()); }
      else if (run.value("stopped").toBool()) { state = Stopped; why = tr("Stopped: %1").arg(run.value("stop_reason").toString()); }
      else if (!check.value("warnings").toArray().isEmpty())
      {
        state = Warning;
        why = plural(int(check.value("warnings").toArray().size()), tr("1 warning. Click to see it."), tr("%1 warnings. Click to see them."));
      }
      else why = tr("Ran without errors.");

      QTreeWidgetItem* item = new QTreeWidgetItem(prim);
      item->setText(NameColumn, name);
      item->setText(KindColumn, tr("LSL script"));
      item->setText(StateColumn, stateWord(state));
      item->setData(NameColumn, KindRole, ScriptRow);
      item->setData(NameColumn, FileRole, file);
      item->setData(NameColumn, StateRole, int(state));
      item->setToolTip(NameColumn, file);
      item->setToolTip(StateColumn, why);
      if (ran && run.contains("memory"))
      {
        const qint64 memory = qint64(run.value("memory").toDouble());
        item->setText(MemoryColumn, memoryText(memory));
        item->setData(NameColumn, MemoryRole, memory);
        item->setToolTip(MemoryColumn, tr("Memory the script used at the end of the run. A script may use up to 64 KB."));
        primMemory += memory;
        primHasMemory = true;
      }
      primStates << state;
      ++scriptCount;
    }
    for (const QJsonValue& nv : p.value("notecards").toArray())
    {
      const QJsonObject n = nv.toObject();
      QTreeWidgetItem* item = new QTreeWidgetItem(prim);
      item->setText(NameColumn, n.value("name").toString());
      const int lines = n.value("lines").toInt();
      item->setText(KindColumn, plural(lines, tr("Notecard, 1 line"), tr("Notecard, %1 lines")));
      item->setData(NameColumn, KindRole, NotecardRow);
      item->setData(NameColumn, FileRole, n.value("file").toString());
      item->setToolTip(NameColumn, n.value("file").toString());
    }
    State worst = NoScripts;
    for (State s : primStates) worst = qMax(worst, s);
    prim->setText(StateColumn, sumText(primStates));
    prim->setData(NameColumn, StateRole, int(worst));
    if (primHasMemory)
    {
      prim->setText(MemoryColumn, memoryText(primMemory));
      prim->setData(NameColumn, MemoryRole, primMemory);
    }
    allStates << primStates;
    allMemory += primMemory;
    anyMemory = anyMemory || primHasMemory;
  }
  State worst = NoScripts;
  for (State s : allStates) worst = qMax(worst, s);
  object->setText(StateColumn, sumText(allStates));
  object->setData(NameColumn, StateRole, int(worst));
  if (anyMemory)
  {
    object->setText(MemoryColumn, memoryText(allMemory));
    object->setData(NameColumn, MemoryRole, allMemory);
  }
  m_count = plural(scriptCount, tr("1 script"), tr("%1 scripts")) + QLatin1Char(' ')
            + plural(int(prims.size()), tr("in 1 prim"), tr("in %1 prims"));
  expandAll();
  applyStyle(m_dark);
  if (QTreeWidgetItem* item = itemForFile(picked))
    if (item->data(NameColumn, KindRole).toInt() == ScriptRow) setCurrentItem(item);
  m_building = false;
}

QTreeWidgetItem* LabFolderTree::itemForFile(const QString& file) const
{
  if (file.isEmpty()) return nullptr;
  for (QTreeWidgetItemIterator it(const_cast<LabFolderTree*>(this)); *it; ++it)
    if ((*it)->data(NameColumn, FileRole).toString() == file) return *it;
  return nullptr;
}

LabFolderTree::State LabFolderTree::scriptState(const QString& file) const
{
  QTreeWidgetItem* item = itemForFile(file);
  if (!item || item->data(NameColumn, KindRole).toInt() != ScriptRow) return NoScripts;
  return State(item->data(NameColumn, StateRole).toInt());
}

bool LabFolderTree::pickScript(const QString& file)
{
  QTreeWidgetItem* item = itemForFile(file);
  if (!item || item->data(NameColumn, KindRole).toInt() != ScriptRow) return false;
  setCurrentItem(item);
  emit scriptPicked(file);
  return true;
}

QString LabFolderTree::pickedScript() const
{
  const QTreeWidgetItem* item = currentItem();
  if (!item || item->data(NameColumn, KindRole).toInt() != ScriptRow) return QString();
  return item->data(NameColumn, FileRole).toString();
}

void LabFolderTree::restyleItem(QTreeWidgetItem* item, bool dark)
{
  const QColor text = Theme::color(Theme::Token::Text, dark);
  const QColor disabled = Theme::color(Theme::Token::TextDisabled, dark);
  static const char* const icons[] = {"box", "square", "file-code", "notebook-text"};
  const int kind = item->data(NameColumn, KindRole).toInt();
  item->setIcon(NameColumn, LucideIcons::icon(QLatin1String(icons[qBound(0, kind, 3)]), text, disabled));
  if (kind == NotecardRow) return;
  const State state = State(item->data(NameColumn, StateRole).toInt());
  item->setIcon(StateColumn, dotIcon(stateColour(state, dark)));
  // the words stay in the text colour (the dot carries the colour), muted for "No scripts"
  item->setForeground(StateColumn, state == NoScripts ? Theme::color(Theme::Token::TextMuted, dark) : text);
}

void LabFolderTree::applyStyle(bool dark)
{
  m_dark = dark;
  for (QTreeWidgetItemIterator it(this); *it; ++it) restyleItem(*it, dark);
}

void LabFolderTree::changeEvent(QEvent* event)
{
  QTreeWidget::changeEvent(event);
  if (event->type() == QEvent::PaletteChange) applyStyle(paletteIsDark(palette()));
}
