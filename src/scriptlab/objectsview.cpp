/*
 * PinglyMotion Script Lab - the objects of the last LSL run (step 5, issue 291)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QEvent>
#include <QFont>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QRegularExpression>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "objectsview.h"
#include "scriptlabpanel.h"

namespace
{
  // design_tokens.json: spacing.s
  const int SpacingS = 8;

  QLabel* sectionLabel(const QString& text, QWidget* parent)
  {
    QLabel* label = new QLabel(text, parent);
    QFont font = label->font();
    font.setWeight(QFont::DemiBold);
    label->setFont(font);
    return label;
  }

  QString number(double v)
  {
    return QString::number(v, 'f', 2).remove(QRegularExpression("\\.?0+$"));
  }
}

ScriptLabObjectsView::ScriptLabObjectsView(QWidget* parent) :
  QWidget(parent)
{
  setObjectName("scriptLabObjectsView");
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, SpacingS, 0, 0);
  layout->setSpacing(SpacingS);

  m_heading = sectionLabel(tr("Objects"), this);
  layout->addWidget(m_heading);

  m_list = new QTreeWidget(this);
  m_list->setObjectName("scriptLabObjects");
  m_list->setHeaderLabels({tr("Object"), tr("Where"), tr("Script"), tr("Restrictions")});
  m_list->setRootIsDecorated(false);
  m_list->setUniformRowHeights(true);
  m_list->setSelectionMode(QAbstractItemView::NoSelection);
  m_list->header()->setSectionResizeMode(QHeaderView::Stretch);
  m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_list->setToolTip(tr("The objects of the last script run: where each one is, its script's state, and the RLV restrictions it holds"));
  m_list->setMinimumHeight(4 * m_list->fontMetrics().height() + 2 * SpacingS);
  layout->addWidget(m_list, 1);

  m_dialogHeading = sectionLabel(tr("Dialog"), this);
  layout->addWidget(m_dialogHeading);
  m_dialog = new QLabel(this);
  m_dialog->setObjectName("scriptLabDialog");
  m_dialog->setWordWrap(true);
  m_dialog->setTextFormat(Qt::PlainText);
  m_dialog->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
  m_dialog->setToolTip(tr("The newest dialog (llDialog) an object showed. An AI assistant answers it with lsl_event."));
  layout->addWidget(m_dialog);

  refresh();
}

void ScriptLabObjectsView::setSession(lsl::Session* session)
{
  if (m_connection) disconnect(m_connection);
  m_session = session;
  if (session) m_connection = connect(session, &lsl::Session::changed, this, &ScriptLabObjectsView::refresh);
  refresh();
}

void ScriptLabObjectsView::changeEvent(QEvent* event)
{
  QWidget::changeEvent(event);
  if (event->type() == QEvent::PaletteChange) refresh();
}

void ScriptLabObjectsView::refresh()
{
  const bool dark = ScriptLabPanel::isDark(palette());
  QPalette muted = palette();
  muted.setColor(QPalette::WindowText, ScriptLabPanel::mutedColour(dark));

  m_list->clear();
  const lsl::Sandbox* sandbox = m_session ? m_session->sandbox() : nullptr;
  const QJsonArray objects = sandbox ? sandbox->objectsJson() : QJsonArray();
  for (const QJsonValue& value : objects)
  {
    const QJsonObject o = value.toObject();
    QTreeWidgetItem* item = new QTreeWidgetItem(m_list);
    item->setText(0, o["name"].toString());
    const int point = o["attached"].toInt();
    if (point > 0) item->setText(1, tr("Worn (point %1)").arg(point));
    else
    {
      const QJsonArray p = o["position"].toArray();
      item->setText(1, QStringLiteral("<%1, %2, %3>").arg(number(p[0].toDouble()), number(p[1].toDouble()), number(p[2].toDouble())));
    }
    const bool stopped = o["stopped"].toBool();
    const QString state = o["state"].toString();
    if (stopped) item->setText(2, tr("Stopped: %1").arg(o["stop_reason"].toString()));
    else item->setText(2, state == QLatin1String("default") ? tr("Running") : tr("Running (state %1)").arg(state));
    item->setToolTip(2, item->text(2));
    if (stopped)
    {
      item->setForeground(2, ScriptLabPanel::dangerColour(dark));
      QFont font = item->font(2);
      font.setWeight(QFont::DemiBold);
      item->setFont(2, font);
    }
    QStringList held;
    for (const QJsonValue& r : o["restrictions"].toArray()) held << r.toString();
    item->setText(3, held.isEmpty() ? tr("None") : held.join(QStringLiteral(", ")));
  }
  // hidden until a script has run, so someone who only types RLV lines sees the panel as before
  setVisible(!objects.isEmpty());

  const QJsonObject dialog = sandbox ? sandbox->lastDialog() : QJsonObject();
  if (dialog.isEmpty())
  {
    m_dialog->setText(tr("No dialog yet."));
    m_dialog->setPalette(muted);
    return;
  }
  QStringList buttons;
  for (const QJsonValue& b : dialog["buttons"].toArray()) buttons << b.toString();
  m_dialog->setText(tr("From %1: %2\nButtons: %3 (channel %4)")
                      .arg(dialog["object"].toString(), dialog["message"].toString(), buttons.join(QStringLiteral(", ")))
                      .arg(dialog["channel"].toInt()));
  m_dialog->setPalette(palette());
}

void ScriptLabObjectsView::setEmbedded(bool embedded)
{
  m_embedded = embedded;
  for (QWidget* w : QList<QWidget*>{m_heading, m_dialogHeading, m_dialog}) w->setVisible(!embedded);
  // the part's body already has its own margin
  if (layout()) layout()->setContentsMargins(0, embedded ? 0 : SpacingS, 0, 0);
}
