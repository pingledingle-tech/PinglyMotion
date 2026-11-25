/*
 * PinglyMotion Script Lab - the Script Lab panel: the input buttons (issue 308), the inventory and the menu box over the 3D view
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QComboBox>
#include <QSpinBox>
#include <QFont>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "labtree.h"
#include "lsl/control.h"
#include "objectsview.h"
#include "lucideicons.h"
#include "scriptlabpanelui.h"

#include "scriptlabpanel.h"

using namespace LabUi;

// --- Input buttons (issue 308): touch, chat and dialog answers for the world of the last run

namespace
{
  // the section's keys for the dialog the buttons were made for
  QString dialogKey(const QJsonObject& dialog)
  {
    if (dialog.isEmpty()) return QString();
    QStringList parts{dialog.value("object").toString(), dialog.value("message").toString(), QString::number(dialog.value("channel").toInt())};
    for (const QJsonValue& b : dialog.value("buttons").toArray()) parts << b.toString();
    return parts.join(QChar(0x1f));
  }
}

void ScriptLabPanel::buildInputSection(QWidget* page, QVBoxLayout* layout)
{
  // the section's header says "Send input"
  m_inputHint = new QLabel(page);
  m_inputHint->setObjectName("scriptLabInputHint");
  m_inputHint->setWordWrap(true);
  layout->addWidget(m_inputHint);

  QLabel* objectLabel = new QLabel(tr("To object"), page);
  m_inputObject = new QComboBox(page);
  m_inputObject->setObjectName("scriptLabInputObject");
  m_inputObject->setMinimumHeight(ControlHeight);
  m_inputObject->setToolTip(tr("The object that is touched. Chat is heard by every object within range of the avatar."));
  objectLabel->setBuddy(m_inputObject);

  m_touch = new QPushButton(tr("Touch"), page);
  m_touch->setObjectName("scriptLabTouch");
  m_touch->setMinimumHeight(ControlHeight);

  QLabel* channelLabel = new QLabel(tr("Channel"), page);
  m_chatChannel = new QSpinBox(page);
  m_chatChannel->setObjectName("scriptLabChatChannel");
  m_chatChannel->setRange(-2147483647 - 1, 2147483647);
  m_chatChannel->setMinimumHeight(ControlHeight);
  m_chatChannel->setToolTip(tr("The chat channel. 0 is open chat; scripts and HUDs usually listen on a number."));
  channelLabel->setBuddy(m_chatChannel);

  QLabel* nameLabel = new QLabel(tr("Name"), page);
  m_chatName = new QLineEdit(page);
  m_chatName->setObjectName("scriptLabChatName");
  m_chatName->setMinimumHeight(ControlHeight);
  m_chatName->setPlaceholderText(tr("Owner"));
  m_chatName->setToolTip(tr("Who says it. Leave empty for the owner's avatar."));
  nameLabel->setBuddy(m_chatName);

  QLabel* textLabel = new QLabel(tr("Text"), page);
  m_chatText = new QLineEdit(page);
  m_chatText->setObjectName("scriptLabChatText");
  m_chatText->setMinimumHeight(ControlHeight);
  m_chatText->setClearButtonEnabled(true);
  m_chatText->setToolTip(tr("The chat line to say. Press Enter to say it."));
  textLabel->setBuddy(m_chatText);

  m_say = new QPushButton(tr("Say"), page);
  m_say->setObjectName("scriptLabSay");
  m_say->setMinimumHeight(ControlHeight);

  QGridLayout* grid = new QGridLayout;
  grid->setHorizontalSpacing(SpacingS);
  grid->setVerticalSpacing(SpacingS);
  grid->addWidget(objectLabel, 0, 0);
  grid->addWidget(m_inputObject, 0, 1);
  grid->addWidget(m_touch, 1, 1, Qt::AlignLeft);
  grid->addWidget(channelLabel, 2, 0);
  grid->addWidget(m_chatChannel, 2, 1);
  grid->addWidget(nameLabel, 3, 0);
  grid->addWidget(m_chatName, 3, 1);
  grid->addWidget(textLabel, 4, 0);
  grid->addWidget(m_chatText, 4, 1);
  grid->addWidget(m_say, 5, 1, Qt::AlignLeft);
  grid->setColumnStretch(1, 1);
  layout->addLayout(grid);

  m_inputResult = new QLabel(page);
  m_inputResult->setObjectName("scriptLabInputResult");
  m_inputResult->setWordWrap(true);
  m_inputResult->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_inputResult->hide();
  layout->addWidget(m_inputResult);

  m_inputLog = new QPlainTextEdit(page);
  m_inputLog->setObjectName("scriptLabInputLog");
  m_inputLog->setReadOnly(true);
  m_inputLog->setLineWrapMode(QPlainTextEdit::WidgetWidth);
  m_inputLog->setMaximumBlockCount(LogLines);
  m_inputLog->setToolTip(tr("What each input sent from here made the scripts do. Times are simulated seconds."));
  m_inputLog->setMinimumHeight(3 * m_inputLog->fontMetrics().height() + 2 * SpacingS);
  layout->addWidget(m_inputLog, 2);

  connect(m_touch, &QPushButton::clicked, this, [this]() { sendTouch(m_inputObject->currentText()); });
  auto say = [this]() { sendChat(m_chatChannel->value(), m_chatName->text(), m_chatText->text()); };
  connect(m_say, &QPushButton::clicked, this, say);
  connect(m_chatText, &QLineEdit::returnPressed, this, say);
  connect(m_chatText, &QLineEdit::textChanged, this, [this]() { refreshInput(); });
}

QList<QPushButton*> ScriptLabPanel::dialogButtons() const
{
  QList<QPushButton*> out;
  if (m_dialogButtonBox) out = m_dialogButtonBox->findChildren<QPushButton*>(QString(), Qt::FindDirectChildrenOnly);
  return out;
}

QString ScriptLabPanel::inputBlocker() const
{
  const lsl::Sandbox* sandbox = m_session ? m_session->sandbox() : nullptr;
  if (!sandbox) return tr("Nothing to send to yet: no script has run. An AI assistant starts one with lsl_run; the buttons work then.");
  if (sandbox->allStopped()) return tr("Every script has stopped, so nothing can hear an input. Run the script again.");
  return QString();
}

void ScriptLabPanel::setInputResult(const QString& text, int tone)
{
  m_inputTone = tone;
  m_inputResult->setText(text);
  m_inputResult->setVisible(!text.isEmpty());
  const bool dark = isDark(palette());
  QPalette p = m_inputResult->palette();
  p.setColor(QPalette::WindowText, tone == 1 ? successColour(dark) : tone == 2 ? dangerColour(dark) : mutedColour(dark));
  m_inputResult->setPalette(p);
}

void ScriptLabPanel::refreshInput()
{
  if (!m_inputHint) return;
  const bool dark = isDark(palette());
  const lsl::Sandbox* sandbox = m_session ? m_session->sandbox() : nullptr;
  const QString blocker = inputBlocker();
  const bool world = blocker.isEmpty();

  // the objects, keeping the one chosen
  const QString chosen = m_inputObject->currentText();
  QStringList names;
  if (sandbox)
    for (const QJsonValue& o : sandbox->objectsJson()) names << o.toObject().value("name").toString();
  m_objectsSection->setCount(QString::number(names.size()));
  updateWorldParts();
  bool same = m_inputObject->count() == names.size();
  for (int i = 0; same && i < names.size(); ++i) same = m_inputObject->itemText(i) == names[i];
  if (!same)
  {
    m_inputObject->clear();
    m_inputObject->addItems(names);
    const int keep = names.indexOf(chosen);
    if (keep > 0) m_inputObject->setCurrentIndex(keep);
  }

  const bool hasText = !m_chatText->text().isEmpty();
  m_inputObject->setEnabled(world);
  m_touch->setEnabled(world);
  m_say->setEnabled(world && hasText);
  QString reason = blocker;
  if (world && !hasText) reason = tr("Say is off: type the chat text first. Touch and the dialog buttons work.");
  m_touch->setToolTip(world ? tr("Touch the object, as an avatar clicking it") : blocker);
  m_say->setToolTip(!world ? blocker : hasText ? tr("Say the line on the channel (Enter)") : tr("Type the chat text first"));
  m_inputHint->setText(reason.isEmpty() ? tr("Ready: input goes to the world of the last run, which then runs one more simulated second.") : reason);
  QPalette hint = m_inputHint->palette();
  hint.setColor(QPalette::WindowText, mutedColour(dark));
  m_inputHint->setPalette(hint);
  setInputResult(m_inputResult->text(), m_inputTone);

  // the newest dialog, with one button for each of its buttons
  const QJsonObject dialog = sandbox ? sandbox->lastDialog() : QJsonObject();
  QStringList buttons;
  for (const QJsonValue& b : dialog.value("buttons").toArray()) buttons << b.toString();
  const QString key = dialogKey(dialog);
  if (key != m_shownDialogKey)
  {
    m_shownDialogKey = key;
    // the same grid as the menu box over the 3D view (job A2): rows of three, the first row on top
    LabDialogBox::fillButtonGrid(m_dialogGrid, m_dialogButtonBox, buttons, QStringLiteral("scriptLabDialogButton"),
                                 [this](const QString& text) { answerDialog(text); });
    applyStyle();
  }
  if (dialog.isEmpty()) m_inputDialog->setText(tr("No dialog yet. When a script shows one, its buttons appear here."));
  else m_inputDialog->setText(tr("Dialog from %1").arg(dialog.value("object").toString()));
  m_inputDialogMessage->setText(dialog.isEmpty() ? QString()
                                                 : tr("\"%1\" on channel %2").arg(dialog.value("message").toString())
                                                     .arg(dialog.value("channel").toInt()));
  m_inputDialogMessage->setVisible(!dialog.isEmpty());
  QFont dialogFont = m_inputDialog->font();   // the "label" token for a dialog's heading, body text without one
  dialogFont.setWeight(dialog.isEmpty() ? QFont::Normal : QFont::DemiBold);
  m_inputDialog->setFont(dialogFont);
  {
    // the dialog's buttons come after the objects list and before Send input in the tab order
    QWidget* before = m_objectsView->list();
    for (QPushButton* b : dialogButtons()) { setTabOrder(before, b); before = b; }
    setTabOrder(before, m_inputSection->header());
  }
  for (QPushButton* b : dialogButtons())
  {
    b->setEnabled(world);
    b->setToolTip(world ? tr("Press this button in the dialog, as the owner's avatar (channel %1)").arg(dialog.value("channel").toInt()) : blocker);
  }
  m_dialogButtonBox->setVisible(!buttons.isEmpty());
  refreshInventory();
}

QString ScriptLabPanel::itemCount(int items)
{
  return items == 1 ? tr("1 item") : tr("%1 items").arg(items);
}

QString ScriptLabPanel::inventoryTypeText(int type, int lines)
{
  // the INVENTORY_* numbers of the Second Life wiki, as lsl::Inventory knows them
  switch (type)
  {
    case 0: return tr("Texture");
    case 1: return tr("Sound");
    case 3: return tr("Landmark");
    case 5: return tr("Clothing");
    case 6: return tr("Object");
    case 7: return lines == 1 ? tr("Notecard, 1 line") : tr("Notecard, %1 lines").arg(lines);
    case 10: return tr("Script");
    case 13: return tr("Body part");
    case 20: return tr("Animation");
    case 21: return tr("Gesture");
    case 56: return tr("Setting");
    case 57: return tr("Material");
    default: return tr("Type %1").arg(type);
  }
}

void ScriptLabPanel::refreshInventory()
{
  if (!m_inventory) return;
  const lsl::Sandbox* sandbox = m_session ? m_session->sandbox() : nullptr;
  // the combo box lists the objects in the world's order, so its index is the object's
  const int index = sandbox && m_inputObject->currentIndex() < sandbox->objectCount() ? m_inputObject->currentIndex() : -1;
  m_inventory->clear();
  if (!sandbox || index < 0)
  {
    m_inventorySection->setTitle(tr("Inventory"));
    m_inventorySection->setCount(tr("none"));
    m_inventoryEmpty->show();
    m_inventory->hide();
    m_inventoryNote->hide();
    return;
  }
  // the scripts first, then the items given with the run in their order, as the mockup lists them
  const std::vector<lsl::InventoryItem>& items = sandbox->inventory(index).items();
  QList<const lsl::InventoryItem*> shown;
  for (const lsl::InventoryItem& item : items)
    if (item.type == 10) shown << &item;
  for (const lsl::InventoryItem& item : items)
    if (item.type != 10) shown << &item;
  const bool dark = isDark(palette());
  const QIcon notecard = LucideIcons::icon(QStringLiteral("notebook-text"), palette().color(QPalette::Text), mutedColour(dark));
  for (const lsl::InventoryItem* item : shown)
  {
    QTreeWidgetItem* row = new QTreeWidgetItem(m_inventory);
    row->setText(0, item->name);
    row->setText(1, inventoryTypeText(item->type, int(item->lines.size())));
    row->setToolTip(0, item->name);
    if (item->type == 7) row->setIcon(1, notecard);
  }
  m_inventorySection->setTitle(tr("Inventory of %1").arg(m_inputObject->currentText()));
  m_inventorySection->setCount(itemCount(int(shown.size())));
  m_inventoryEmpty->hide();
  m_inventory->show();
  m_inventoryNote->show();
}

bool ScriptLabPanel::sendInput(QJsonObject request, const QString& what)
{
  if (!m_session)
  {
    setInputResult(tr("Error: %1 was not sent. No script world is connected to this panel.").arg(what), 2);
    return false;
  }
  request["command"] = QStringLiteral("lsl_event");
  const QJsonObject answer = lsl::Control::event(request, m_session);   // the same code as the control command
  if (!answer.value("ok").toBool())
  {
    const QString error = answer.value("error").toString();
    setInputResult(tr("Error: %1 was not sent. %2").arg(what, error), 2);
    m_inputLog->appendPlainText(tr("> %1\nError: %2").arg(what, error));
    return false;
  }
  QStringList lines{tr("> %1").arg(what)};
  for (const QJsonValue& value : answer.value("log").toArray()) lines << logLine(value.toObject());
  if (answer.value("log_truncated").toBool()) lines << tr("(the log was cut here: the input made more lines than it keeps)");
  m_inputLog->appendPlainText(lines.join(QLatin1Char('\n')));
  setInputResult(tr("Sent: %1 (%n log line(s)).", nullptr, int(answer.value("log").toArray().size())).arg(what), 1);
  return true;
}

bool ScriptLabPanel::sendTouch(const QString& object)
{
  QJsonObject request{{"type", "touch"}};
  if (!object.isEmpty()) request["object"] = object;
  return sendInput(request, object.isEmpty() ? tr("Touch") : tr("Touch on %1").arg(object));
}

bool ScriptLabPanel::sendChat(int channel, const QString& name, const QString& text)
{
  const QString who = name.isEmpty() ? tr("Owner") : name;
  const QString what = tr("Chat on channel %1 from %2: %3").arg(channel).arg(who, text);
  if (text.isEmpty())
  {
    setInputResult(tr("Error: %1 was not sent. Type the chat text first.").arg(tr("Chat")), 2);
    return false;
  }
  QJsonObject request{{"type", "chat"}, {"channel", channel}, {"text", text}};
  if (!name.isEmpty()) request["name"] = name;
  return sendInput(request, what);
}

bool ScriptLabPanel::answerDialog(const QString& button)
{
  const lsl::Sandbox* sandbox = m_session ? m_session->sandbox() : nullptr;
  const QJsonObject dialog = sandbox ? sandbox->lastDialog() : QJsonObject();
  if (dialog.isEmpty())
  {
    setInputResult(tr("Error: the dialog answer was not sent. No script has shown a dialog yet."), 2);
    return false;
  }
  QJsonObject request = LabWatcher::dialogAnswer(dialog, button);   // the same input as the menu box (job A2)
  return sendInput(request, tr("Dialog answer \"%1\" to %2").arg(button, dialog.value("object").toString()));
}

// --- The menu box over the 3D view (job A2) -------------------------------------

void ScriptLabPanel::setMenuBoxHost(QWidget* host)
{
  if (m_menuBox) m_menuBox->deleteLater();
  m_menuBox = nullptr;
  m_menuKey.clear();
  if (!host) return;
  m_menuBox = new LabDialogBox(host);
  connect(m_menuBox, &LabDialogBox::answered, this, [this](const QString& text) { answerMenu(text); });
  connect(m_menuBox, &LabDialogBox::ignored, this, [this]()
  {
    m_menuDone = m_menuKey;
    if (m_menuBox && m_menuBox->parentWidget()) m_menuBox->parentWidget()->setFocus(Qt::OtherFocusReason);
  });
  refreshMenuBox();
}

void ScriptLabPanel::refreshMenuBox()
{
  if (!m_menuBox) return;
  const QJsonObject dialog = primPropsShown() ? m_watcher->dialog() : QJsonObject();
  // the time is part of the key: the same menu shown again after an answer is a new menu
  const QString key = dialog.isEmpty() ? QString() : dialogKey(dialog) + QChar(0x1f) + QString::number(dialog.value("t").toDouble(), 'f', 3);
  if (key.isEmpty() || key == m_menuDone)
  {
    m_menuKey = key;
    m_menuBox->hide();
    return;
  }
  if (key == m_menuKey && m_menuBox->isVisible()) return;
  m_menuKey = key;
  m_menuBox->showDialog(dialog, m_menuAnchor);
}

bool ScriptLabPanel::answerMenu(const QString& button)
{
  if (!m_watcher) return false;
  m_menuDone = m_menuKey;
  if (m_menuBox) m_menuBox->hide();
  const LabWatcher::Result result = m_watcher->answerDialog(button);
  if (!result.ok())
  {
    setLiveStatus(tr("Error: the menu answer \"%1\" was not sent. %2").arg(button, result.error), 2);
    return false;
  }
  return true;
}

