/*
 * PinglyMotion - the Keys and mouse dialog (see keysmousedialog.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "keysmousedialog.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "lucideicons.h"
#include "theme.h"

namespace
{
  QString tr2(const char* text) { return QCoreApplication::translate("KeysMouseDialog", text); }
}

KeysMouseDialog::KeysMouseDialog(QWidget* parent) : QDialog(parent), m_table(nullptr)
{
  setObjectName(QStringLiteral("keysMouseDialog"));
  setWindowTitle(tr2(KeysMouseText::title));
  setWindowIcon(LucideIcons::icon(QStringLiteral("keyboard"), Theme::color(Theme::Token::Text), Theme::color(Theme::Token::TextDisabled)));

  QVBoxLayout* outer = new QVBoxLayout(this);

  QHBoxLayout* header = new QHBoxLayout();
  QLabel* icon = new QLabel(this);
  icon->setPixmap(LucideIcons::icon(QStringLiteral("keyboard"), Theme::color(Theme::Token::Text),
                                     Theme::color(Theme::Token::TextDisabled)).pixmap(24, 24));
  header->addWidget(icon);
  QLabel* title = new QLabel(tr2(KeysMouseText::title), this);
  QFont titleFont = title->font();
  titleFont.setBold(true);
  titleFont.setPointSizeF(titleFont.pointSizeF() * 1.3);
  title->setFont(titleFont);
  header->addWidget(title);
  header->addStretch(1);
  outer->addLayout(header);

  const int rowCount = int(sizeof(KeysMouseSpec::rows) / sizeof(KeysMouseSpec::rows[0]));
  m_table = new QTableWidget(rowCount, 2, this);
  m_table->setHorizontalHeaderLabels({tr2(KeysMouseText::actionHeader), tr2(KeysMouseText::gestureHeader)});
  m_table->verticalHeader()->setVisible(false);
  m_table->horizontalHeader()->setStretchLastSection(true);
  m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_table->setSelectionMode(QAbstractItemView::NoSelection);
  m_table->setFocusPolicy(Qt::NoFocus);
  m_table->setWordWrap(true);
  setMinimumWidth(520);
  for (int row = 0; row < rowCount; ++row)
  {
    m_table->setItem(row, 0, new QTableWidgetItem(KeysMouseSpec::action(KeysMouseSpec::rows[row])));
    m_table->setItem(row, 1, new QTableWidgetItem(KeysMouseSpec::gesture(KeysMouseSpec::rows[row])));
  }
  m_table->resizeColumnToContents(0);
  m_table->resizeRowsToContents();
  outer->addWidget(m_table);

  QHBoxLayout* actions = new QHBoxLayout();
  actions->addStretch(1);
  QPushButton* close = new QPushButton(tr2(KeysMouseText::closeButton), this);
  close->setDefault(true);
  connect(close, &QPushButton::clicked, this, &QDialog::accept);
  actions->addWidget(close);
  outer->addLayout(actions);
}
