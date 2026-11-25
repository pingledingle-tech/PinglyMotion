/*
 * PinglyMotion Script Lab - the page «Improve a setup» (queue 1, item 5, parts b and c)
 *
 * The approved mockup's page (docs/mockup/index.html, m2Improve, texts LABTWO-12 to -26) built on what
 * the program measures:
 *
 *   Improve a finished setup                       title
 *   1. Watch ... 2. Record ... 3. Read ...         the three steps, with Import a setup and Record today's behaviour
 *   Weight                                         the weight (labweight.h): memory kept and messages
 *     table: Script | Kept | Values | Sent | Got | Not used
 *   Before and after                               what changed in the weight since the last replay of the recording
 *   What can be better                             the suggestions (labimprove.h); a click shows the script's line
 *   Can this run here?                             one row per script: the verdict, then what works and what is missing
 *
 * The panel fills it from the watched folder (setContent) after every folder run and replay. A greyed
 * button says why in its tip (charter P6); the verdicts carry a word and an icon, not colour alone (P10).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LABIMPROVEVIEW_H
#define LABIMPROVEVIEW_H

#include <QStringList>
#include <QVector>
#include <QWidget>

#include "labimprove.h"
#include "labweight.h"

class QLabel;
class QListWidget;
class QPushButton;
class QTreeWidget;
class QVBoxLayout;

class LabImproveView : public QWidget
{
    Q_OBJECT

  public:
    struct Content
    {
      bool watched = false;          // a folder is watched as one object
      bool ran = false;              // its last run ran (every script compiled, no limit broken)
      QString notRun;                // why it did not run
      bool recording = false;        // the recording is on (Record today's behaviour is greyed)
      LabWeight::Weight weight;
      bool replayed = false;         // the recording was replayed since watching
      bool compared = false;         // that replay had a weight from the one before to compare with
      QStringList changes;           // what changed in the weight (LabWeight::compare)
      QVector<LabImprove::Suggestion> suggestions;
      QVector<LabImprove::Report> reports;
      bool logFull = false;          // the run's event log was full
    };

    explicit LabImproveView(QWidget* parent = nullptr);

    void setContent(const Content& content);
    const Content& content() const { return m_content; }

    // the texts shown now, for the tests: the line under the title, the weight lines, the before-and-after lines,
    // the suggestions, the rows of the table (cells joined with " | ") and of Can this run here (verdict rows)
    QString statusLine() const;
    QStringList weightLines() const;
    QStringList beforeAfterLines() const;
    QStringList suggestionLines() const;
    QStringList tableRows() const;
    QStringList runRows() const;

  signals:
    void importRequested();
    void recordRequested();
    // a suggestion about a line of a script was picked (Enter or a double click)
    void placeRequested(const QString& file, int line);

  protected:
    void changeEvent(QEvent* event) override;

  private:
    void refresh();
    void applyStyle();
    QLabel* heading(const QString& text, const char* name);

    Content m_content;
    QVBoxLayout* m_layout = nullptr;
    QLabel* m_title = nullptr;
    QLabel* m_steps = nullptr;
    QPushButton* m_import = nullptr;
    QPushButton* m_record = nullptr;
    QLabel* m_status = nullptr;
    QLabel* m_weightTitle = nullptr;
    QLabel* m_weight = nullptr;
    QTreeWidget* m_table = nullptr;
    QLabel* m_tableNote = nullptr;
    QLabel* m_beforeTitle = nullptr;
    QLabel* m_before = nullptr;
    QLabel* m_betterTitle = nullptr;
    QListWidget* m_better = nullptr;
    QLabel* m_runTitle = nullptr;
    QTreeWidget* m_run = nullptr;
    QLabel* m_runNote = nullptr;
    bool m_styling = false;
};

#endif
