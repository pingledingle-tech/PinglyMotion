/*
 * PinglyMotion Script Lab - the script editor in the Live page (lane 1, job F1)
 *
 * The script of the Live page in the program's own text box (QPlainTextEdit,
 * no outside editor component): editable, with LSL syntax colours from the
 * theme tokens (lslhighlighter.h), line numbers with error and warning marks,
 * undo and redo that survive saving, an unsaved mark, Save (Ctrl+S) that
 * writes the file so the folder watch checks and runs it, and a clear line
 * after the run ("Saved and run 15:30. 0 errors.").
 *
 * What the user types is never overwritten: when the file changes on disk
 * while there are unsaved edits, the editor asks once (Keep my changes, Load
 * the new one, Show the difference) and does nothing until answered. Loading
 * the new file is one undo step, so Ctrl+Z brings the edits back.
 *
 * Keys: Ctrl+S save, Ctrl+F find (Enter or F3 next, Shift+Enter previous),
 * Ctrl+G go to line, Ctrl+Z and Ctrl+Shift+Z (Ctrl+Y) undo and redo, Tab and
 * Shift+Tab indent, Enter keeps the indent (one more after "{"), "}" on an
 * empty line goes back one step, a typed quote gets its closing quote. Tab
 * inserts an indent, so Esc then Tab moves the focus on (WCAG 2.1.2); the
 * message line says so the first time the text box gets the focus.
 *
 * Starter help (charter P4): with no script, "New from example" offers the
 * scripts in examples/scriptlab, and a short card says what an RLV line is
 * (hidden when hints are off).
 *
 * Kept apart from scriptlabpanel.cpp so later jobs (suggestions, RLV help,
 * the language choice) change this file and little else. Places for them:
 * LabCodeEdit::keyPressEvent (a suggestion list takes Enter and Tab first)
 * and LabEditor::setProblems (warnings from the RLV table join the list).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LABEDITOR_H
#define LABEDITOR_H

#include <QColor>
#include <QList>
#include <QPlainTextEdit>
#include <QPointer>
#include <QString>
#include <QTime>
#include <QWidget>

class LslHighlighter;
class QLabel;
class QLineEdit;
class QMenu;
class QPushButton;
class QToolButton;
class ScriptLabFrame;

// one error or warning to mark in the text (line and column count from 1)
struct LabProblem
{
  int line = 0;
  int column = 0;
  QString message;
  bool error = true;
};

// The text box: line numbers, marks, indent, quotes, bracket pairs, the Esc-then-Tab way out
class LabCodeEdit : public QPlainTextEdit
{
  Q_OBJECT

  public:
    static constexpr int IndentWidth = 4;   // spaces per indent step, as in examples/scriptlab

    explicit LabCodeEdit(QWidget* parent = nullptr);

    // the errors and warnings to mark: a wavy underline (danger or warning colour), an icon and
    // the bold number in the line number strip, and the message as the tip (charter P10)
    void setProblems(const QList<LabProblem>& problems);
    QList<LabProblem> problems() const { return m_problems; }
    // the marks only (the text box's extra selections are the marks and nothing else)
    QList<QTextEdit::ExtraSelection> problemSelections() const { return extraSelections(); }
    // line numbers (from 1) with an error, or only a warning
    QList<int> errorLines() const;
    QList<int> warningLines() const;

    // the two brackets shown as a pair now (positions in the document), or -1, -1
    QPair<int, int> bracketPair() const { return m_bracketPair; }
    // the position of the bracket that matches the one at pos, or -1 (no bracket, or none matches);
    // brackets inside strings and comments do not count
    static int matchingBracket(const QString& text, int pos);

    // true after Esc, until the next key: Tab then moves the focus on
    bool escapeArmed() const { return m_escArmed; }

    // the colours for the theme; follows the palette by itself
    void setDark(bool dark);
    bool dark() const { return m_dark; }
    LslHighlighter* highlighter() const { return m_highlighter; }

    // width of the line number strip
    int numberWidth() const;
    void paintNumbers(QPaintEvent* event);

  signals:
    // the editor's own keys, given to LabEditor
    void saveRequested();
    void findRequested();
    void goToLineRequested();
    // Esc was pressed (armed) or the focus came in for the first time (the message line explains)
    void escapePressed();
    void firstFocus();

  protected:
    bool event(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void changeEvent(QEvent* event) override;

  private:
    void updateMargin();
    void updateBracketPair();
    void applyFixedFont();
    void fontChanged();
    void indentSelection(bool back);
    QList<QTextEdit::ExtraSelection> makeMarks() const;

    class Numbers;
    Numbers* m_numbers;
    LslHighlighter* m_highlighter;
    QList<LabProblem> m_problems;
    QPair<int, int> m_bracketPair{-1, -1};
    bool m_escArmed = false;
    bool m_hadFocus = false;
    bool m_dark = true;
    bool m_settingFont = false;
};

// The whole editor: tool row, find and go-to-line rows, the changed-outside question, the text box,
// the message line and the starter help
class LabEditor : public QWidget
{
  Q_OBJECT

  public:
    // the answers to "The file changed outside."
    enum OutsideChoice { KeepMine = 0, LoadNew = 1, ShowDifference = 2 };

    explicit LabEditor(QWidget* parent = nullptr);

    // --- the file
    // Shows the script at absolutePath (name: as the panel shows it, relative to the folder).
    // Another file replaces the text only when there are no unsaved edits (false otherwise:
    // nothing changed). The same file is checked against the disk (checkDisk).
    bool openFile(const QString& absolutePath, const QString& name);
    // looks at the shown file on disk again: unchanged does nothing; changed with no unsaved
    // edits loads it (one undo step); changed with unsaved edits asks once. True when it changed.
    bool checkDisk();
    // empties the editor (no folder watched); false (and nothing changes) with unsaved edits
    bool closeFile();
    QString filePath() const { return m_path; }
    QString fileName() const { return m_name; }
    // the text as last loaded from or saved to the file
    QString savedText() const { return m_saved; }
    bool isModified() const;

    // writes the text to the file (Ctrl+S); false with the reason in the message line
    bool save();
    // the result of the run after the last save: "Saved and run 15:30. 0 errors." and so on
    void reportRun(int errors, int warnings, const QString& notRun = QString());
    // true from save() until reportRun()
    bool waitingForRun() const { return m_waitingForRun; }

    // --- the changed-outside question
    bool outsideChangePending() const { return m_outsidePending; }
    void answerOutsideChange(OutsideChoice choice);
    // the difference shown by Show the difference: "- " your line, "+ " the new line, "  " the same
    static QString differenceText(const QString& mine, const QString& theirs);

    // --- go to line and find
    // moves the cursor to the start of the line (from 1); false and a message when there is no such line
    bool goToLine(int line);
    // finds the next (or previous) match after the cursor, wrapping round; selects it.
    // False and a message when there is none. Case does not matter.
    bool find(const QString& text, bool backwards = false);
    void showFindRow();
    void showGoToRow();

    // --- starter help (charter P4)
    void setExamplesFolder(const QString& folder);
    QString examplesFolder() const { return m_examplesFolder; }
    QStringList exampleFiles() const;
    // the folder a new script from an example goes in (the watched folder)
    void setTargetFolder(const QString& folder) { m_targetFolder = folder; }
    // puts the example's text in the editor as a new, unsaved file in the target folder
    // (its own name, or name_2.lsl and so on when taken); false when it cannot be read
    bool newFromExample(const QString& exampleFile);
    void setHintsOn(bool on);
    bool hintsOn() const { return m_hintsOn; }

    // --- the parts, for the panel and the tests
    LabCodeEdit* codeEdit() const { return m_edit; }
    QPushButton* saveButton() const { return m_save; }
    QPushButton* findButton() const { return m_findButton; }
    QPushButton* goToButton() const { return m_goToButton; }
    QLabel* message() const { return m_message; }
    QWidget* findRow() const { return m_findRow; }
    QLineEdit* findField() const { return m_findField; }
    QLabel* findResult() const { return m_findResult; }
    QWidget* goToRow() const { return m_goToRow; }
    QLineEdit* goToField() const { return m_goToField; }
    ScriptLabFrame* outsideBar() const { return m_outsideBar; }
    QLabel* outsideText() const { return m_outsideText; }
    QPushButton* keepButton() const { return m_keep; }
    QPushButton* loadButton() const { return m_load; }
    QPushButton* differenceButton() const { return m_difference; }
    QPlainTextEdit* differenceView() const { return m_differenceView; }
    ScriptLabFrame* starter() const { return m_starter; }
    QLabel* starterHeading() const { return m_starterHeading; }
    QLabel* starterText() const { return m_starterText; }
    QPushButton* exampleButton() const { return m_exampleButton; }
    QMenu* exampleMenu() const { return m_exampleMenu; }
    ScriptLabFrame* rlvCard() const { return m_rlvCard; }
    QLabel* rlvCardTitle() const { return m_rlvTitle; }
    QLabel* rlvCardText() const { return m_rlvText; }
    // "● unsaved changes" while there are unsaved edits, otherwise empty
    QString unsavedMark() const;

    // the clock for "Saved and run 15:30"; the tests give a fixed one (nullptr: the real clock)
    static void setClockForTests(QTime (*clock)());

  signals:
    // the file was written; the panel asks the folder watch to run it
    void saved(const QString& absolutePath);
    // unsaved edits came or went
    void modifiedChanged(bool modified);

  protected:
    void changeEvent(QEvent* event) override;
    bool event(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    void setMessage(const QString& text, int tone);   // 0 muted, 1 success, 2 danger
    void applyStyle();
    void updateState();
    void loadText(const QString& text, bool undoable);
    void showOutsideBar(bool on);
    void fillExampleMenu();
    QString readDisk(bool* ok) const;

    LabCodeEdit* m_edit = nullptr;
    QPushButton* m_save = nullptr;
    QPushButton* m_findButton = nullptr;
    QPushButton* m_goToButton = nullptr;
    QLabel* m_message = nullptr;
    int m_messageTone = 0;
    QWidget* m_findRow = nullptr;
    QLineEdit* m_findField = nullptr;
    QLabel* m_findResult = nullptr;
    QWidget* m_goToRow = nullptr;
    QLineEdit* m_goToField = nullptr;
    ScriptLabFrame* m_outsideBar = nullptr;
    QLabel* m_outsideIcon = nullptr;
    QLabel* m_outsideText = nullptr;
    QPushButton* m_keep = nullptr;
    QPushButton* m_load = nullptr;
    QPushButton* m_difference = nullptr;
    QPlainTextEdit* m_differenceView = nullptr;
    ScriptLabFrame* m_starter = nullptr;
    QLabel* m_starterHeading = nullptr;
    QLabel* m_starterText = nullptr;
    QPushButton* m_exampleButton = nullptr;
    QMenu* m_exampleMenu = nullptr;
    ScriptLabFrame* m_rlvCard = nullptr;
    QLabel* m_rlvIcon = nullptr;
    QLabel* m_rlvTitle = nullptr;
    QLabel* m_rlvText = nullptr;

    QString m_path;
    QString m_name;
    QString m_saved;              // the text as last loaded or saved
    QString m_outsideDisk;        // the disk text the question is about
    QString m_acknowledgedDisk;   // a disk text the user chose to keep their changes over
    bool m_outsidePending = false;
    bool m_waitingForRun = false;
    bool m_lastModified = false;
    QTime m_savedAt;
    QString m_examplesFolder;
    QString m_targetFolder;
    bool m_hintsOn = true;
};

#endif
