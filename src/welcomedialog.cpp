/*
 * PinglyMotion - the welcome card (see welcomedialog.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "welcomedialog.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QToolButton>
#include <QVBoxLayout>

#include "camerahabitspec.h"
#include "dialogfit.h"
#include "emptystate.h"
#include "guidancespec.h"
#include "lucideicons.h"
#include "theme.h"
#include "usersettings.h"

namespace
{
  QString tr2(const char* text) { return QCoreApplication::translate("WelcomeDialog", text); }
}

WelcomeDialog::WelcomeDialog(QWidget* parent) : QDialog(parent), m_new(nullptr), m_pro(nullptr), m_cameraRow(nullptr), m_skip(nullptr)
{
  setObjectName(QStringLiteral("welcomeDialog"));
  setWindowTitle(tr2(GuidanceSpec::welcomeTitle));
  setWindowIcon(LucideIcons::icon(QStringLiteral("sparkles"), Theme::color(Theme::Token::Text), Theme::color(Theme::Token::TextDisabled)));

  QVBoxLayout* outer = DialogFit::contentLayout(this);
  outer->setSpacing(Theme::Size::spacingL);

  // the sparkles icon and the title
  QHBoxLayout* header = new QHBoxLayout();
  QLabel* icon = new QLabel(this);
  icon->setPixmap(LucideIcons::icon(QStringLiteral("sparkles"), Theme::color(Theme::Token::AccentText),
                                    Theme::color(Theme::Token::TextDisabled)).pixmap(24, 24));
  header->addWidget(icon);
  QLabel* title = new QLabel(tr2(GuidanceSpec::welcomeTitle), this);
  title->setObjectName(QStringLiteral("welcomeTitle"));
  QFont titleFont = title->font();
  titleFont.setBold(true);
  titleFont.setPointSizeF(titleFont.pointSizeF() * 1.3);
  title->setFont(titleFont);
  header->addWidget(title);
  header->addStretch(1);
  outer->addLayout(header);

  m_intro = new QLabel(tr2(GuidanceSpec::welcomeIntro), this);
  m_intro->setObjectName(QStringLiteral("welcomeIntro"));
  m_intro->setTextFormat(Qt::RichText);
  m_intro->setWordWrap(true);
  outer->addWidget(m_intro);

  // Have you made animations before?  New to animation / Experienced
  QLabel* experience = new QLabel(tr2(GuidanceSpec::experienceLabel), this);
  QFont labelFont = experience->font();
  labelFont.setWeight(QFont::DemiBold);
  experience->setFont(labelFont);
  outer->addWidget(experience);
  QHBoxLayout* experienceRow = new QHBoxLayout();
  QButtonGroup* experienceGroup = new QButtonGroup(this);
  m_new = new QRadioButton(tr2(GuidanceSpec::newChoice), this);
  m_new->setObjectName(QStringLiteral("welcomeNewRadio"));
  m_pro = new QRadioButton(tr2(GuidanceSpec::proChoice), this);
  m_pro->setObjectName(QStringLiteral("welcomeProRadio"));
  experienceGroup->addButton(m_new);
  experienceGroup->addButton(m_pro);
  experienceRow->addWidget(m_new);
  experienceRow->addWidget(m_pro);
  experienceRow->addStretch(1);
  outer->addLayout(experienceRow);
  // the card opens on what Setup says now (first start: New to animation)
  m_pro->setChecked(UserSettings::expertMode());
  m_new->setChecked(!UserSettings::expertMode());

  // Camera and keys like: only for the experienced, as in the mockup (nothing else moves: it is
  // an own row that is hidden or shown; the start choices stay where they are)
  m_cameraRow = new QWidget(this);
  m_cameraRow->setObjectName(QStringLiteral("welcomeCameraRow"));
  QVBoxLayout* cameraColumn = new QVBoxLayout(m_cameraRow);
  cameraColumn->setContentsMargins(0, 0, 0, 0);
  QLabel* cameraLabel = new QLabel(tr2(GuidanceSpec::cameraLabel), m_cameraRow);
  cameraLabel->setFont(labelFont);
  cameraColumn->addWidget(cameraLabel);
  QHBoxLayout* cameraButtons = new QHBoxLayout();
  QButtonGroup* cameraGroup = new QButtonGroup(this);
  const QString habit = UserSettings::cameraHabit();
  for (const CameraHabitSpec::Habit* candidate : CameraHabitSpec::all)
  {
    QRadioButton* radio = new QRadioButton(CameraHabitSpec::name(*candidate), m_cameraRow);
    radio->setObjectName(QStringLiteral("welcomeCamera_") + QLatin1String(candidate->id));
    radio->setChecked(QLatin1String(candidate->id) == habit);
    cameraGroup->addButton(radio);
    cameraButtons->addWidget(radio);
    m_cameras.append(radio);
  }
  cameraButtons->addStretch(1);
  cameraColumn->addLayout(cameraButtons);
  outer->addWidget(m_cameraRow);
  m_cameraRow->setVisible(m_pro->isChecked());
  connect(m_pro, &QRadioButton::toggled, m_cameraRow, &QWidget::setVisible);

  // the three start choices, the same as the empty state's (same clips)
  const QString text = Theme::color(Theme::Token::Text).name();
  const QString radius = QString::number(Theme::Size::radiusControl);
  for (const EmptySpec::Choice* choice : EmptySpec::choices)
  {
    QToolButton* button = new QToolButton(this);
    button->setObjectName(QStringLiteral("welcome_") + QLatin1String(choice->id));
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setText(QCoreApplication::translate("EmptyState", choice->text) + QLatin1Char('\n')
                    + QCoreApplication::translate("EmptyState", choice->small));
    button->setMinimumHeight(Theme::Size::controlLarge + Theme::Size::spacingM);
    const QString fill = (choice->main ? Theme::color(Theme::Token::Accent) : Theme::color(Theme::Token::SurfaceRaised)).name();
    const QString fg = (choice->main ? Theme::color(Theme::Token::OnAccent) : Theme::color(Theme::Token::Text)).name();
    const QString edge = (choice->main ? Theme::color(Theme::Token::Accent) : Theme::color(Theme::Token::Border)).name();
    button->setStyleSheet(QStringLiteral(
      "QToolButton { background: %1; color: %2; border: %5px solid %3; border-radius: %4px; padding: 8px 16px; }"
      "QToolButton:focus { border: %6px solid %7; }")
      .arg(fill, fg, edge, radius).arg(Theme::Size::borderWidth).arg(Theme::Size::focusRingWidth)
      .arg(Theme::color(Theme::Token::FocusRing).name()));
    const QString clip = QLatin1String(choice->clip);
    connect(button, &QToolButton::clicked, this, [this, clip]() { finishWith(clip); });
    m_starts.append(button);
    outer->addWidget(button);
  }
  Q_UNUSED(text);

  QHBoxLayout* actions = new QHBoxLayout();
  actions->addStretch(1);
  m_skip = new QToolButton(this);
  m_skip->setObjectName(QStringLiteral("welcomeSkipButton"));
  m_skip->setText(tr2(GuidanceSpec::skipButton));
  m_skip->setToolButtonStyle(Qt::ToolButtonTextOnly);
  connect(m_skip, &QToolButton::clicked, this, [this]() { finishWith(QString()); });
  actions->addWidget(m_skip);
  DialogFit::finish(this, actions);

  // the dance example (the main choice) has the keys first
  m_starts.last()->setFocus();
}

// the answer: what the user chose now, then close
void WelcomeDialog::collect(const QString& clipId)
{
  m_answer.skipped = clipId.isEmpty();
  m_answer.clipId = clipId;
  m_answer.expert = m_pro->isChecked();
  m_answer.cameraHabit.clear();
  if (m_answer.expert)
    for (int i = 0; i < m_cameras.size(); ++i)
      if (m_cameras.at(i)->isChecked()) m_answer.cameraHabit = QLatin1String(CameraHabitSpec::all[i]->id);
}

void WelcomeDialog::finishWith(const QString& clipId)
{
  collect(clipId);
  accept();
}

// Esc and the window's close button are Skip (the mockup's closeDialogs)
void WelcomeDialog::reject()
{
  collect(QString());
  QDialog::reject();
}

bool WelcomeDialog::due()
{
  return !UserSettings::welcomeSeen();
}

void WelcomeDialog::apply(const Answer& answer)
{
  UserSettings::setExpertMode(answer.expert);
  if (answer.expert && !answer.cameraHabit.isEmpty()) UserSettings::setCameraHabit(answer.cameraHabit);
  UserSettings::setWelcomeSeen(true);
}
