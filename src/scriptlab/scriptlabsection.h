/*
 * PinglyMotion Script Lab - a folding part of the Script Lab panel
 *
 * One part of the panel in the new design (docs/mockup/index.html, the Script
 * Lab page; docs/UX_DECISIONS.md, 6d): a header button with a chevron and the
 * part's name, a short count on the right ("2", "none", "read only") and the
 * body under it, shown or hidden with the header (click, Space or Enter). The
 * chevron points right when folded and down when open, so the state is a
 * shape too, not only a colour (design charter P10).
 *
 * Colours come from the theme tokens (src/theme.h); sizes from
 * docs/design_tokens.json.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef SCRIPTLABSECTION_H
#define SCRIPTLABSECTION_H

#include <QColor>
#include <QFrame>
#include <QString>

class QLabel;
class QPushButton;
class QVBoxLayout;

// A frame that draws its own rounded border (and fill), so no style sheet sits on a parent of
// labels whose text colour the panel sets (a parent's style sheet would reset those colours)
class ScriptLabFrame : public QFrame
{
  public:
    explicit ScriptLabFrame(QWidget* parent = nullptr) : QFrame(parent) {}

    void setLook(const QColor& border, const QColor& fill, int radius, bool dashed);
    QColor borderColour() const { return m_border; }
    QColor fillColour() const { return m_fill; }   // invalid: no fill
    int radius() const { return m_radius; }
    bool dashed() const { return m_dashed; }

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    QColor m_border;
    QColor m_fill;
    int m_radius = 0;
    bool m_dashed = false;
};

class ScriptLabSection : public ScriptLabFrame
{
  Q_OBJECT

  public:
    // key: a short stable name ("restrictions"), used in the object names
    // ("scriptLabSection_restrictions") and by the tests
    ScriptLabSection(const QString& key, const QString& title, bool expanded, QWidget* parent = nullptr);

    QString key() const { return m_key; }

    void setTitle(const QString& title);
    QString title() const { return m_title; }

    // the short count on the right; empty shows none
    void setCount(const QString& count);
    QString count() const { return m_count; }

    void setExpanded(bool expanded);
    bool isExpanded() const;

    QPushButton* header() const { return m_header; }
    QLabel* countLabel() const { return m_countLabel; }
    QWidget* body() const { return m_body; }
    QVBoxLayout* bodyLayout() const { return m_bodyLayout; }

    // the token colours of the dark or light theme
    void applyStyle(bool dark);

  signals:
    void expandedChanged(bool expanded);

  private:
    void updateHeader();

    QString m_key;
    QString m_title;
    QString m_count;
    bool m_dark = true;
    QPushButton* m_header;
    QLabel* m_countLabel;
    QWidget* m_body;
    QVBoxLayout* m_bodyLayout;
};

#endif
