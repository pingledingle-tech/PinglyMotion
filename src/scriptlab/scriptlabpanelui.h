/*
 * PinglyMotion Script Lab - the small shared parts of the panel's source files
 *
 * The Script Lab panel's member functions are spread over several source files (scriptlabpanel.cpp,
 * scriptlabpanellive.cpp, scriptlabpanelview.cpp, scriptlabpanelinput.cpp, scriptlabpanelbeginner.cpp,
 * scriptlabpanelwindow.cpp). This header holds what more than one of them uses: the design tokens, the
 * spacings, the flow layout and two small helpers. Only those files include it.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef SCRIPTLABPANELUI_H
#define SCRIPTLABPANELUI_H

#include <QFont>
#include <QJsonArray>
#include <QJsonValue>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QMargins>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStringList>

namespace LabUi
{
  // Copied from docs/design_tokens.json (themes.dark / themes.light). The test
  // tests/scriptlabpanel reads that file and checks every value here.
  struct Tokens
  {
    const char* success;
    const char* danger;
    const char* dangerFill;
    const char* textMuted;
    const char* accent;
    const char* onAccent;
    const char* border;
    const char* focusRing;
    const char* warning;
    const char* surfaceRaised;
    const char* text;
    const char* textDisabled;
  };
  inline const Tokens DarkTokens = {"#56D39A", "#FF7B72", "#C93A32", "#AEB2BA", "#3272DD", "#FFFFFF", "#7C818B", "#9CC2FF", "#F2C14E",
                             "#35373C", "#E6E7EA", "#868A93"};
  inline const Tokens LightTokens = {"#18794E", "#B42318", "#B42318", "#535963", "#1F63C9", "#FFFFFF", "#7A808A", "#1F63C9", "#8A5A00",
                              "#E9EBEF", "#1C1E22", "#7A808A"};

  inline const Tokens& tokens(bool dark) { return dark ? DarkTokens : LightTokens; }

  // design_tokens.json: spacing.s, spacing.m, spacing.l, control_height.default, radius.control
  const int SpacingXs = 4;
  const int SpacingS = 8;
  const int SpacingM = 12;
  const int SpacingL = 16;
  const int ControlHeight = 32;
  const int ControlCompact = 24;   // control_height.compact
  const int ControlLarge = 40;     // control_height.large: empty-state actions
  const int HeldChipHeight = 20;   // the mockup's .chip min-height
  const int RadiusControl = 6;

  const int LogLines = 500;

  // a section name: the "label" type token (body size, semibold)
  inline QLabel* sectionLabel(const QString& text, QWidget* parent)
  {
    QLabel* label = new QLabel(text, parent);
    QFont font = label->font();
    font.setWeight(QFont::DemiBold);
    label->setFont(font);
    return label;
  }

  // A layout that puts its items in a row and goes on in the next row when the
  // row is full, so the "Try:" lines wrap in a narrow docked panel
  class FlowLayout : public QLayout
  {
    public:
      FlowLayout(QWidget* parent, int spacing) : QLayout(parent) { setContentsMargins(0, 0, 0, 0); setSpacing(spacing); }
      ~FlowLayout() override { while (QLayoutItem* item = takeAt(0)) delete item; }
      void addItem(QLayoutItem* item) override { m_items << item; }
      int count() const override { return int(m_items.size()); }
      QLayoutItem* itemAt(int index) const override { return m_items.value(index); }
      QLayoutItem* takeAt(int index) override { return index >= 0 && index < m_items.size() ? m_items.takeAt(index) : nullptr; }
      Qt::Orientations expandingDirections() const override { return {}; }
      bool hasHeightForWidth() const override { return true; }
      int heightForWidth(int width) const override { return place(QRect(0, 0, width, 0), false); }
      void setGeometry(const QRect& rect) override { QLayout::setGeometry(rect); place(rect, true); }
      QSize sizeHint() const override { return minimumSize(); }
      QSize minimumSize() const override
      {
        QSize size;
        for (const QLayoutItem* item : m_items) size = size.expandedTo(item->minimumSize());
        const QMargins m = contentsMargins();
        return size + QSize(m.left() + m.right(), m.top() + m.bottom());
      }

    private:
      int place(const QRect& rect, bool apply) const
      {
        int x = rect.x();
        int y = rect.y();
        int rowHeight = 0;
        for (QLayoutItem* item : m_items)
        {
          const QSize hint = item->sizeHint().expandedTo(item->minimumSize());
          if (x > rect.x() && x + hint.width() > rect.right() + 1)
          {
            x = rect.x();
            y += rowHeight + spacing();
            rowHeight = 0;
          }
          if (apply) item->setGeometry(QRect(QPoint(x, y), hint));
          x += hint.width() + spacing();
          rowHeight = qMax(rowHeight, hint.height());
        }
        return y + rowHeight - rect.y();
      }

      QList<QLayoutItem*> m_items;
  };

  inline QString fieldText(const QJsonValue& value)
  {
    if (value.isString()) return value.toString();
    if (value.isDouble()) return QString::number(value.toDouble(), 'g', 10);
    if (value.isBool()) return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    if (value.isArray())
    {
      QStringList parts;
      for (const QJsonValue& v : value.toArray()) parts << fieldText(v);
      return parts.join(QStringLiteral(", "));
    }
    return QString();
  }
}

#endif
