/*
 * PinglyMotion Script Lab - the player (job B3, see labplayer.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QCheckBox>
#include <QEvent>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSet>
#include <QVBoxLayout>

#include <cmath>

#include "labplayer.h"
#include "lucideicons.h"
#include "playback.h"
#include "scriptlabpanel.h"
#include "theme.h"
#include "transportbar.h"

namespace
{
  // seconds as the player writes them: "12 s", "12.5 s"
  QString seconds(double t)
  {
    return QObject::tr("%1 s").arg(QString::number(std::round(t * 1000.0) / 1000.0, 'g', 10));
  }

  // the subject of a fact sentence: "Send instant messages" of "Send instant messages is blocked."
  QString subjectOf(const QString& sentence)
  {
    const int is = sentence.indexOf(QStringLiteral(" is "));
    return is > 0 ? sentence.left(is) : sentence;
  }

  int group(LabPlayerModel::Look look)
  {
    switch (look)
    {
      case LabPlayerModel::Look::Locked:
      case LabPlayerModel::Look::Free: return 0;
      case LabPlayerModel::Look::Blocked:
      case LabPlayerModel::Look::Allowed: return 1;
      case LabPlayerModel::Look::Problem: return 2;
      case LabPlayerModel::Look::Plain: break;
    }
    return 3;
  }

  // the rows of one moment, without "changed"
  QVector<LabPlayerModel::Row> plainRows(const QVector<LabPlayerModel::Moment>& moments, int index)
  {
    using namespace LabPlayerModel;
    QVector<Row> rows;
    if (index < 0 || index >= moments.size()) return rows;
    const Moment& m = moments.at(index);
    QSet<QString> here;
    for (const Fact& f : m.facts)
    {
      here.insert(f.key);
      if (f.key.startsWith(QLatin1String("lock:")))
        rows.append({QObject::tr("Locked: %1").arg(f.key.mid(5)), Look::Locked, false});
      else if (f.key.startsWith(QLatin1String("blocked:")))
        rows.append({QObject::tr("Blocked: %1").arg(subjectOf(f.text)), Look::Blocked, false});
      else if (f.key.startsWith(QLatin1String("step:")) || f.key == QLatin1String("stopped") || f.key == QLatin1String("run"))
        rows.append({f.text, Look::Problem, false});
      else
        rows.append({f.text, Look::Plain, false});
    }
    // locked or blocked at another moment of the replay, not now: free and allowed, in green
    QSet<QString> seen;
    for (const Moment& other : moments)
      for (const Fact& f : other.facts)
      {
        if (here.contains(f.key) || seen.contains(f.key)) continue;
        if (f.key.startsWith(QLatin1String("lock:")))
          rows.append({QObject::tr("Free: %1").arg(f.key.mid(5)), Look::Free, false});
        else if (f.key.startsWith(QLatin1String("blocked:")))
          rows.append({QObject::tr("Allowed: %1").arg(subjectOf(f.text)), Look::Allowed, false});
        else continue;
        seen.insert(f.key);
      }
    std::stable_sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return group(a.look) < group(b.look); });
    return rows;
  }
}

QVector<LabPlayerModel::Moment> LabPlayerModel::moments(const QJsonObject& replay)
{
  QVector<Moment> out;
  for (const QJsonValue& v : replay.value("frames").toArray())
  {
    const QJsonObject f = v.toObject();
    Moment m;
    m.step = f.value("step").toInt();
    m.t = f.value("t").toDouble();
    m.logEnd = f.value("log_end").toInt();
    m.done = f.value("done").toBool(true);
    m.text = f.value("text").toString();
    for (const QJsonValue& fact : f.value("facts").toArray())
      m.facts.append({fact.toObject().value("key").toString(), fact.toObject().value("text").toString()});
    out.append(m);
  }
  return out;
}

int LabPlayerModel::firstDifference(const QJsonObject& replay)
{
  const QJsonValue v = replay.value("first_difference");
  if (!v.isDouble()) return -1;
  const int n = v.toInt(-1);
  return n >= 0 && n < replay.value("frames").toArray().size() ? n : -1;
}

QJsonObject LabPlayerModel::folderAt(const QJsonObject& folder, const QJsonArray& log, int logEnd)
{
  if (!folder.value("ok").toBool()) return QJsonObject();
  QJsonArray cut;
  const int end = qBound(0, logEnd, int(log.size()));
  for (int i = 0; i < end; ++i) cut.append(log.at(i));
  return QJsonObject{{"ok", true}, {"linkset", folder.value("linkset")}, {"result", QJsonObject{{"log", cut}}}};
}

QString LabPlayerModel::momentTitle(const Moment& moment)
{
  QString line = moment.step == 0 ? QObject::tr("Start") : QObject::tr("Step %1: %2").arg(moment.step).arg(moment.text);
  if (!moment.done) line += QObject::tr(" Not done.");
  return line;
}

QString LabPlayerModel::positionLine(const QVector<Moment>& moments, int index)
{
  if (moments.isEmpty()) return QObject::tr("No replay yet. Press Record what I do, click your props, then stop recording.");
  if (index < 0 || index >= moments.size()) return QString();
  const Moment& m = moments.at(index);
  if (m.step == 0) return QObject::tr("Start, at %1.").arg(seconds(m.t));
  QString line = QObject::tr("Step %1 of %2, at %3: %4").arg(m.step).arg(moments.size() - 1).arg(seconds(m.t), m.text);
  if (!m.done) line += QObject::tr(" It could not be done.");
  return line;
}

QVector<LabPlayerModel::Row> LabPlayerModel::rows(const QVector<Moment>& moments, int index)
{
  QVector<Row> rows = plainRows(moments, index);
  if (rows.isEmpty())
  {
    if (index >= 0 && index < moments.size()) rows.append({QObject::tr("Nothing locked, coloured or said yet."), Look::Plain, false});
    return rows;
  }
  // bold where it changed at this step (the start has nothing before it)
  if (index > 0)
  {
    const QVector<Row> before = plainRows(moments, index - 1);
    for (Row& r : rows) r.changed = !before.contains(r);
  }
  return rows;
}

QStringList LabPlayerModel::logLines(const QJsonArray& log, int logEnd)
{
  QStringList out;
  const int end = qBound(0, logEnd, int(log.size()));
  for (int i = 0; i < end; ++i)
  {
    const QJsonObject e = log.at(i).toObject();
    if (e.value("type").toString() == QLatin1String("step") && e.value("source").toString() == QLatin1String("recipe"))
      out << QObject::tr("--- Step %1: %2 ---").arg(e.value("step").toInt()).arg(e.value("text").toString());
    else
      out << ScriptLabPanel::logLine(e);
  }
  return out;
}

// --- the widget ------------------------------------------------------------------------------------

LabPlayer::LabPlayer(QWidget* parent) :
  QWidget(parent)
{
  setObjectName("scriptLabPlayer");
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(Theme::Size::spacingXs);

  m_title = new QLabel(tr("Player: watch your steps again"), this);
  m_title->setObjectName("scriptLabPlayerTitle");
  m_title->setTextFormat(Qt::PlainText);
  m_title->setWordWrap(true);
  layout->addWidget(m_title);

  m_position = new QLabel(this);
  m_position->setObjectName("scriptLabPlayerPosition");
  m_position->setTextFormat(Qt::PlainText);
  m_position->setWordWrap(true);
  m_position->setTextInteractionFlags(Qt::TextSelectableByMouse);
  layout->addWidget(m_position);

  // the animation timeline's own play buttons, with words for steps instead of frames
  m_bar = new TransportBar(this);
  m_bar->setObjectName("scriptLabPlayerBar");
  m_bar->backButton()->setText(tr("Step back"));
  m_bar->forwardButton()->setText(tr("Step forward"));
  m_bar->loopCheckBox()->hide();   // a replay plays once (charter P5)
  layout->addWidget(m_bar);
  connect(m_bar, &TransportBar::startClicked, this, &LabPlayer::play);
  connect(m_bar, &TransportBar::stopClicked, this, &LabPlayer::stop);
  connect(m_bar, &TransportBar::replayClicked, this, &LabPlayer::replayFromStart);
  connect(m_bar, &TransportBar::backClicked, this, [this]() { stepBy(-1); });
  connect(m_bar, &TransportBar::forwardClicked, this, [this]() { stepBy(1); });

  m_list = new QListWidget(this);
  m_list->setObjectName("scriptLabPlayerMoments");
  m_list->setAccessibleName(tr("Moments of the replay"));
  m_list->setToolTip(tr("Pick a moment to see what was true then. The arrow keys move one step."));
  m_list->setSelectionMode(QAbstractItemView::SingleSelection);
  layout->addWidget(m_list);
  connect(m_list, &QListWidget::currentRowChanged, this, [this](int row)
  {
    if (m_styling || row < 0 || row == m_current) return;
    stop();
    setCurrent(row, true);
  });

  QHBoxLayout* buttons = new QHBoxLayout;
  buttons->setSpacing(Theme::Size::spacingS);
  m_firstDifference = new QPushButton(tr("Go to first difference"), this);
  m_firstDifference->setObjectName("scriptLabPlayerFirstDifference");
  m_firstDifference->setMinimumHeight(Theme::Size::controlCompact);
  m_firstDifference->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  buttons->addWidget(m_firstDifference);
  m_now = new QPushButton(tr("Show now"), this);
  m_now->setObjectName("scriptLabPlayerNow");
  m_now->setMinimumHeight(Theme::Size::controlCompact);
  m_now->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  buttons->addWidget(m_now);
  buttons->addStretch(1);
  layout->addLayout(buttons);
  connect(m_firstDifference, &QPushButton::clicked, this, [this]() { goToFirstDifference(); });
  connect(m_now, &QPushButton::clicked, this, &LabPlayer::showNow);

  m_view = new QLabel(this);
  m_view->setObjectName("scriptLabPlayerView");
  m_view->setTextFormat(Qt::PlainText);
  m_view->setWordWrap(true);
  layout->addWidget(m_view);

  m_stateTitle = new QLabel(tr("True at this moment:"), this);
  m_stateTitle->setObjectName("scriptLabPlayerStateTitle");
  m_stateTitle->setTextFormat(Qt::PlainText);
  layout->addWidget(m_stateTitle);
  m_state = new QListWidget(this);
  m_state->setObjectName("scriptLabPlayerState");
  m_state->setAccessibleName(tr("True at this moment"));
  m_state->setSelectionMode(QAbstractItemView::NoSelection);
  m_state->setFocusPolicy(Qt::TabFocus);
  layout->addWidget(m_state);

  // Expert: the event log up to the moment
  m_logTitle = new QLabel(tr("Log up to this moment:"), this);
  m_logTitle->setObjectName("scriptLabPlayerLogTitle");
  m_logTitle->setTextFormat(Qt::PlainText);
  layout->addWidget(m_logTitle);
  m_log = new QPlainTextEdit(this);
  m_log->setObjectName("scriptLabPlayerLog");
  m_log->setReadOnly(true);
  m_log->setLineWrapMode(QPlainTextEdit::NoWrap);
  m_log->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  m_log->setAccessibleName(tr("Log up to this moment"));
  layout->addWidget(m_log);

  const int line = fontMetrics().height();
  m_list->setMinimumHeight(4 * line);
  m_list->setMaximumHeight(7 * line + 2 * Theme::Size::spacingS);
  m_state->setMinimumHeight(3 * line);
  m_state->setMaximumHeight(8 * line + 2 * Theme::Size::spacingS);
  m_log->setMinimumHeight(5 * line);

  m_timer.setInterval(PLAY_INTERVAL_MS);
  connect(&m_timer, &QTimer::timeout, this, &LabPlayer::tick);

  applyStyle();
  refresh();
}

void LabPlayer::setReplay(const QJsonObject& replay)
{
  if (replay == m_replay) return;
  m_replay = replay;
  m_timer.stop();
  m_moments = LabPlayerModel::moments(replay);
  m_logEntries = replay.value("log").toArray();
  m_firstDiff = LabPlayerModel::firstDifference(replay);
  m_current = m_moments.isEmpty() ? 0 : int(m_moments.size()) - 1;
  m_showing = false;
  refresh();
  emit momentChanged();
}

void LabPlayer::setExpertMode(bool expert)
{
  m_expert = expert;
  refresh();
}

void LabPlayer::setCurrent(int index, bool show)
{
  if (m_moments.isEmpty()) return;
  const int next = qBound(0, index, int(m_moments.size()) - 1);
  const bool changed = next != m_current || show != m_showing;
  m_current = next;
  m_showing = show;
  refresh();
  if (changed) emit momentChanged();
}

void LabPlayer::showMoment(int index)
{
  setCurrent(index, true);
}

void LabPlayer::stepBy(int delta)
{
  if (m_moments.isEmpty()) return;
  stop();
  setCurrent(Playback::stepFrame(m_current, int(m_moments.size()), delta), true);
}

void LabPlayer::play()
{
  if (m_moments.isEmpty() || playing()) return;
  // on the last moment, Start begins at the start again (as the timeline does)
  setCurrent(Playback::startFrame(m_current, int(m_moments.size()), -1), true);
  if (Playback::atEnd(m_current, int(m_moments.size()))) { refresh(); return; }   // only one moment
  m_timer.start();
  refresh();
}

void LabPlayer::stop()
{
  if (!m_timer.isActive()) return;
  m_timer.stop();
  refresh();
}

void LabPlayer::replayFromStart()
{
  if (m_moments.isEmpty()) return;
  m_timer.stop();
  setCurrent(Playback::replayFrame(), true);
  play();
}

bool LabPlayer::goToFirstDifference()
{
  if (m_firstDiff < 0) return false;
  stop();
  setCurrent(m_firstDiff, true);
  return true;
}

void LabPlayer::showNow()
{
  if (!m_showing) return;
  stop();
  setCurrent(m_current, false);
}

QJsonObject LabPlayer::folderAt(const QJsonObject& folder) const
{
  if (m_moments.isEmpty()) return folder;
  return LabPlayerModel::folderAt(folder, m_logEntries, m_moments.at(m_current).logEnd);
}

void LabPlayer::tick()
{
  if (m_moments.isEmpty() || Playback::atEnd(m_current, int(m_moments.size())))
  {
    stop();
    return;
  }
  setCurrent(m_current + 1, true);
  // played once to the last moment: stop there (the replay plays once)
  if (Playback::atEnd(m_current, int(m_moments.size()))) stop();
}

void LabPlayer::refresh()
{
  const bool any = !m_moments.isEmpty();
  m_position->setText(LabPlayerModel::positionLine(m_moments, m_current));

  // the list of moments
  m_styling = true;
  if (m_list->count() != m_moments.size())
  {
    m_list->clear();
    for (int i = 0; i < m_moments.size(); ++i) m_list->addItem(QString());
  }
  const bool dark = palette().color(QPalette::Window).lightness() < 128;
  for (int i = 0; i < m_moments.size(); ++i)
  {
    QListWidgetItem* item = m_list->item(i);
    QString text = LabPlayerModel::momentTitle(m_moments.at(i));
    if (i == m_firstDiff) text += tr(" First difference.");
    item->setText(text);
    const bool problem = !m_moments.at(i).done;
    const QColor colour = Theme::color(problem ? Theme::Token::Warning : Theme::Token::Text, dark);
    item->setForeground(colour);
    item->setIcon(problem ? LucideIcons::icon(QStringLiteral("triangle-alert"), colour, Theme::color(Theme::Token::TextDisabled, dark))
                          : i == m_firstDiff ? LucideIcons::icon(QStringLiteral("crosshair"), colour, Theme::color(Theme::Token::TextDisabled, dark))
                                             : QIcon());
  }
  if (any && m_list->currentRow() != m_current) m_list->setCurrentRow(m_current);
  m_styling = false;
  m_list->setEnabled(any);

  refreshButtons();
  refreshState();

  m_view->setText(!any ? QString()
                       : m_showing ? tr("The props in the 3D view show this moment. Press Show now to see now again.")
                                   : tr("The props in the 3D view show now. Pick a moment to see it there."));
  m_logTitle->setVisible(m_expert && any);
  m_log->setVisible(m_expert && any);
  if (m_expert && any)
  {
    const QString text = LabPlayerModel::logLines(m_logEntries, m_moments.at(m_current).logEnd).join(QLatin1Char('\n'));
    if (m_log->toPlainText() != text)
    {
      m_log->setPlainText(text);
      m_log->verticalScrollBar()->setValue(m_log->verticalScrollBar()->maximum());
    }
  }
}

void LabPlayer::refreshButtons()
{
  const bool any = !m_moments.isEmpty();
  const int total = any ? int(m_moments.size()) : 1;
  m_bar->setPlaystate(playing() ? PLAYSTATE_PLAYING : PLAYSTATE_STOPPED);
  m_bar->setFrame(m_current, total);
  auto tip = [](QPushButton* button, const QString& text)
  {
    button->setToolTip(text);
    button->setStatusTip(text);
    button->setAccessibleDescription(text);
  };
  // the bar's words are about frames of an animation; here they are about steps (charter P6, P13)
  QPushButton* start = m_bar->startButton();
  QPushButton* stopButton = m_bar->stopButton();
  QPushButton* replay = m_bar->replayButton();
  QPushButton* back = m_bar->backButton();
  QPushButton* forward = m_bar->forwardButton();
  if (!any)
  {
    for (QPushButton* b : {start, stopButton, replay, back, forward}) b->setEnabled(false);
    for (QPushButton* b : {start, stopButton, replay, back, forward}) tip(b, tr("No replay yet. Record what you do first."));
  }
  else
  {
    tip(start, start->isEnabled() ? tr("Play the steps one by one, one each second") : tr("It is already playing. Click Stop to stop it."));
    tip(stopButton, stopButton->isEnabled() ? tr("Stop playing; the player stays on this step") : tr("It is not playing. Click Start to play it."));
    tip(replay, tr("Go back to the start and play the steps"));
    tip(back, back->isEnabled() ? tr("Go to the step before") : tr("This is the start. There is no step before it."));
    tip(forward, forward->isEnabled() ? tr("Go to the next step") : tr("This is the last step. Click Replay from start to see it again."));
  }

  m_firstDifference->setEnabled(m_firstDiff >= 0);
  if (m_firstDiff >= 0)
    tip(m_firstDifference, m_firstDiff == 0 ? tr("Go to the start: it already differed from the replay before")
                                            : tr("Go to step %1, where this replay first differed").arg(m_firstDiff));
  else if (!any)
    tip(m_firstDifference, tr("No replay yet. Record what you do first."));
  else if (m_replay.value("first").toBool())
    tip(m_firstDifference, tr("This is the first replay, so there is nothing to compare with."));
  else
    tip(m_firstDifference, tr("Every step gave the same as the replay before."));

  m_now->setEnabled(m_showing);
  tip(m_now, m_showing ? tr("Show the props as they are now, not at this moment") : tr("The 3D view already shows now."));
}

void LabPlayer::refreshState()
{
  m_state->clear();
  const bool any = !m_moments.isEmpty();
  m_stateTitle->setVisible(any);
  m_state->setVisible(any);
  if (!any) return;
  const bool dark = palette().color(QPalette::Window).lightness() < 128;
  const QColor disabled = Theme::color(Theme::Token::TextDisabled, dark);
  for (const LabPlayerModel::Row& r : LabPlayerModel::rows(m_moments, m_current))
  {
    Theme::Token token = Theme::Token::Text;
    QString icon = QStringLiteral("info");
    switch (r.look)
    {
      case LabPlayerModel::Look::Locked: token = Theme::Token::Danger; icon = QStringLiteral("circle-x"); break;
      case LabPlayerModel::Look::Blocked: token = Theme::Token::Danger; icon = QStringLiteral("ban"); break;
      case LabPlayerModel::Look::Free:
      case LabPlayerModel::Look::Allowed: token = Theme::Token::Success; icon = QStringLiteral("circle-check"); break;
      case LabPlayerModel::Look::Problem: token = Theme::Token::Warning; icon = QStringLiteral("triangle-alert"); break;
      case LabPlayerModel::Look::Plain: break;
    }
    const QColor colour = Theme::color(token, dark);
    // bold and the words "changed at this step": not by weight alone (charter P10)
    QListWidgetItem* item = new QListWidgetItem(r.changed ? tr("%1 (changed at this step)").arg(r.text) : r.text, m_state);
    item->setForeground(colour);
    item->setIcon(LucideIcons::icon(icon, colour, disabled));
    QFont f = item->font();
    f.setBold(r.changed);
    item->setFont(f);
  }
}

void LabPlayer::applyStyle()
{
  QFont f = m_title->font();
  f.setWeight(QFont::Weight(Theme::Size::labelWeight));
  m_title->setFont(f);
  QPalette muted = m_view->palette();
  muted.setColor(QPalette::WindowText, Theme::color(Theme::Token::TextMuted, palette().color(QPalette::Window).lightness() < 128));
  m_view->setPalette(muted);
}

void LabPlayer::changeEvent(QEvent* event)
{
  QWidget::changeEvent(event);
  if (event->type() == QEvent::PaletteChange && !m_styling)
  {
    // a new theme: the colours of the rows and icons are made again
    m_styling = true;
    applyStyle();
    m_styling = false;
    refresh();
  }
}

void LabPlayer::resizeEvent(QResizeEvent* event)
{
  QWidget::resizeEvent(event);
  m_bar->setWrapped(event->size().width() < m_bar->oneLineWidth());
}
