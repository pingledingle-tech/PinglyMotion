/*
 * PinglyMotion - the empty state (see emptystate.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "emptystate.h"

#include <QEvent>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

#include "lucideicons.h"
#include "theme.h"

namespace
{
  QString tr2(const char* text)
  {
    return QCoreApplication::translate("EmptyState", text);
  }
}

EmptyState::EmptyState(QWidget* parent) : QWidget(parent)
{
  setObjectName(QStringLiteral("emptyState"));
  setAutoFillBackground(true);

  QVBoxLayout* outer = new QVBoxLayout(this);
  outer->setContentsMargins(Theme::Size::spacingXl, Theme::Size::spacingXl, Theme::Size::spacingXl, Theme::Size::spacingXl);
  outer->addStretch(1);

  QWidget* holder = new QWidget(this);
  holder->setMinimumWidth(360);
  holder->setMaximumWidth(480);
  QVBoxLayout* column = new QVBoxLayout(holder);
  column->setContentsMargins(0, 0, 0, 0);
  column->setSpacing(Theme::Size::spacingM);

  m_icon = new QLabel(this);
  m_icon->setObjectName(QStringLiteral("emptyStateIcon"));
  m_icon->setAlignment(Qt::AlignCenter);
  column->addWidget(m_icon, 0, Qt::AlignHCenter);

  m_heading = new QLabel(tr2(EmptySpec::heading), this);
  m_heading->setObjectName(QStringLiteral("emptyStateHeading"));
  m_heading->setAlignment(Qt::AlignCenter);
  QFont headingFont = m_heading->font();
  headingFont.setBold(true);
  headingFont.setPointSizeF(headingFont.pointSizeF() * 1.3);
  m_heading->setFont(headingFont);
  column->addWidget(m_heading);

  m_line = new QLabel(tr2(EmptySpec::line), this);
  m_line->setObjectName(QStringLiteral("emptyStateLine"));
  m_line->setAlignment(Qt::AlignCenter);
  m_line->setWordWrap(true);
  column->addWidget(m_line);

  for (const EmptySpec::Choice* choice : EmptySpec::choices)
  {
    QToolButton* button = new QToolButton(this);
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    button->setFocusPolicy(Qt::StrongFocus);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setObjectName(QLatin1String(choice->id));
    button->setText(tr2(choice->text) + QLatin1Char('\n') + tr2(choice->small));
    button->setMinimumHeight(Theme::Size::controlLarge + Theme::Size::spacingM);
    button->setProperty("main", choice->main);
        const QString clip = QLatin1String(choice->clip);
    connect(button, &QToolButton::clicked, this, [this, clip]() { emit startChosen(clip); });
    m_choices.append(button);
    column->addWidget(button);
  }

  m_new = new QToolButton(this);
  m_new->setText(tr2(EmptySpec::newText));
  m_new->setToolButtonStyle(Qt::ToolButtonTextOnly);
  m_new->setFocusPolicy(Qt::StrongFocus);
  m_new->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_new->setObjectName(QStringLiteral("startNewButton"));
  m_new->setMinimumHeight(Theme::Size::controlLarge);
  connect(m_new, &QToolButton::clicked, this, &EmptyState::newChosen);
  column->addWidget(m_new);

  outer->addWidget(holder, 0, Qt::AlignHCenter);
  outer->addStretch(1);
  applyLook();

  if (parent)
  {
    parent->installEventFilter(this);
    setGeometry(parent->rect());
  }
}

bool EmptyState::eventFilter(QObject* watched, QEvent* event)
{
  if (watched == parentWidget() && event->type() == QEvent::Resize) setGeometry(parentWidget()->rect());
  return QWidget::eventFilter(watched, event);
}

void EmptyState::applyLook()
{
  using Theme::Token;
  QPalette palette = this->palette();
  palette.setColor(QPalette::Window, Theme::color(Token::Surface));
  setPalette(palette);

  const QColor text = Theme::color(Token::Text);
  m_icon->setPixmap(LucideIcons::icon(QStringLiteral("sparkles"), Theme::color(Token::AccentText),
                                      Theme::color(Token::TextDisabled)).pixmap(32, 32));
  m_heading->setStyleSheet(QStringLiteral("color: %1;").arg(text.name()));
  m_line->setStyleSheet(QStringLiteral("color: %1;").arg(Theme::color(Token::TextMuted).name()));

  // plain buttons in the design system's shape; the main one in the accent colour
  const QString border = Theme::color(Token::Border).name();
  const QString radius = QString::number(Theme::Size::radiusControl);
  for (QToolButton* button : m_choices + QList<QToolButton*>{m_new})
  {
    const bool main = button->property("main").toBool();
    const QString fill = (main ? Theme::color(Token::Accent) : Theme::color(Token::SurfaceRaised)).name();
    const QString fg = (main ? Theme::color(Token::OnAccent) : text).name();
    const QString edge = (main ? Theme::color(Token::Accent) : Theme::color(Token::Border)).name();
    button->setStyleSheet(QStringLiteral(
      "QToolButton { background: %1; color: %2; border: %6px solid %3; border-radius: %4px; padding: 8px 16px; }"
      "QToolButton:hover { background: %5; }"
      "QToolButton:focus { border: %7px solid %8; }")
      .arg(fill, fg, edge, radius, Theme::mix(QColor(fill), QColor(fg), 0.08).name())
      .arg(Theme::Size::borderWidth).arg(Theme::Size::focusRingWidth).arg(Theme::color(Token::FocusRing).name()));
  }
  Q_UNUSED(border);
}
