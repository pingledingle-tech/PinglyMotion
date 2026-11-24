/*
 * PinglyMotion Script Lab - a script's menu as a box over the 3D view (job A2)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "labdialogbox.h"
#include "textsize.h"
#include "theme.h"

namespace
{
  // design_tokens.json: spacing.s, spacing.m, control_height.default, radius.control, radius.panel
  const int SpacingS = 8;
  const int SpacingM = 12;
  const int ControlHeight = 32;
  const int RadiusControl = 6;
  const int RadiusPanel = 8;
  // the box's width at 100 percent text size: three buttons of about 96 pixels side by side
  const int BoxWidth = 336;

  QString hex(Theme::Token token, bool dark)
  {
    return Theme::color(token, dark).name(QColor::HexRgb).toUpper();
  }
}

LabDialogBox::LabDialogBox(QWidget* host) :
  QFrame(host)
{
  setObjectName("scriptLabMenuBox");
  setFrameShape(QFrame::NoFrame);
  setAttribute(Qt::WA_StyledBackground, true);
  setAutoFillBackground(true);
  setFocusPolicy(Qt::NoFocus);
  setAccessibleName(tr("Menu from a script"));

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(SpacingM, SpacingM, SpacingM, SpacingM);
  layout->setSpacing(SpacingS);

  m_title = new QLabel(this);
  m_title->setObjectName("scriptLabMenuTitle");
  m_title->setTextFormat(Qt::PlainText);
  m_title->setWordWrap(true);
  QFont titleFont = m_title->font();
  titleFont.setWeight(QFont::DemiBold);
  m_title->setFont(titleFont);
  layout->addWidget(m_title);

  m_message = new QLabel(this);
  m_message->setObjectName("scriptLabMenuMessage");
  m_message->setTextFormat(Qt::PlainText);
  m_message->setWordWrap(true);
  m_message->setTextInteractionFlags(Qt::TextSelectableByMouse);
  layout->addWidget(m_message);

  m_buttonBox = new QWidget(this);
  m_buttonBox->setObjectName("scriptLabMenuButtons");
  m_grid = new QGridLayout(m_buttonBox);
  m_grid->setContentsMargins(0, 0, 0, 0);
  m_grid->setSpacing(SpacingS);
  layout->addWidget(m_buttonBox);

  // llTextBox: a field and Send instead of the buttons
  m_textRow = new QWidget(this);
  QHBoxLayout* textLayout = new QHBoxLayout(m_textRow);
  textLayout->setContentsMargins(0, 0, 0, 0);
  textLayout->setSpacing(SpacingS);
  m_text = new QLineEdit(m_textRow);
  m_text->setObjectName("scriptLabMenuText");
  m_text->setMinimumHeight(ControlHeight);
  m_text->setMaxLength(512);
  m_text->setPlaceholderText(tr("Type your answer"));
  m_text->setAccessibleName(tr("Your answer"));
  m_send = new QPushButton(tr("Send"), m_textRow);
  m_send->setObjectName("scriptLabMenuSend");
  m_send->setMinimumHeight(ControlHeight);
  m_send->setToolTip(tr("Send the text, as the owner's avatar types it in the box"));
  textLayout->addWidget(m_text, 1);
  textLayout->addWidget(m_send);
  layout->addWidget(m_textRow);
  connect(m_send, &QPushButton::clicked, this, &LabDialogBox::sendText);
  connect(m_text, &QLineEdit::returnPressed, this, &LabDialogBox::sendText);
  connect(m_text, &QLineEdit::textChanged, this, [this]() { m_send->setEnabled(!m_text->text().isEmpty()); });

  m_channel = new QLabel(this);
  m_channel->setObjectName("scriptLabMenuChannel");
  m_channel->setTextFormat(Qt::PlainText);
  m_channel->setWordWrap(true);
  layout->addWidget(m_channel);

  QHBoxLayout* bottom = new QHBoxLayout;
  bottom->setSpacing(SpacingS);
  bottom->addStretch(1);
  m_ignore = new QPushButton(tr("Ignore"), this);
  m_ignore->setObjectName("scriptLabMenuIgnore");
  m_ignore->setMinimumHeight(ControlHeight);
  m_ignore->setToolTip(tr("Close the menu and send nothing (Esc)"));
  bottom->addWidget(m_ignore);
  layout->addLayout(bottom);
  connect(m_ignore, &QPushButton::clicked, this, [this]()
  {
    hide();
    emit ignored();
  });

  applyStyle();
  hide();
}

QList<QPushButton*> LabDialogBox::fillButtonGrid(QGridLayout* grid, QWidget* parent, const QStringList& buttons,
                                                 const QString& objectName, const std::function<void(const QString&)>& clicked)
{
  // the old buttons go first
  for (QPushButton* old : parent->findChildren<QPushButton*>(objectName, Qt::FindDirectChildrenOnly))
  {
    grid->removeWidget(old);
    old->deleteLater();
    old->setParent(nullptr);
  }
  QList<QPushButton*> made;
  for (int i = 0; i < buttons.size(); ++i)
  {
    QPushButton* button = new QPushButton(QString(buttons[i]).replace(QLatin1Char('&'), QStringLiteral("&&")), parent);
    button->setObjectName(objectName);
    button->setProperty("button", buttons[i]);
    button->setMinimumHeight(ControlHeight);
    QObject::connect(button, &QPushButton::clicked, parent, [clicked, text = buttons[i]]() { clicked(text); });
    grid->addWidget(button, i / 3, i % 3);
    made << button;
  }
  return made;
}

void LabDialogBox::showDialog(const QJsonObject& dialog, const QPoint& at)
{
  m_dialog = dialog;
  m_anchor = at;
  const bool textBox = dialog.value("text_box").toBool();
  QStringList texts;
  for (const QJsonValue& b : dialog.value("buttons").toArray()) texts << b.toString();
  m_title->setText(tr("Menu from %1").arg(dialog.value("object").toString()));
  m_message->setText(dialog.value("message").toString());
  m_message->setVisible(!m_message->text().isEmpty());
  const int channel = dialog.value("channel").toInt();
  m_channel->setText(textBox ? tr("What you type is said on channel %1. Only the script hears it.").arg(channel)
                             : tr("A button says its text on channel %1. Only the script hears it.").arg(channel));
  m_buttons = fillButtonGrid(m_grid, m_buttonBox, textBox ? QStringList() : texts, QStringLiteral("scriptLabMenuButton"),
                             [this](const QString& text)
                             {
                               hide();
                               emit answered(text);
                             });
  for (QPushButton* b : m_buttons) b->setToolTip(tr("Press this button, as the owner's avatar (channel %1)").arg(channel));
  m_buttonBox->setVisible(!m_buttons.isEmpty());
  m_textRow->setVisible(textBox);
  m_text->clear();
  m_send->setEnabled(false);
  setAccessibleName(m_title->text());

  // the tab order follows the screen: buttons row by row (or the field and Send), then Ignore
  QWidget* before = nullptr;
  const QList<QWidget*> order = textBox ? QList<QWidget*>{m_text, m_send, m_ignore}
                                        : [this]() { QList<QWidget*> l; for (QPushButton* b : m_buttons) l << b; l << m_ignore; return l; }();
  for (QWidget* w : order)
  {
    if (before) setTabOrder(before, w);
    before = w;
  }

  applyStyle();
  setFixedWidth(TextSize::scaled(BoxWidth));
  adjustSize();
  place();
  show();
  raise();
  // the keys go to the menu, so it can be answered without the mouse (Esc ignores it)
  QWidget* first = textBox ? static_cast<QWidget*>(m_text) : m_buttons.isEmpty() ? static_cast<QWidget*>(m_ignore) : m_buttons.first();
  first->setFocus(Qt::OtherFocusReason);
}

void LabDialogBox::place()
{
  QWidget* host = parentWidget();
  if (!host) return;
  const QSize size = sizeHint().expandedTo(QSize(width(), 0));
  resize(width(), size.height());
  const QRect area = host->rect().adjusted(Margin, Margin, -Margin, -Margin);
  int x;
  int y;
  if (!host->rect().contains(m_anchor))
  {
    // no click to stand next to: the top right corner
    x = area.right() + 1 - width();
    y = area.top();
  }
  else
  {
    // to the right of the click; to the left when it does not fit there
    x = m_anchor.x() + Gap;
    if (x + width() > area.right() + 1) x = m_anchor.x() - Gap - width();
    y = m_anchor.y() - height() / 2;
  }
  x = qBound(area.left(), x, qMax(area.left(), area.right() + 1 - width()));
  y = qBound(area.top(), y, qMax(area.top(), area.bottom() + 1 - height()));
  move(x, y);
}

void LabDialogBox::sendText()
{
  if (m_text->text().isEmpty()) return;
  const QString text = m_text->text();
  hide();
  emit answered(text);
}

void LabDialogBox::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_Escape)
  {
    hide();
    emit ignored();
    return;
  }
  QFrame::keyPressEvent(event);
}

void LabDialogBox::changeEvent(QEvent* event)
{
  QFrame::changeEvent(event);
  if (!m_styling && (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)) applyStyle();
}

void LabDialogBox::applyStyle()
{
  m_styling = true;
  const bool dark = Theme::isDark();
  // the box: surface with a border and radius.panel, as a menu (DESIGN_SYSTEM.md, "Shapes");
  // its buttons are normal buttons, with the focus ring on keyboard focus
  setStyleSheet(QStringLiteral(
    "QFrame#scriptLabMenuBox { background: %1; border: 1px solid %2; border-radius: %3px; }"
    "QLabel { color: %4; background: transparent; }"
    "QLabel#scriptLabMenuChannel { color: %5; }"
    "QPushButton { background: %6; color: %4; border: 1px solid %2; border-radius: %7px; padding: 0 %8px; }"
    "QPushButton:disabled { color: %9; }"
    "QPushButton:focus { border: 2px solid %10; }"
    "QLineEdit { background: %1; color: %4; border: 1px solid %2; border-radius: %7px; padding: 0 %8px; }"
    "QLineEdit:focus { border: 2px solid %10; }")
    .arg(hex(Theme::Token::Surface, dark), hex(Theme::Token::Border, dark)).arg(RadiusPanel)
    .arg(hex(Theme::Token::Text, dark), hex(Theme::Token::TextMuted, dark), hex(Theme::Token::SurfaceRaised, dark))
    .arg(RadiusControl).arg(SpacingS)
    .arg(hex(Theme::Token::TextDisabled, dark), hex(Theme::Token::FocusRing, dark)));
  m_styling = false;
}
