/*
 * PinglyMotion - the Setup dialog (see setupdialog.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "setupdialog.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

#include "camerahabitspec.h"
#include "lucideicons.h"
#include "theme.h"
#include "usersettings.h"

namespace
{
  QString tr2(const char* text) { return QCoreApplication::translate("SetupDialog", text); }

  QLabel* sectionLabel(QWidget* parent, const QString& text)
  {
    QLabel* label = new QLabel(text, parent);
    QFont font = label->font();
    font.setWeight(QFont::DemiBold);
    label->setFont(font);
    return label;
  }

  QLabel* captionLabel(QWidget* parent, const QString& text)
  {
    QLabel* label = new QLabel(text, parent);
    label->setWordWrap(true);
    QPalette palette = label->palette();
    palette.setColor(QPalette::WindowText, Theme::color(Theme::Token::TextMuted));
    label->setPalette(palette);
    return label;
  }
}

SetupDialog::SetupDialog(QWidget* parent) :
  QDialog(parent),
  m_beginnerRadio(nullptr), m_expertRadio(nullptr), m_darkRadio(nullptr), m_lightRadio(nullptr), m_hintsCheck(nullptr)
{
  setObjectName(QStringLiteral("setupDialog"));
  setWindowTitle(tr2(SetupText::title));
  setWindowIcon(LucideIcons::icon(QStringLiteral("settings"), Theme::color(Theme::Token::Text), Theme::color(Theme::Token::TextDisabled)));

  QVBoxLayout* outer = new QVBoxLayout(this);
  outer->setSpacing(Theme::Size::spacingL);

  QHBoxLayout* header = new QHBoxLayout();
  QLabel* icon = new QLabel(this);
  icon->setPixmap(LucideIcons::icon(QStringLiteral("settings"), Theme::color(Theme::Token::Text),
                                     Theme::color(Theme::Token::TextDisabled)).pixmap(24, 24));
  header->addWidget(icon);
  QLabel* title = new QLabel(tr2(SetupText::title), this);
  QFont titleFont = title->font();
  titleFont.setBold(true);
  titleFont.setPointSizeF(titleFont.pointSizeF() * 1.3);
  title->setFont(titleFont);
  header->addWidget(title);
  header->addStretch(1);
  outer->addLayout(header);

  outer->addWidget(captionLabel(this, tr2(SetupText::intro)));

  // Have you made animations before? New to animation (Beginner mode) / Experienced (Expert mode)
  outer->addWidget(sectionLabel(this, tr2(SetupText::experienceLabel)));
  QHBoxLayout* experienceRow = new QHBoxLayout();
  QButtonGroup* experienceGroup = new QButtonGroup(this);
  m_beginnerRadio = new QRadioButton(tr2(SetupText::beginnerChoice), this);
  m_expertRadio = new QRadioButton(tr2(SetupText::expertChoice), this);
  experienceGroup->addButton(m_beginnerRadio);
  experienceGroup->addButton(m_expertRadio);
  experienceRow->addWidget(m_beginnerRadio);
  experienceRow->addWidget(m_expertRadio);
  experienceRow->addStretch(1);
  outer->addLayout(experienceRow);
  outer->addWidget(captionLabel(this, tr2(SetupText::experienceCaption)));
  m_expertRadio->setChecked(UserSettings::expertMode());
  m_beginnerRadio->setChecked(!UserSettings::expertMode());
  connect(m_expertRadio, &QRadioButton::toggled, this, [this](bool on) { if (on) emit expertModeRequested(true); });
  connect(m_beginnerRadio, &QRadioButton::toggled, this, [this](bool on) { if (on) emit expertModeRequested(false); });

  // Mouse and keys like: PinglyMotion, Maya, Blender, Second Life
  outer->addWidget(sectionLabel(this, tr2(SetupText::cameraLabel)));
  QVBoxLayout* cameraColumn = new QVBoxLayout();
  QButtonGroup* cameraGroup = new QButtonGroup(this);
  const QString habit = UserSettings::cameraHabit();
  for (const CameraHabitSpec::Habit* candidate : CameraHabitSpec::all)
  {
    QRadioButton* radio = new QRadioButton(CameraHabitSpec::name(*candidate), this);
    radio->setChecked(QLatin1String(candidate->id) == habit);
    const QString id = QLatin1String(candidate->id);
    connect(radio, &QRadioButton::toggled, this, [this, id](bool on) { if (on) emit cameraHabitRequested(id); });
    cameraGroup->addButton(radio);
    cameraColumn->addWidget(radio);
    m_cameraRadios.append(radio);
  }
  outer->addLayout(cameraColumn);
  QPushButton* showKeys = new QPushButton(tr2(SetupText::showAllKeys), this);
  showKeys->setIcon(LucideIcons::icon(QStringLiteral("keyboard"), Theme::color(Theme::Token::Text), Theme::color(Theme::Token::TextDisabled)));
  connect(showKeys, &QPushButton::clicked, this, [this]() { emit showCameraKeysRequested(); });
  outer->addWidget(showKeys, 0, Qt::AlignLeft);

  // Theme: Dark / Light
  outer->addWidget(sectionLabel(this, tr2(SetupText::themeLabel)));
  QHBoxLayout* themeRow = new QHBoxLayout();
  QButtonGroup* themeGroup = new QButtonGroup(this);
  m_darkRadio = new QRadioButton(tr2(SetupText::darkChoice), this);
  m_darkRadio->setIcon(LucideIcons::icon(QStringLiteral("moon"), Theme::color(Theme::Token::Text), Theme::color(Theme::Token::TextDisabled)));
  m_lightRadio = new QRadioButton(tr2(SetupText::lightChoice), this);
  m_lightRadio->setIcon(LucideIcons::icon(QStringLiteral("sun"), Theme::color(Theme::Token::Text), Theme::color(Theme::Token::TextDisabled)));
  themeGroup->addButton(m_darkRadio);
  themeGroup->addButton(m_lightRadio);
  themeRow->addWidget(m_darkRadio);
  themeRow->addWidget(m_lightRadio);
  themeRow->addStretch(1);
  outer->addLayout(themeRow);
  m_darkRadio->setChecked(UserSettings::darkTheme());
  m_lightRadio->setChecked(!UserSettings::darkTheme());
  connect(m_darkRadio, &QRadioButton::toggled, this, [this](bool on) { if (on) emit darkThemeRequested(true); });
  connect(m_lightRadio, &QRadioButton::toggled, this, [this](bool on) { if (on) emit darkThemeRequested(false); });

  // Text size: 100% to 200%, steps of 25 (docs/UX_DECISIONS.md, section 8)
  outer->addWidget(sectionLabel(this, tr2(SetupText::textSizeLabel)));
  QHBoxLayout* sizeRow = new QHBoxLayout();
  QButtonGroup* sizeGroup = new QButtonGroup(this);
  const int currentSize = UserSettings::textSizePercent();
  for (int percent : SetupText::sizes)
  {
    QRadioButton* radio = new QRadioButton(QStringLiteral("%1%").arg(percent), this);
    radio->setChecked(percent == currentSize);
    connect(radio, &QRadioButton::toggled, this, [this, percent](bool on) { if (on) emit textSizeRequested(percent); });
    sizeGroup->addButton(radio);
    sizeRow->addWidget(radio);
    m_textSizeRadios.append(radio);
  }
  sizeRow->addStretch(1);
  outer->addLayout(sizeRow);

  // Guidance: show hints, tips and the next-step line (the same setting as Help > Show hints
  // and the status bar's Tips switch)
  outer->addWidget(sectionLabel(this, tr2(SetupText::guidanceLabel)));
  m_hintsCheck = new QCheckBox(tr2(SetupText::hintsChoice), this);
  m_hintsCheck->setChecked(UserSettings::tipsOn());
  connect(m_hintsCheck, &QCheckBox::toggled, this, [this](bool on) { emit hintsRequested(on); });
  outer->addWidget(m_hintsCheck, 0, Qt::AlignLeft);

  QHBoxLayout* actions = new QHBoxLayout();
  actions->addStretch(1);
  QPushButton* done = new QPushButton(tr2(SetupText::doneButton), this);
  done->setDefault(true);
  connect(done, &QPushButton::clicked, this, &QDialog::accept);
  actions->addWidget(done);
  outer->addLayout(actions);
}

bool SetupDialog::expertChecked() const { return m_expertRadio && m_expertRadio->isChecked(); }

QString SetupDialog::cameraHabitChecked() const
{
  for (int i = 0; i < m_cameraRadios.size(); ++i)
    if (m_cameraRadios.at(i)->isChecked()) return QLatin1String(CameraHabitSpec::all[i]->id);
  return QStringLiteral("pinglymotion");
}

bool SetupDialog::darkChecked() const { return m_darkRadio && m_darkRadio->isChecked(); }

int SetupDialog::textSizeChecked() const
{
  for (int i = 0; i < m_textSizeRadios.size(); ++i)
    if (m_textSizeRadios.at(i)->isChecked()) return SetupText::sizes[i];
  return 100;
}

bool SetupDialog::hintsChecked() const { return m_hintsCheck && m_hintsCheck->isChecked(); }
