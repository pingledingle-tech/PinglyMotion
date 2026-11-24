/*
 * PinglyMotion - helper mode (see helpermode.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "helpermode.h"

#include <QAction>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLayout>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QStyle>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

#include "helperspec.h"
#include "hintcard.h"
#include "lucideicons.h"
#include "theme.h"
#include "usersettings.h"

namespace
{
  QString tr2(const char* text) { return HelperSpec::tr(text); }

  int ringPad() { return Theme::Size::focusRingOffset + Theme::Size::focusRingWidth; }
}

// ---------------------------------------------------------------------------- the ring

HelperHighlight::HelperHighlight(QWidget* parent) : QWidget(parent)
{
  setObjectName(QStringLiteral("helperHighlight"));
  setAttribute(Qt::WA_TransparentForMouseEvents, true);
  setAttribute(Qt::WA_NoSystemBackground, true);
  setFocusPolicy(Qt::NoFocus);
  hide();
}

void HelperHighlight::setTarget(QWidget* target)
{
  if (target == m_target)
  {
    follow();
    return;
  }
  watch(false);
  m_target = target;
  watch(true);
  follow();
}

bool HelperHighlight::shown() const
{
  return m_target && isVisible();
}

QRect HelperHighlight::ringFor(QWidget* target) const
{
  if (!target) return QRect();
  QWidget* window = target->window();
  const QPoint topLeft = target == window ? QPoint(0, 0) : target->mapTo(window, QPoint(0, 0));
  const int pad = ringPad();
  return QRect(topLeft, target->size()).adjusted(-pad, -pad, pad, pad) & window->rect();
}

void HelperHighlight::follow()
{
  if (!m_target || !m_target->isVisible())
  {
    hide();
    return;
  }
  QWidget* window = m_target->window();
  if (parentWidget() != window) setParent(window);   // a control in a floating window gets its ring there
  const QRect ring = ringFor(m_target);
  if (geometry() != ring) setGeometry(ring);
  if (!isVisible()) show();
  raise();
}

// the control and every widget above it, up to its window: a panel that moves moves the control
void HelperHighlight::watch(bool on)
{
  for (const QPointer<QWidget>& widget : std::as_const(m_watched))
    if (widget) widget->removeEventFilter(this);
  m_watched.clear();
  if (!on || !m_target) return;
  for (QWidget* widget = m_target; widget; widget = widget->parentWidget())
  {
    widget->installEventFilter(this);
    m_watched.append(widget);
    if (widget->isWindow()) break;
  }
}

bool HelperHighlight::eventFilter(QObject* watched, QEvent* event)
{
  switch (event->type())
  {
    case QEvent::ParentChange:
      watch(true);
      follow();
      break;
    case QEvent::Move:
    case QEvent::Resize:
    case QEvent::Show:
    case QEvent::Hide:
      follow();
      break;
    default:
      break;
  }
  return QWidget::eventFilter(watched, event);
}

void HelperHighlight::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  QPen pen(Theme::color(Theme::Token::FocusRing));
  const int width = Theme::Size::focusRingWidth;
  pen.setWidth(width);
  painter.setPen(pen);
  painter.setBrush(Qt::NoBrush);
  const qreal half = width / 2.0;
  painter.drawRoundedRect(QRectF(rect()).adjusted(half, half, -half, -half), Theme::Size::radiusControl,
                          Theme::Size::radiusControl);
}

// ---------------------------------------------------------------------------- the panel

HelperPanel::HelperPanel(QWidget* parent) : QFrame(parent)
{
  setObjectName(QStringLiteral("helperPanel"));
  setAttribute(Qt::WA_StyledBackground, true);
  setFocusPolicy(Qt::StrongFocus);
  setAccessibleName(tr2(HelperSpec::panelTitle));

  QVBoxLayout* column = new QVBoxLayout(this);
  const int m = Theme::Size::spacingM;
  column->setContentsMargins(m, m, m, m);
  column->setSpacing(Theme::Size::spacingS);

  QHBoxLayout* head = new QHBoxLayout();
  head->setSpacing(Theme::Size::spacingS);
  m_icon = new QLabel(this);
  m_icon->setObjectName(QStringLiteral("helperIcon"));
  head->addWidget(m_icon);
  m_title = new QLabel(tr2(HelperSpec::panelTitle), this);
  m_title->setObjectName(QStringLiteral("helperTitle"));
  QFont titleFont = m_title->font();
  titleFont.setWeight(QFont::DemiBold);
  m_title->setFont(titleFont);
  head->addWidget(m_title, 1);
  m_close = new QToolButton(this);
  m_close->setObjectName(QStringLiteral("helperClose"));
  m_close->setToolTip(tr2(HelperSpec::closeName));
  m_close->setAccessibleName(tr2(HelperSpec::closeName));
  m_close->setAutoRaise(true);
  m_close->setFocusPolicy(Qt::StrongFocus);
  m_close->setFixedSize(Theme::Size::controlCompact, Theme::Size::controlCompact);
  head->addWidget(m_close, 0, Qt::AlignTop);
  column->addLayout(head);

  m_pages = new QStackedWidget();
  m_pages->setObjectName(QStringLiteral("helperPages"));

  // the home page: the hint card now due, then the lessons
  m_home = new QWidget();
  m_home->setObjectName(QStringLiteral("helperHome"));
  QVBoxLayout* home = new QVBoxLayout(m_home);
  home->setContentsMargins(0, 0, 0, 0);
  home->setSpacing(Theme::Size::spacingS);
  // the hint card's place has no layout: HelperMode gives it the card's own size, because a layout
  // would offer the fixed-width card more width and so too little height
  m_cardHolder = new QWidget(m_home);
  m_cardHolder->setObjectName(QStringLiteral("helperCardHolder"));
  m_cardHolder->setFixedHeight(0);
  home->addWidget(m_cardHolder);
  m_lessonsHeading = new QLabel(tr2(HelperSpec::lessonsHeading), m_home);
  m_lessonsHeading->setObjectName(QStringLiteral("helperLessonsHeading"));
  QFont headingFont = m_lessonsHeading->font();
  headingFont.setWeight(QFont::DemiBold);
  m_lessonsHeading->setFont(headingFont);
  home->addWidget(m_lessonsHeading);
  for (int i = 0; i < HelperSpec::lessonCount; ++i)
  {
    const HelperSpec::Lesson& spec = HelperSpec::lessons[i];
    QToolButton* button = new QToolButton(m_home);
    button->setObjectName(QStringLiteral("helperLesson_") + QLatin1String(spec.id));
    button->setText(tr2(spec.title));
    button->setToolTip(tr2(spec.summary));
    button->setStatusTip(tr2(spec.summary));
    button->setAccessibleName(tr2(spec.title));
    button->setAccessibleDescription(tr2(spec.summary));
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setFocusPolicy(Qt::StrongFocus);
    connect(button, &QToolButton::clicked, this, [this, i]() { emit lessonRequested(i); });
    home->addWidget(button);
    m_lessonButtons.append(button);
  }
  home->addStretch(1);
  m_pages->addWidget(m_home);

  // the page of the lesson that runs: its title, the step, and Stop or Back to lessons
  m_lesson = new QWidget();
  m_lesson->setObjectName(QStringLiteral("helperLesson"));
  QVBoxLayout* lesson = new QVBoxLayout(m_lesson);
  lesson->setContentsMargins(0, 0, 0, 0);
  lesson->setSpacing(Theme::Size::spacingXs);
  m_lessonTitle = new QLabel(m_lesson);
  m_lessonTitle->setObjectName(QStringLiteral("helperLessonTitle"));
  m_lessonTitle->setWordWrap(true);
  m_lessonTitle->setFont(headingFont);
  lesson->addWidget(m_lessonTitle);
  m_progress = new QLabel(m_lesson);
  m_progress->setObjectName(QStringLiteral("helperProgress"));
  lesson->addWidget(m_progress);
  m_step = new QLabel(m_lesson);
  m_step->setObjectName(QStringLiteral("helperStep"));
  m_step->setWordWrap(true);
  lesson->addWidget(m_step);
  m_notOnScreen = new QLabel(tr2(HelperSpec::notOnScreen), m_lesson);
  m_notOnScreen->setObjectName(QStringLiteral("helperNotOnScreen"));
  m_notOnScreen->setWordWrap(true);
  m_notOnScreen->hide();
  lesson->addWidget(m_notOnScreen);
  lesson->addSpacing(Theme::Size::spacingS);
  QHBoxLayout* buttons = new QHBoxLayout();
  buttons->setSpacing(Theme::Size::spacingS);
  m_stop = new QToolButton(m_lesson);
  m_stop->setObjectName(QStringLiteral("helperStop"));
  m_stop->setText(tr2(HelperSpec::stopLesson));
  m_stop->setToolTip(tr2(HelperSpec::stopName));
  m_stop->setAccessibleName(tr2(HelperSpec::stopName));
  m_stop->setToolButtonStyle(Qt::ToolButtonTextOnly);
  m_stop->setFocusPolicy(Qt::StrongFocus);
  buttons->addWidget(m_stop);
  buttons->addStretch(1);
  m_back = new QToolButton(m_lesson);
  m_back->setObjectName(QStringLiteral("helperBack"));
  m_back->setText(tr2(HelperSpec::backToLessons));
  m_back->setToolTip(tr2(HelperSpec::backToLessons));
  m_back->setAccessibleName(tr2(HelperSpec::backToLessons));
  m_back->setToolButtonStyle(Qt::ToolButtonTextOnly);
  m_back->setFocusPolicy(Qt::StrongFocus);
  m_back->hide();
  buttons->addWidget(m_back);
  lesson->addLayout(buttons);
  lesson->addStretch(1);
  m_pages->addWidget(m_lesson);

  m_scroll = new QScrollArea(this);
  m_scroll->setObjectName(QStringLiteral("helperScroll"));
  m_scroll->setFrameShape(QFrame::NoFrame);
  m_scroll->setWidgetResizable(true);
  m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_scroll->setWidget(m_pages);
  m_scroll->viewport()->setAutoFillBackground(false);
  m_pages->setAutoFillBackground(false);
  column->addWidget(m_scroll, 1);

  // only the page on show asks for room
  connect(m_pages, &QStackedWidget::currentChanged, this, [this](int)
  {
    for (int i = 0; i < m_pages->count(); ++i)
      m_pages->widget(i)->setSizePolicy(QSizePolicy::Preferred,
                                        m_pages->widget(i) == m_pages->currentWidget() ? QSizePolicy::Preferred : QSizePolicy::Ignored);
  });
  m_lesson->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);

  connect(m_close, &QToolButton::clicked, this, &HelperPanel::closeRequested);
  connect(m_stop, &QToolButton::clicked, this, &HelperPanel::stopRequested);
  connect(m_back, &QToolButton::clicked, this, &HelperPanel::backRequested);
  applyLook();
}

int HelperPanel::panelWidth() const
{
  // as wide as the hint card (300 at 100 percent text, more when its footer needs it)
  int content = 300 * UserSettings::textSizePercent() / 100;
  for (QWidget* child : m_cardHolder->findChildren<QWidget*>(Qt::FindDirectChildrenOnly))
    if (child->minimumWidth() == child->maximumWidth()) content = std::max(content, child->minimumWidth());
  const int bar = style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, m_scroll);
  return content + bar + 2 * Theme::Size::spacingM + 2 * Theme::Size::borderWidth;
}

int HelperPanel::wantedHeight() const
{
  const int inner = panelWidth() - 2 * Theme::Size::spacingM - 2 * Theme::Size::borderWidth
                    - style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, m_scroll);
  // summed here from the parts, not asked from the page: the page's layout keeps an old answer
  // until its events have run, and a new hint card must get its room at once
  QWidget* page = m_pages->currentWidget();
  QLayout* column = page->layout();
  int pageHeight = column->contentsMargins().top() + column->contentsMargins().bottom();
  int shown = 0;
  for (int i = 0; i < column->count(); ++i)
  {
    QLayoutItem* item = column->itemAt(i);
    // a fixed space counts with its own height and no spacing (as the box layout does)
    if (item->spacerItem())
    {
      pageHeight += item->sizeHint().height();
      continue;
    }
    if (item->isEmpty()) continue;
    int h = 0;
    if (QWidget* widget = item->widget())
    {
      if (widget == m_cardHolder)
      {
        for (QWidget* card : m_cardHolder->findChildren<QWidget*>(Qt::FindDirectChildrenOnly))
          if (!card->isHidden()) h += card->sizeHint().height();
      }
      else
        h = widget->hasHeightForWidth() ? widget->heightForWidth(inner) : widget->sizeHint().height();
    }
    else
      h = item->sizeHint().height();
    if (shown++ > 0) pageHeight += column->spacing();
    pageHeight += h;
  }
  const int head = std::max({m_title->sizeHint().height(), m_close->height(), m_icon->sizeHint().height()});
  return 2 * Theme::Size::spacingM + 2 * Theme::Size::borderWidth + head + Theme::Size::spacingS + pageHeight;
}

void HelperPanel::applyLook()
{
  using Theme::Token;
  const QString text = Theme::color(Token::Text).name();
  const QString muted = Theme::color(Token::TextMuted).name();
  const QString control = QString::number(Theme::Size::radiusControl);
  const QString edge = Theme::color(Token::Border).name();
  const QString focus = QStringLiteral("border: %1px solid %2;").arg(Theme::Size::focusRingWidth)
                          .arg(Theme::color(Token::FocusRing).name());
  setStyleSheet(QStringLiteral(
    "QFrame#helperPanel { background: %1; border: %2px solid %3; border-radius: %4px; }"
    "QFrame#helperPanel:focus { %5 }"
    "QScrollArea#helperScroll, QStackedWidget#helperPages, QWidget#helperHome, QWidget#helperLesson,"
    " QWidget#helperCardHolder { background: transparent; border: 0; }")
    .arg(Theme::color(Token::Surface).name()).arg(Theme::Size::borderWidth).arg(edge).arg(Theme::Size::radiusPanel)
    .arg(focus));
  for (QLabel* label : {m_title, m_lessonTitle, m_step})
    label->setStyleSheet(QStringLiteral("color: %1; background: transparent; border: 0;").arg(text));
  for (QLabel* label : {m_lessonsHeading, m_progress, m_notOnScreen})
    label->setStyleSheet(QStringLiteral("color: %1; background: transparent; border: 0;").arg(muted));
  m_icon->setStyleSheet(QStringLiteral("background: transparent; border: 0;"));
  m_icon->setPixmap(LucideIcons::icon(QStringLiteral("lightbulb"), Theme::color(Token::Warning),
                                      Theme::color(Token::TextDisabled)).pixmap(Theme::Size::iconSmall, Theme::Size::iconSmall));
  m_close->setIcon(LucideIcons::icon(QStringLiteral("x"), Theme::color(Token::Text), Theme::color(Token::TextDisabled)));
  m_close->setStyleSheet(QStringLiteral("QToolButton { background: transparent; border: 0; } QToolButton:focus { %1 border-radius: %2px; }")
                           .arg(focus, control));
  const QString normal = QStringLiteral(
    "QToolButton { background: %1; color: %2; border: %5px solid %3; border-radius: %4px; padding: 4px 8px; text-align: left; }"
    "QToolButton:disabled { color: %6; }"
    "QToolButton:focus { %7 }")
    .arg(Theme::color(Token::SurfaceRaised).name(), text, edge, control).arg(Theme::Size::borderWidth)
    .arg(Theme::color(Token::TextDisabled).name(), focus);
  for (QToolButton* button : std::as_const(m_lessonButtons)) button->setStyleSheet(normal);
  m_stop->setStyleSheet(normal);
  m_back->setStyleSheet(QStringLiteral(
    "QToolButton { background: %1; color: %2; border: %4px solid %1; border-radius: %3px; padding: 4px 12px; }"
    "QToolButton:focus { %5 }")
    .arg(Theme::color(Token::Accent).name(), Theme::color(Token::OnAccent).name(), control).arg(Theme::Size::borderWidth)
    .arg(focus));
}

// Esc in the panel stops the lesson that runs (or goes back to the lessons when it is done)
void HelperPanel::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_Escape && m_pages->currentWidget() == m_lesson)
  {
    if (m_back->isVisible()) emit backRequested();
    else emit stopRequested();
    return;
  }
  QFrame::keyPressEvent(event);
}

// ---------------------------------------------------------------------------- what shows

HelperMode::HelperMode(QWidget* host, HintCards* cards, TargetFor targetFor, AlreadyDone alreadyDone, Area area) :
  QObject(host), m_host(host), m_cards(cards), m_targetFor(std::move(targetFor)), m_alreadyDone(std::move(alreadyDone)),
  m_area(std::move(area)), m_panel(new HelperPanel(host)), m_highlight(new HelperHighlight(host)), m_switch(new QAction(this))
{
  m_panel->hide();
  m_switch->setObjectName(QLatin1String(HelperSpec::switchId));
  m_switch->setText(tr2(HelperSpec::switchText));
  m_switch->setCheckable(true);
  connect(m_switch, &QAction::triggered, this, &HelperMode::switchClicked);

  // the hint cards move into the panel
  if (m_cards)
  {
    m_cards->setHolder(m_panel->cardHolder());
    connect(m_cards, &HintCards::changed, this, &HelperMode::refresh);
  }

  connect(m_panel, &HelperPanel::closeRequested, this, &HelperMode::offRequested);
  connect(m_panel, &HelperPanel::lessonRequested, this, &HelperMode::startLesson);
  connect(m_panel, &HelperPanel::stopRequested, this, &HelperMode::stopLesson);
  connect(m_panel, &HelperPanel::backRequested, this, &HelperMode::stopLesson);
  host->installEventFilter(this);
  // a page whose content changed (a longer hint card, a step text) asks for room again
  for (QWidget* page : {m_panel->homePage(), m_panel->lessonPage()}) page->installEventFilter(this);
  syncSwitch();
}

void HelperMode::addToBar(QToolBar* bar, QAction* before)
{
  bar->insertAction(before, m_switch);
  if (QToolButton* tool = qobject_cast<QToolButton*>(bar->widgetForAction(m_switch)))
  {
    tool->setObjectName(m_switch->objectName() + QStringLiteral("Button"));
    tool->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    tool->setFocusPolicy(Qt::StrongFocus);
  }
  syncSwitch();
}

void HelperMode::setOn(bool on)
{
  if (!on)
  {
    m_lesson = -1;
    m_step = 0;
    m_done = false;
  }
  const bool changed = on != m_on;
  m_on = on;
  refresh();
  if (changed && !on) emit lessonChanged();
}

bool HelperMode::panelShown() const
{
  return m_panel->isVisibleTo(m_host);
}

void HelperMode::setAnimationOpen(bool open)
{
  m_open = open;
  if (!open && m_lesson >= 0 && HelperSpec::lessons[m_lesson].needsAnimation)
  {
    stopLesson();
    return;
  }
  refresh();
}

QString HelperMode::blockedReason(int index) const
{
  if (index < 0 || index >= HelperSpec::lessonCount) return QString();
  if (HelperSpec::lessons[index].needsAnimation && !m_open) return tr2(HelperSpec::needsAnimation);
  return QString();
}

void HelperMode::startLesson(int index)
{
  if (!m_on || index < 0 || index >= HelperSpec::lessonCount || !blockedReason(index).isEmpty()) return;
  m_lesson = index;
  m_step = 0;
  m_done = false;
  skipDone();
  refresh();
  emit lessonChanged();
  // the keys stay in the panel, so Esc stops the lesson
  m_panel->setFocus(Qt::OtherFocusReason);
}

void HelperMode::stopLesson()
{
  if (m_lesson < 0) return;
  m_lesson = -1;
  m_step = 0;
  m_done = false;
  refresh();
  emit lessonChanged();
}

// a step whose thing is already so is skipped (a joint is already picked, the Library is open)
void HelperMode::skipDone()
{
  if (m_lesson < 0) return;
  const HelperSpec::Lesson& lesson = HelperSpec::lessons[m_lesson];
  while (!m_done && m_alreadyDone && m_alreadyDone(QLatin1String(lesson.steps[m_step].event)))
  {
    if (++m_step >= lesson.stepCount)
    {
      m_step = lesson.stepCount - 1;
      m_done = true;
    }
  }
}

void HelperMode::notify(const QString& event)
{
  if (!m_on || m_lesson < 0 || m_done) return;
  const HelperSpec::Lesson& lesson = HelperSpec::lessons[m_lesson];
  if (event != QLatin1String(lesson.steps[m_step].event)) return;
  if (m_step + 1 >= lesson.stepCount) m_done = true;
  else
  {
    ++m_step;
    skipDone();
  }
  refresh();
  emit lessonChanged();
}

void HelperMode::refresh()
{
  if (m_refreshing) return;
  m_refreshing = true;
  syncSwitch();
  syncCardHolder();
  syncPanel();
  place();
  QWidget* target = nullptr;
  if (m_on)
  {
    if (m_lesson >= 0)
    {
      if (!m_done && m_targetFor) target = m_targetFor(QLatin1String(HelperSpec::lessons[m_lesson].steps[m_step].target));
    }
    else if (m_cards)
      target = m_cards->currentTarget();
  }
  m_highlight->setTarget(target);
  m_refreshing = false;
}

void HelperMode::applyLook()
{
  m_panel->applyLook();
  if (m_cards) m_cards->card()->applyLook();
  syncSwitch();
  m_highlight->update();
}

// the switch: pressed and a tick when on, the lightbulb when off, and a tip that says which
// (never by colour alone, charter P10)
void HelperMode::syncSwitch()
{
  if (m_switch->isChecked() != m_on) m_switch->setChecked(m_on);
  const QString key = QKeySequence(QLatin1String(HelperSpec::switchKey)).toString(QKeySequence::NativeText);
  const QString tip = tr2(m_on ? HelperSpec::switchTipOn : HelperSpec::switchTipOff);
  m_switch->setToolTip(tip + QStringLiteral("  (") + key + QLatin1Char(')'));
  m_switch->setStatusTip(tip);
  m_switch->setIcon(LucideIcons::icon(m_on ? QStringLiteral("check") : QStringLiteral("lightbulb"),
                                      Theme::color(Theme::Token::Text), Theme::color(Theme::Token::TextDisabled)));
  for (QObject* object : m_switch->associatedObjects())
    if (QToolButton* tool = qobject_cast<QToolButton*>(object))
    {
      QFont font = tool->font();
      if (font.bold() != m_on)
      {
        font.setBold(m_on);
        tool->setFont(font);
      }
      tool->setAccessibleName(m_switch->text());
      tool->setAccessibleDescription(tip);
    }
}

// the hint card's place is exactly as big as the card, and nothing when no card shows
void HelperMode::syncCardHolder()
{
  if (!m_cards) return;
  QWidget* holder = m_panel->cardHolder();
  HintCard* card = m_cards->card();
  const int h = card->isHidden() ? 0 : card->sizeHint().height();
  if (holder->minimumWidth() != card->width()) holder->setMinimumWidth(card->width());
  if (holder->height() != h || holder->minimumHeight() != h) holder->setFixedHeight(h);
  const QRect place(0, 0, card->width(), h);
  if (!card->isHidden() && card->geometry() != place) card->setGeometry(place);
}

void HelperMode::syncPanel()
{
  if (!m_on)
  {
    m_panel->hide();
    return;
  }
  if (m_lesson < 0)
  {
    m_panel->pages()->setCurrentWidget(m_panel->homePage());
    for (int i = 0; i < HelperSpec::lessonCount; ++i)
    {
      QToolButton* button = m_panel->lessonButtons().at(i);
      const QString why = blockedReason(i);
      const QString summary = tr2(HelperSpec::lessons[i].summary);
      // a lesson that cannot start now is greyed with the reason (charter P6)
      button->setEnabled(why.isEmpty());
      const QString tip = why.isEmpty() ? summary : summary + QLatin1Char(' ') + why;
      if (button->toolTip() != tip) button->setToolTip(tip);
      const QString status = why.isEmpty() ? summary : why;
      if (button->statusTip() != status) button->setStatusTip(status);
    }
  }
  else
  {
    const HelperSpec::Lesson& lesson = HelperSpec::lessons[m_lesson];
    m_panel->pages()->setCurrentWidget(m_panel->lessonPage());
    m_panel->lessonTitle()->setText(tr2(lesson.title));
    if (m_done)
    {
      m_panel->progressLabel()->setText(tr2(HelperSpec::doneTitle));
      m_panel->stepLabel()->setText(tr2(lesson.doneText));
      m_panel->notOnScreenLabel()->hide();
    }
    else
    {
      const HelperSpec::Step& step = lesson.steps[m_step];
      m_panel->progressLabel()->setText(tr2(HelperSpec::stepProgress).arg(m_step + 1).arg(lesson.stepCount));
      m_panel->stepLabel()->setText(tr2(step.text));
      QWidget* target = m_targetFor ? m_targetFor(QLatin1String(step.target)) : nullptr;
      m_panel->notOnScreenLabel()->setVisible(!target || !target->isVisible());
    }
    m_panel->stopButton()->setVisible(!m_done);
    m_panel->backButton()->setVisible(m_done);
  }
  if (!m_panel->isVisible()) m_panel->show();
  m_panel->raise();
}

// at the bottom right of the part the panel stands beside, inside the host
void HelperMode::place()
{
  if (!m_on) return;
  const int margin = Theme::Size::spacingS;
  const int gap = Theme::Size::spacingL;
  const QRect hostRect(QPoint(0, 0), m_host->size());
  const int w = std::max(1, std::min(m_panel->panelWidth(), hostRect.width() - 2 * margin));
  QRect area = m_area ? m_area() : hostRect;
  if (area.width() < w + 2 * gap || area.height() <= 0) area = hostRect;
  const int h = std::max(1, std::min(m_panel->wantedHeight(), hostRect.height() - 2 * margin));
  int x = area.right() + 1 - w - gap;
  int y = area.bottom() + 1 - h - gap;
  x = std::max(margin, std::min(hostRect.width() - w - margin, x));
  y = std::max(margin, std::min(hostRect.height() - h - margin, y));
  const QRect geometry(x, y, w, h);
  if (m_panel->geometry() != geometry) m_panel->setGeometry(geometry);
}

bool HelperMode::eventFilter(QObject* watched, QEvent* event)
{
  if (watched != m_host && event->type() == QEvent::LayoutRequest && !m_refreshing)
  {
    place();
    return QObject::eventFilter(watched, event);
  }
  if (watched == m_host && !m_refreshing)
    switch (event->type())
    {
      case QEvent::Show:
      case QEvent::Resize:
      case QEvent::LayoutRequest:
        refresh();
        break;
      default:
        break;
    }
  return QObject::eventFilter(watched, event);
}
