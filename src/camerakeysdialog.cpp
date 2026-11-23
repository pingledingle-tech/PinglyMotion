/*
 * PinglyMotion - the Camera and keys dialog (see camerakeysdialog.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "camerakeysdialog.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "camerahabitspec.h"
#include "dialogfit.h"
#include "lucideicons.h"
#include "theme.h"
#include "usersettings.h"

namespace
{
  QString tr2(const char* text) { return QCoreApplication::translate("CameraKeysDialog", text); }
}

CameraKeysDialog::CameraKeysDialog(QWidget* parent) : QDialog(parent), m_table(nullptr)
{
  setObjectName(QStringLiteral("cameraKeysDialog"));
  setWindowTitle(tr2(CameraKeysText::title));
  setWindowIcon(LucideIcons::icon(QStringLiteral("video"), Theme::color(Theme::Token::Text), Theme::color(Theme::Token::TextDisabled)));

  QVBoxLayout* outer = DialogFit::contentLayout(this);

  QHBoxLayout* header = new QHBoxLayout();
  QLabel* icon = new QLabel(this);
  icon->setPixmap(LucideIcons::icon(QStringLiteral("video"), Theme::color(Theme::Token::Text),
                                     Theme::color(Theme::Token::TextDisabled)).pixmap(24, 24));
  header->addWidget(icon);
  QLabel* title = new QLabel(tr2(CameraKeysText::title), this);
  QFont titleFont = title->font();
  titleFont.setBold(true);
  titleFont.setPointSizeF(titleFont.pointSizeF() * 1.3);
  title->setFont(titleFont);
  header->addWidget(title);
  header->addStretch(1);
  outer->addLayout(header);

  QLabel* intro = new QLabel(tr2(CameraKeysText::intro), this);
  intro->setWordWrap(true);
  outer->addWidget(intro);

  QVBoxLayout* radios = new QVBoxLayout();
  QButtonGroup* group = new QButtonGroup(this);
  const QString habit = UserSettings::cameraHabit();
  for (const CameraHabitSpec::Habit* candidate : CameraHabitSpec::all)
  {
    QRadioButton* radio = new QRadioButton(CameraHabitSpec::name(*candidate), this);
    radio->setChecked(QLatin1String(candidate->id) == habit);
    const QString id = QLatin1String(candidate->id);
    connect(radio, &QRadioButton::toggled, this, [this, id](bool on)
    {
      if (!on) return;
      emit cameraHabitRequested(id);
      fillTable(id);
    });
    group->addButton(radio);
    radios->addWidget(radio);
    m_radios.append(radio);
  }
  outer->addLayout(radios);

  m_table = new QTableWidget(5 + CameraHabitSpec::sharedRowCount, 2, this);
  m_table->setHorizontalHeaderLabels({tr2(CameraKeysText::actionHeader), tr2(CameraKeysText::gestureHeader)});
  m_table->verticalHeader()->setVisible(false);
  m_table->horizontalHeader()->setStretchLastSection(true);
  m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_table->setSelectionMode(QAbstractItemView::NoSelection);
  m_table->setFocusPolicy(Qt::NoFocus);
  m_table->setWordWrap(true);
  setMinimumWidth(520);
  outer->addWidget(m_table);
  fillTable(habit);

  QHBoxLayout* actions = new QHBoxLayout();
  actions->addStretch(1);
  QPushButton* done = new QPushButton(tr2(CameraKeysText::doneButton), this);
  done->setDefault(true);
  connect(done, &QPushButton::clicked, this, &QDialog::accept);
  actions->addWidget(done);
  DialogFit::finish(this, actions);
}

void CameraKeysDialog::fillTable(const QString& habitId)
{
  const CameraHabitSpec::Habit& habit = CameraHabitSpec::find(habitId);
  for (int row = 0; row < 5; ++row)
  {
    m_table->setItem(row, 0, new QTableWidgetItem(CameraHabitSpec::action(habit.rows[row])));
    m_table->setItem(row, 1, new QTableWidgetItem(CameraHabitSpec::gesture(habit.rows[row])));
  }
  // the keys that work the same in every habit, under the habit's own rows (the mockup's camTable())
  for (int row = 0; row < CameraHabitSpec::sharedRowCount; ++row)
  {
    m_table->setItem(5 + row, 0, new QTableWidgetItem(CameraHabitSpec::action(CameraHabitSpec::sharedRows[row])));
    m_table->setItem(5 + row, 1, new QTableWidgetItem(CameraHabitSpec::gesture(CameraHabitSpec::sharedRows[row])));
  }
  m_table->resizeColumnToContents(0);
  m_table->resizeRowsToContents();
}

QString CameraKeysDialog::habitChecked() const
{
  for (int i = 0; i < m_radios.size(); ++i)
    if (m_radios.at(i)->isChecked()) return QLatin1String(CameraHabitSpec::all[i]->id);
  return QStringLiteral("pinglymotion");
}
