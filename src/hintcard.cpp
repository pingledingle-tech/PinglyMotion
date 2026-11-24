/*
 * PinglyMotion - the hint cards (see hintcard.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "hintcard.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QPainter>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

#include "guidancespec.h"
#include "lucideicons.h"
#include "theme.h"
#include "usersettings.h"

namespace
{
  QString tr2(const char* text) { return QCoreApplication::translate("HintCard", text); }

  // the dots under the text, one per card; the current one is longer (the mockup's .dots)
  class Dots : public QWidget
  {
    public:
      explicit Dots(QWidget* parent) : QWidget(parent) { setObjectName(QStringLiteral("hintCardDots")); }
      void setPlace(int index, int count)
      {
        m_index = index;
        m_count = count;
        setAccessibleName(tr2(GuidanceSpec::progress).arg(index + 1).arg(count));
        updateGeometry();
        update();
      }
      QSize sizeHint() const override { return QSize(m_count * 6 + 8 + (m_count - 1) * 3, 6); }

    protected:
      void paintEvent(QPaintEvent*) override
      {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        int x = 0;
        const int y = (height() - 6) / 2;
        for (int i = 0; i < m_count; ++i)
        {
          const bool on = i == m_index;
          painter.setBrush(Theme::color(on ? Theme::Token::AccentText : Theme::Token::Border));
          const int w = on ? 14 : 6;
          painter.drawRoundedRect(QRect(x, y, w, 6), on ? 3 : 3, 3);
          x += w + 3;
        }
      }

    private:
      int m_index = 0;
      int m_count = GuidanceSpec::cardCount;   // the row has this many dots from the start
  };
}

HintCard::HintCard(QWidget* parent) : QFrame(parent)
{
  setObjectName(QStringLiteral("hintCard"));
  setAttribute(Qt::WA_StyledBackground, true);
  setFocusPolicy(Qt::StrongFocus);

  QVBoxLayout* column = new QVBoxLayout(this);
  column->setContentsMargins(Theme::Size::spacingM, Theme::Size::spacingM, Theme::Size::spacingM, Theme::Size::spacingM);
  column->setSpacing(Theme::Size::spacingXs);

  // the title row: a lightbulb in the warning colour, the title, and the close button at the right
  QHBoxLayout* head = new QHBoxLayout();
  head->setSpacing(Theme::Size::spacingS);
  m_icon = new QLabel(this);
  m_icon->setObjectName(QStringLiteral("hintCardIcon"));
  head->addWidget(m_icon);
  m_title = new QLabel(this);
  m_title->setObjectName(QStringLiteral("hintCardTitle"));
  m_title->setWordWrap(true);
  QFont titleFont = m_title->font();
  titleFont.setWeight(QFont::DemiBold);
  m_title->setFont(titleFont);
  head->addWidget(m_title, 1);
  m_close = new QToolButton(this);
  m_close->setObjectName(QStringLiteral("hintCardClose"));
  m_close->setToolTip(tr2(GuidanceSpec::closeName));
  m_close->setAccessibleName(tr2(GuidanceSpec::closeName));
  m_close->setAutoRaise(true);
  m_close->setFixedSize(Theme::Size::controlCompact, Theme::Size::controlCompact);
  head->addWidget(m_close, 0, Qt::AlignTop);
  column->addLayout(head);

  m_text = new QLabel(this);
  m_text->setObjectName(QStringLiteral("hintCardText"));
  m_text->setWordWrap(true);
  column->addWidget(m_text);
  column->addSpacing(Theme::Size::spacingS);

  QHBoxLayout* foot = new QHBoxLayout();
  foot->setSpacing(Theme::Size::spacingS);
  m_hintsOff = new QToolButton(this);
  m_hintsOff->setObjectName(QStringLiteral("hintCardHintsOff"));
  m_hintsOff->setText(tr2(GuidanceSpec::hintsOffButton));
  m_hintsOff->setToolTip(tr2(GuidanceSpec::hintsOffName));
  m_hintsOff->setAccessibleName(tr2(GuidanceSpec::hintsOffName));
  m_hintsOff->setToolButtonStyle(Qt::ToolButtonTextOnly);
  foot->addWidget(m_hintsOff);
  foot->addStretch(1);
  m_dots = new Dots(this);
  foot->addWidget(m_dots);
  foot->addStretch(1);
  m_next = new QToolButton(this);
  m_next->setObjectName(QStringLiteral("hintCardNext"));
  m_next->setText(tr2(GuidanceSpec::nextButton));
  m_next->setToolTip(tr2(GuidanceSpec::nextName));
  m_next->setAccessibleName(tr2(GuidanceSpec::nextName));
  m_next->setToolButtonStyle(Qt::ToolButtonTextOnly);
  foot->addWidget(m_next);
  column->addLayout(foot);

  connect(m_next, &QToolButton::clicked, this, &HintCard::nextRequested);
  connect(m_close, &QToolButton::clicked, this, &HintCard::closeRequested);
  connect(m_hintsOff, &QToolButton::clicked, this, &HintCard::hintsOffRequested);
  applyLook();
  fitWidth();
}

// 300 pixels at 100 percent text (the mockup), wider with bigger text, and never narrower than the
// footer's three parts need
void HintCard::fitWidth()
{
  if (!m_next || !m_dots || !m_hintsOff) return;
  const int footer = m_hintsOff->sizeHint().width() + m_dots->sizeHint().width() + m_next->sizeHint().width()
                     + 4 * Theme::Size::spacingS + 2 * Theme::Size::spacingM + 2 * Theme::Size::borderWidth;
  const int scaled = 300 * UserSettings::textSizePercent() / 100;
  setFixedWidth(std::max(scaled, footer));
}

QSize HintCard::sizeHint() const
{
  const int w = width() > 0 && minimumWidth() == maximumWidth() ? width() : 300;
  return QSize(w, layout() ? layout()->totalHeightForWidth(w) : 0);
}

void HintCard::changeEvent(QEvent* event)
{
  QFrame::changeEvent(event);
  if (event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange) fitWidth();
}

void HintCard::setCard(int index, int count, const QString& title, const QString& text)
{
  m_index = index;
  m_count = count;
  m_title->setText(title);
  m_text->setText(text);
  static_cast<Dots*>(m_dots)->setPlace(index, count);
  resize(width(), sizeHint().height());
}

void HintCard::applyLook()
{
  using Theme::Token;
  const int radius = Theme::Size::radiusPanel;
  setStyleSheet(QStringLiteral("QFrame#hintCard { background: %1; border: %2px solid %3; border-radius: %4px; }")
                .arg(Theme::color(Token::SurfaceRaised).name()).arg(Theme::Size::borderWidth)
                .arg(Theme::color(Token::AccentText).name()).arg(radius));
  m_icon->setPixmap(LucideIcons::icon(QStringLiteral("lightbulb"), Theme::color(Token::Warning),
                                      Theme::color(Token::TextDisabled)).pixmap(16, 16));
  m_close->setIcon(LucideIcons::icon(QStringLiteral("x"), Theme::color(Token::Text), Theme::color(Token::TextDisabled)));
  const QString text = Theme::color(Token::Text).name();
  for (QLabel* label : {m_title, m_text}) label->setStyleSheet(QStringLiteral("color: %1; background: transparent; border: 0;").arg(text));
  m_icon->setStyleSheet(QStringLiteral("background: transparent; border: 0;"));
  const QString edge = Theme::color(Token::Border).name();
  const QString control = QString::number(Theme::Size::radiusControl);
  m_hintsOff->setStyleSheet(QStringLiteral(
    "QToolButton { background: %1; color: %2; border: %5px solid %3; border-radius: %4px; padding: 2px 8px; }"
    "QToolButton:focus { border: %6px solid %7; }")
    .arg(Theme::color(Token::SurfaceRaised).name(), text, edge, control).arg(Theme::Size::borderWidth)
    .arg(Theme::Size::focusRingWidth).arg(Theme::color(Token::FocusRing).name()));
  m_next->setStyleSheet(QStringLiteral(
    "QToolButton { background: %1; color: %2; border: %4px solid %1; border-radius: %3px; padding: 2px 12px; }"
    "QToolButton:focus { border: %5px solid %6; }")
    .arg(Theme::color(Token::Accent).name(), Theme::color(Token::OnAccent).name(), control).arg(Theme::Size::borderWidth)
    .arg(Theme::Size::focusRingWidth).arg(Theme::color(Token::FocusRing).name()));
  m_dots->update();
}

// Esc on the card closes it, as its close button does
void HintCard::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_Escape)
  {
    emit closeRequested();
    return;
  }
  QFrame::keyPressEvent(event);
}

// ---------------------------------------------------------------------------

HintCards::HintCards(QWidget* host, TargetFor targetFor) :
  QObject(host), m_host(host), m_targetFor(std::move(targetFor)), m_card(new HintCard(host)),
  m_index(std::clamp(UserSettings::hintCard(), 0, GuidanceSpec::cardCount))
{
  m_card->hide();
  connect(m_card, &HintCard::nextRequested, this, &HintCards::next);
  connect(m_card, &HintCard::closeRequested, this, &HintCards::close);
  connect(m_card, &HintCard::hintsOffRequested, this, &HintCards::hintsOffRequested);
  host->installEventFilter(this);
}

bool HintCards::cardShown() const
{
  return m_card->isVisibleTo(m_host);
}

void HintCards::setHintsOn(bool on)
{
  // hints turned off: a closed card is forgotten, so turning them on again shows the next card
  if (!on) m_closed = false;
  m_hintsOn = on;
  refresh();
}

void HintCards::setAnimationOpen(bool open)
{
  m_open = open;
  refresh();
}

void HintCards::setPicked(bool picked)
{
  m_picked = picked;
  refresh();
}

void HintCards::markPulled()
{
  m_pulled = true;
  refresh();
}

void HintCards::markTurned()
{
  m_turned = true;
  refresh();
}

void HintCards::next()
{
  if (m_index < GuidanceSpec::cardCount) ++m_index;
  UserSettings::setHintCard(m_index);
  refresh();
}

void HintCards::close()
{
  const bool shown = cardShown();
  if (m_index < GuidanceSpec::cardCount) ++m_index;
  UserSettings::setHintCard(m_index);
  m_closed = true;
  refresh();
  if (shown) emit cardClosed();
}

void HintCards::restart()
{
  m_closed = false;
  m_index = 0;
  UserSettings::setHintCard(0);
  refresh();
}

// a card whose job is done is not shown (the mockup's "skip what is done already")
bool HintCards::isDone(int index) const
{
  switch (index)
  {
    case 0: return m_picked;
    case 1: return m_pulled;
    case 2: return m_turned;
    default: return false;
  }
}

void HintCards::refresh()
{
  if (m_refreshing) return;
  m_refreshing = true;
  refreshNow();
  m_refreshing = false;
  emit changed();
}

void HintCards::setHolder(QWidget* holder)
{
  m_embedded = holder != nullptr;
  m_card->setParent(holder ? holder : m_host);
  m_card->hide();
  refresh();
}

QWidget* HintCards::currentTarget() const
{
  if (!cardShown() || m_index >= GuidanceSpec::cardCount || !m_targetFor) return nullptr;
  return m_targetFor(QLatin1String(GuidanceSpec::cards[m_index].target));
}

void HintCards::refreshNow()
{
  while (m_index < GuidanceSpec::cardCount && isDone(m_index))
  {
    ++m_index;
    UserSettings::setHintCard(m_index);
  }
  if (m_closed || !m_hintsOn || !m_open || m_index >= GuidanceSpec::cardCount)
  {
    m_card->hide();
    return;
  }
  const GuidanceSpec::Card& spec = GuidanceSpec::cards[m_index];
  QWidget* target = m_targetFor ? m_targetFor(QLatin1String(spec.target)) : nullptr;
  if (!target || !target->isVisible())
  {
    m_card->hide();
    return;
  }
  m_card->setCard(m_index, GuidanceSpec::cardCount, QCoreApplication::translate("HintCard", spec.title),
                  QCoreApplication::translate("HintCard", spec.text));
  if (m_embedded)
  {
    m_card->show();
    return;
  }
  place(QLatin1String(spec.target));
  m_card->show();
  m_card->raise();
}

// beside the part the card talks about, inside the window (the mockup's showCard rules)
void HintCards::place(const QString& targetName)
{
  QWidget* target = m_targetFor(targetName);
  const QPoint topLeft = m_host->mapFromGlobal(target->mapToGlobal(QPoint(0, 0)));
  const QRect r(topLeft, target->size());
  const int w = m_card->width();
  const int h = m_card->sizeHint().height();
  const int gap = Theme::Size::spacingM;
  int x = r.left() + Theme::Size::spacingL;
  int y = r.top() + Theme::Size::spacingL;
  if (targetName == QLatin1String("setKey") || targetName == QLatin1String("start") || targetName == QLatin1String("tiles"))
  {
    x = r.left();
    y = r.top() - h - gap;
  }
  else if (targetName == QLatin1String("view"))
  {
    // bottom right, so it never covers the figure or the view tools
    x = r.right() - w - Theme::Size::spacingL;
    y = r.bottom() - h - 56;
  }
  else if (targetName == QLatin1String("selected"))
  {
    x = r.left() - w - gap;
    y = r.top() + 40;
  }
  else if (targetName == QLatin1String("joints"))
  {
    x = r.right() + gap;
    y = r.top() + 40;
  }
  else if (targetName == QLatin1String("setup"))
  {
    x = r.right() - w;
    y = r.bottom() + gap;
  }
  const int margin = Theme::Size::spacingS;
  x = std::max(margin, std::min(m_host->width() - w - margin, x));
  y = std::max(margin, std::min(m_host->height() - h - margin, y));
  m_card->setGeometry(x, y, w, h);
}

bool HintCards::eventFilter(QObject* watched, QEvent* event)
{
  if (watched == m_host && !m_refreshing)
    switch (event->type())
    {
      // the window was shown, moved or resized, or its parts were laid out again: the part a card
      // talks about may be on screen now, or somewhere else
      case QEvent::Show:
      case QEvent::Resize:
      case QEvent::Move:
      case QEvent::LayoutRequest:
        refresh();
        break;
      default:
        break;
    }
  return QObject::eventFilter(watched, event);
}
