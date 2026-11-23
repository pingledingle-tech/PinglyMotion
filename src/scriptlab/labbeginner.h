/*
 * PinglyMotion Script Lab - help for someone who has never scripted (job U3)
 *
 * The parts of "Script Lab for beginners" (docs/UX_DECISIONS.md, 6o) that are
 * not the panel itself:
 *
 * - LabExample reads an example object's guide and props (example.json of
 *   job E1, docs/EXAMPLES.md, "The data file example.json").
 * - LabBeginner makes the user's own folders: "Make example project" copies
 *   the first example into Documents/PinglyMotion/Scripts, "New folder" makes
 *   an empty one there. Both only add: nothing is ever overwritten or deleted,
 *   and names and paths have limits.
 * - LabStepCard shows an example's steps (at most five), each with a tick and
 *   a button that does the step.
 * - LabTipCard is one short explanation of a word (prim, RLV, ...); closed, it
 *   never comes back.
 *
 * Colours and sizes come from docs/design_tokens.json through theme.h.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LABBEGINNER_H
#define LABBEGINNER_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QVector3D>

#include "scriptlabsection.h"

class QLabel;
class QPushButton;
class QToolButton;
class QVBoxLayout;

// one example object of examples/objects (job E1)
struct LabExample
{
  struct Prop
  {
    QString name;    // the prim's name, and the prop's
    QString shape;   // box, sphere, cone or torus
    QString joint;   // a joint name such as lHand, or empty when not attached
    QVector3D position;
    QVector3D scale;
    QVector3D rotation;
  };
  struct Entry
  {
    QString id;       // the row in docs/EXAMPLES.md, for example EXCOLOUR-04
    QString part;     // what, word, piece, step, see, next
    QString text;
    QString control;  // steps only: the control the step names
  };

  QString folder;     // the example's folder (absolute)
  QString object;     // the top folder's name
  QString title;
  int order = 0;
  bool worksNow = false;
  QList<Prop> props;
  QList<Entry> guide;

  bool isValid() const { return !object.isEmpty() && !title.isEmpty(); }
  // the texts of one part, in order
  QStringList texts(const QString& part) const;
  // the steps, at most MaxSteps
  QList<Entry> steps() const;
  static constexpr int MaxSteps = 5;
};

namespace LabBeginner
{
  // limits on a folder the program makes (charter: nothing is ever deleted)
  constexpr int MaxNameLength = 40;
  constexpr int MaxPathLength = 200;
  constexpr int MaxCopiedFiles = 64;

  // reads <folder>/example.json; an invalid example and the reason when it cannot be read
  LabExample readExample(const QString& folder, QString* error = nullptr);
  // the example with the lowest "order" in a folder of examples (examples/objects)
  LabExample firstExample(const QString& objectsFolder, QString* error = nullptr);

  // Documents/PinglyMotion/Scripts: the only place the program makes folders
  QString defaultScriptsRoot();

  // "" when the name may be used for a new folder; otherwise why not, in plain words
  QString checkFolderName(const QString& name);
  // makes <root>/<name> (and root itself when missing); "" and the reason when refused.
  // An existing folder is refused: nothing is ever written into it.
  QString makeFolder(const QString& root, const QString& name, QString* error = nullptr);
  // copies the example's scripts, notecards and example.json to <root>/<object>; when that
  // folder is there already it is used as it is (nothing is overwritten). "" and the reason
  // when refused.
  QString copyExample(const LabExample& example, const QString& root, QString* error = nullptr);

  // a word a beginner may not know, with its one-line meaning
  struct Tip
  {
    const char* key;    // stored in the settings when closed
    QString word;       // the word, as the card's title
    QString text;       // the meaning, one or two short sentences
  };
  QList<Tip> jargonTips();
  Tip jargonTip(const QString& key);
}

// The steps of an example: a heading with "N of M done", what you should see, the steps with
// a tick and a button each, and what to try next. The panel decides the buttons and ticks.
class LabStepCard : public ScriptLabFrame
{
  Q_OBJECT

  public:
    struct Step
    {
      QString text;        // the step, from the example's guide
      QString button;      // the button's word (a verb)
      bool enabled = true;
      QString reason;      // why the button is greyed (tooltip and the line under the step)
      bool done = false;
    };

    explicit LabStepCard(QWidget* parent = nullptr);

    void setExample(const QString& title, const QStringList& see, const QStringList& next);
    void setSteps(const QList<Step>& steps);
    QList<Step> steps() const { return m_steps; }
    int doneCount() const;

    QLabel* titleLabel() const { return m_title; }
    QLabel* countLabel() const { return m_count; }
    QLabel* seeLabel() const { return m_see; }
    QLabel* nextLabel() const { return m_next; }
    QToolButton* closeButton() const { return m_close; }
    QList<QPushButton*> stepButtons() const { return m_buttons; }
    QList<QLabel*> ticks() const { return m_ticks; }
    QList<QLabel*> stepTexts() const { return m_texts; }
    QList<QLabel*> reasons() const { return m_reasons; }

    void applyStyle(bool dark);

  signals:
    void stepRequested(int index);
    void closeRequested();

  protected:
    void keyPressEvent(QKeyEvent* event) override;

  private:
    void restyleTicks();

    bool m_dark = true;
    QList<Step> m_steps;
    QLabel* m_title = nullptr;
    QLabel* m_count = nullptr;
    QLabel* m_seeHeading = nullptr;
    QLabel* m_see = nullptr;
    QLabel* m_nextHeading = nullptr;
    QLabel* m_next = nullptr;
    QToolButton* m_close = nullptr;
    QVBoxLayout* m_stepsLayout = nullptr;
    QList<QLabel*> m_ticks;
    QList<QLabel*> m_texts;
    QList<QLabel*> m_reasons;
    QList<QPushButton*> m_buttons;
};

// One word explained: the word in bold, its meaning, and a close button. Closed, it never shows again.
class LabTipCard : public ScriptLabFrame
{
  Q_OBJECT

  public:
    LabTipCard(const LabBeginner::Tip& tip, QWidget* parent = nullptr);
    QString key() const { return m_key; }
    QLabel* wordLabel() const { return m_word; }
    QLabel* textLabel() const { return m_text; }
    QToolButton* closeButton() const { return m_close; }
    void applyStyle(bool dark);

  signals:
    void closed(const QString& key);

  protected:
    void keyPressEvent(QKeyEvent* event) override;

  private:
    QString m_key;
    QLabel* m_word = nullptr;
    QLabel* m_text = nullptr;
    QToolButton* m_close = nullptr;
};

#endif
